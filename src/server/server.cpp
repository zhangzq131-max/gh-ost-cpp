/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "server/server.hpp"
#include "server/interactive_commands.hpp"
#include "utils/logger.hpp"
#include "utils/time_utils.hpp"
#include "utils/string_utils.hpp"
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace gh_ost {

Server::Server(MigrationContext& context)
    : context_(context)
    , commands_(context)
    , running_(false)
    , tcp_port_(0)
    , server_socket_(-1) {
}

Server::~Server() {
    Stop();
}

bool Server::Start() {
    if (running_) return true;
    
    auto config = context_.GetConfig();
    
    if (config.status_tcp_port) {
        tcp_port_ = *config.status_tcp_port;
        if (!CreateTCPSocket()) {
            LOG_ERROR("Failed to create TCP socket");
            return false;
        }
    } else if (config.status_socket_path) {
        socket_path_ = *config.status_socket_path;
#ifndef _WIN32
        if (!CreateUnixSocket()) {
            LOG_ERROR("Failed to create Unix socket");
            return false;
        }
#else
        LOG_WARN("Unix sockets not supported on Windows");
        tcp_port_ = 9999;
        if (!CreateTCPSocket()) return false;
#endif
    } else {
        return true;
    }
    
    if (!BindSocket() || !ListenSocket()) {
        LOG_ERROR("Failed to bind/listen socket");
        CloseSocket(server_socket_);
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&Server::ServerLoop, this);
    
    LOG_INFO_FMT("Status server started on port {}", tcp_port_);
    return true;
}

void Server::Stop() {
    running_ = false;
    
    if (server_socket_ >= 0) {
        CloseSocket(server_socket_);
        server_socket_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    LOG_INFO("Status server stopped");
}

std::string Server::GetStatusJSON() const {
    json status;
    
    status["database"] = context_.DatabaseName();
    status["table"] = context_.OriginalTableName();
    status["ghost_table"] = context_.GhostTableName();
    status["status"] = static_cast<int>(context_.GetStatus());
    status["phase"] = context_.PhaseToString(context_.GetCurrentPhase());
    status["throttled"] = context_.IsThrottled();
    
    return status.dump();
}

std::string Server::GetProgressJSON() const {
    auto progress = context_.GetProgress();
    
    json prog;
    prog["total_rows"] = progress.total_rows;
    prog["copied_rows"] = progress.copied_rows;
    prog["percentage"] = progress.percentage_complete;
    prog["events_applied"] = progress.events_applied;
    
    return prog.dump();
}

void Server::ServerLoop() {
    LOG_INFO("Server loop started");
    
    while (running_) {
        int client_socket = AcceptConnection();
        
        if (client_socket < 0) {
            if (running_) TimeUtils::SleepFor(100);
            continue;
        }
        
        HandleConnection(client_socket);
        CloseSocket(client_socket);
    }
    
    LOG_INFO("Server loop stopped");
}

void Server::HandleConnection(int client_socket) {
    std::string command = ReadFromSocket(client_socket);
    
    if (!command.empty()) {
        LOG_DEBUG_FMT("Received command: {}", command);
        std::string response = HandleCommand(command);
        WriteToSocket(client_socket, response);
    }
}

std::string Server::HandleCommand(const std::string& command) {
    auto result = commands_.ProcessCommand(command);
    
    if (result.success) {
        return BuildResponse("OK", result.message);
    } else {
        return BuildResponse("ERROR", result.message);
    }
}

std::string Server::BuildResponse(const std::string& status, const std::string& message) {
    json response;
    response["status"] = status;
    response["message"] = message;
    response["progress"] = json::parse(GetProgressJSON());
    
    return response.dump();
}

bool Server::CreateTCPSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WSAStartup failed");
        return false;
    }
#endif
    
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_socket_ < 0) {
        LOG_ERROR("Failed to create socket");
        return false;
    }
    
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
               reinterpret_cast<char*>(&opt), sizeof(opt));
#else
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    return true;
}

bool Server::CreateUnixSocket() {
#ifndef _WIN32
    server_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket_ < 0) return false;
    unlink(socket_path_.c_str());
    return true;
#else
    return false;
#endif
}

bool Server::BindSocket() {
    if (tcp_port_ > 0) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(tcp_port_);
        
        if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&addr), 
                 sizeof(addr)) < 0) {
            LOG_ERROR_FMT("Failed to bind to port {}", tcp_port_);
            return false;
        }
    }
#ifndef _WIN32
    else if (!socket_path_.empty()) {
        struct sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&addr), 
                 sizeof(addr)) < 0) return false;
    }
#endif
    
    return true;
}

bool Server::ListenSocket() {
    return listen(server_socket_, 10) >= 0;
}

int Server::AcceptConnection() {
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    
    return accept(server_socket_, 
                  reinterpret_cast<struct sockaddr*>(&client_addr),
                  &client_len);
}

void Server::CloseSocket(int socket) {
    if (socket >= 0) {
#ifdef _WIN32
        closesocket(socket);
#else
        close(socket);
#endif
    }
}

std::string Server::ReadFromSocket(int socket) {
    char buffer[1024];
    std::string result;
    
    int bytes_read = recv(socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        result = buffer;
    }
    
    return StringUtils::Trim(result);
}

bool Server::WriteToSocket(int socket, const std::string& data) {
    int bytes_sent = send(socket, data.c_str(), data.length(), 0);
    return bytes_sent == static_cast<int>(data.length());
}

} // namespace gh_ost