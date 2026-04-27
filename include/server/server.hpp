/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_SERVER_HPP
#define GH_OST_SERVER_HPP

#include "context/migration_context.hpp"
#include "server/interactive_commands.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace gh_ost {

class Server {
public:
    Server(MigrationContext& context);
    ~Server();
    
    bool Start();
    void Stop();
    bool IsRunning() const { return running_; }
    
    void SetTCPPort(uint16_t port);
    void SetUnixSocketPath(const std::string& path);
    
    std::string GetStatusJSON() const;
    std::string GetProgressJSON() const;
    
    void SetCommandHandler(InteractiveCommandHandler handler);
    
private:
    MigrationContext& context_;
    InteractiveCommands commands_;
    
    std::atomic<bool> running_;
    std::thread server_thread_;
    
    uint16_t tcp_port_;
    std::string socket_path_;
    
    int server_socket_;
    
    void ServerLoop();
    void HandleConnection(int client_socket);
    std::string HandleCommand(const std::string& command);
    std::string BuildResponse(const std::string& status, const std::string& message);
    
    bool CreateTCPSocket();
    bool CreateUnixSocket();
    bool BindSocket();
    bool ListenSocket();
    int AcceptConnection();
    
    void CloseSocket(int socket);
    std::string ReadFromSocket(int socket);
    bool WriteToSocket(int socket, const std::string& data);
};

} // namespace gh_ost

#endif // GH_OST_SERVER_HPP