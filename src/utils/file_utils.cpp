/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "utils/file_utils.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace fs = std::filesystem;

namespace gh_ost {

// File existence
bool FileUtils::Exists(const std::string& path) {
    return fs::exists(path);
}

bool FileUtils::IsFile(const std::string& path) {
    return fs::is_regular_file(path);
}

bool FileUtils::IsDirectory(const std::string& path) {
    return fs::is_directory(path);
}

// File read
std::optional<std::string> FileUtils::ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::optional<std::vector<std::string>> FileUtils::ReadLines(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::optional<std::vector<uint8_t>> FileUtils::ReadBinary(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

// File write
bool FileUtils::WriteFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(content.data(), content.size());
    return file.good();
}

bool FileUtils::WriteLines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    for (const auto& line : lines) {
        file << line << '\n';
    }
    return file.good();
}

bool FileUtils::WriteBinary(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool FileUtils::Append(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::out | std::ios::app);
    if (!file.is_open()) return false;
    
    file.write(content.data(), content.size());
    return file.good();
}

// File operations
bool FileUtils::CreateFile(const std::string& path) {
    std::ofstream file(path, std::ios::out);
    return file.is_open();
}

bool FileUtils::CreateDirectory(const std::string& path) {
    try {
        return fs::create_directory(path);
    } catch (...) {
        return false;
    }
}

bool FileUtils::Delete(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

bool FileUtils::DeleteIfExists(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

bool FileUtils::Rename(const std::string& old_path, const std::string& new_path) {
    try {
        fs::rename(old_path, new_path);
        return true;
    } catch (...) {
        return false;
    }
}

bool FileUtils::Copy(const std::string& src, const std::string& dst) {
    try {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        return true;
    } catch (...) {
        return false;
    }
}

// Directory operations
std::vector<std::string> FileUtils::ListFiles(const std::string& directory) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (...) {}
    return files;
}

std::vector<std::string> FileUtils::ListFilesRecursive(const std::string& directory) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (...) {}
    return files;
}

bool FileUtils::EnsureDirectory(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (...) {
        return false;
    }
}

// File info
uint64_t FileUtils::FileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

std::string FileUtils::BaseName(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string FileUtils::DirectoryName(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::string FileUtils::Extension(const std::string& path) {
    return fs::path(path).extension().string();
}

std::string FileUtils::FileNameWithoutExtension(const std::string& path) {
    return fs::path(path).stem().string();
}

// Path operations
std::string FileUtils::NormalizePath(const std::string& path) {
    try {
        return fs::path(path).lexically_normal().string();
    } catch (...) {
        return path;
    }
}

std::string FileUtils::JoinPath(const std::string& a, const std::string& b) {
    return (fs::path(a) / fs::path(b)).string();
}

std::string FileUtils::JoinPaths(const std::vector<std::string>& parts) {
    if (parts.empty()) return "";
    
    fs::path result(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        result /= parts[i];
    }
    return result.string();
}

std::string FileUtils::AbsolutePath(const std::string& path) {
    try {
        return fs::absolute(path).string();
    } catch (...) {
        return path;
    }
}

// Temporary file
std::string FileUtils::TempDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return std::string(buffer);
#else
    return "/tmp";
#endif
}

std::string FileUtils::TempFileName() {
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1);
    
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return std::string(buffer) + "gh-ost-" + std::to_string(id) + ".tmp";
#else
    return "/tmp/gh-ost-" + std::to_string(id) + ".tmp";
#endif
}

std::string FileUtils::CreateTempFile() {
    return CreateTempFile("gh-ost");
}

std::string FileUtils::CreateTempFile(const std::string& prefix) {
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1);
    std::string path = JoinPath(TempDirectory(), prefix + "-" + std::to_string(id) + ".tmp");
    
    if (CreateFile(path)) {
        return path;
    }
    return "";
}

bool FileUtils::DeleteTempFile(const std::string& path) {
    return DeleteIfExists(path);
}

// File permissions
bool FileUtils::IsReadable(const std::string& path) {
    std::ifstream file(path);
    return file.is_open();
}

bool FileUtils::IsWritable(const std::string& path) {
    std::ofstream file(path, std::ios::out | std::ios::app);
    return file.is_open();
}

bool FileUtils::IsExecutable(const std::string& path) {
#ifdef _WIN32
    return Exists(path);  // Windows doesn't use executable permission like Unix
#else
    return access(path.c_str(), X_OK) == 0;
#endif
}

// Flag file operations
bool FileUtils::CreateFlagFile(const std::string& path) {
    return CreateFile(path);
}

bool FileUtils::FlagFileExists(const std::string& path) {
    return Exists(path);
}

bool FileUtils::RemoveFlagFile(const std::string& path) {
    return DeleteIfExists(path);
}

bool FileUtils::WaitForFlagFile(const std::string& path, uint64_t timeout_millis) {
    auto start = std::chrono::steady_clock::now();
    
    while (!Exists(path)) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        
        if (elapsed >= timeout_millis) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return true;
}

// PID file operations
bool FileUtils::WritePidFile(const std::string& path, uint64_t pid) {
    return WriteFile(path, std::to_string(pid));
}

std::optional<uint64_t> FileUtils::ReadPidFile(const std::string& path) {
    auto content = ReadFile(path);
    if (!content) return std::nullopt;
    
    try {
        return std::stoull(*content);
    } catch (...) {
        return std::nullopt;
    }
}

bool FileUtils::RemovePidFile(const std::string& path) {
    return DeleteIfExists(path);
}

} // namespace gh_ost