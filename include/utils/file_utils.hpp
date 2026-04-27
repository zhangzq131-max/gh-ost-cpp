/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_FILE_UTILS_HPP
#define GH_OST_FILE_UTILS_HPP

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <fstream>

namespace gh_ost {

class FileUtils {
public:
    // File existence
    static bool Exists(const std::string& path);
    static bool IsFile(const std::string& path);
    static bool IsDirectory(const std::string& path);
    
    // File read
    static std::optional<std::string> ReadFile(const std::string& path);
    static std::optional<std::vector<std::string>> ReadLines(const std::string& path);
    static std::optional<std::vector<uint8_t>> ReadBinary(const std::string& path);
    
    // File write
    static bool WriteFile(const std::string& path, const std::string& content);
    static bool WriteLines(const std::string& path, const std::vector<std::string>& lines);
    static bool WriteBinary(const std::string& path, const std::vector<uint8_t>& data);
    static bool Append(const std::string& path, const std::string& content);
    
    // File operations
    static bool CreateFile(const std::string& path);
    static bool CreateDirectory(const std::string& path);
    static bool Delete(const std::string& path);
    static bool DeleteIfExists(const std::string& path);
    static bool Rename(const std::string& old_path, const std::string& new_path);
    static bool Copy(const std::string& src, const std::string& dst);
    
    // Directory operations
    static std::vector<std::string> ListFiles(const std::string& directory);
    static std::vector<std::string> ListFilesRecursive(const std::string& directory);
    static bool EnsureDirectory(const std::string& path);
    
    // File info
    static uint64_t FileSize(const std::string& path);
    static std::string BaseName(const std::string& path);
    static std::string DirectoryName(const std::string& path);
    static std::string Extension(const std::string& path);
    static std::string FileNameWithoutExtension(const std::string& path);
    
    // Path operations
    static std::string NormalizePath(const std::string& path);
    static std::string JoinPath(const std::string& a, const std::string& b);
    static std::string JoinPaths(const std::vector<std::string>& parts);
    static std::string AbsolutePath(const std::string& path);
    
    // Temporary file
    static std::string TempDirectory();
    static std::string TempFileName();
    static std::string CreateTempFile();
    static std::string CreateTempFile(const std::string& prefix);
    static bool DeleteTempFile(const std::string& path);
    
    // File permissions
    static bool IsReadable(const std::string& path);
    static bool IsWritable(const std::string& path);
    static bool IsExecutable(const std::string& path);
    
    // Flag file operations (used by gh-ost for signaling)
    static bool CreateFlagFile(const std::string& path);
    static bool FlagFileExists(const std::string& path);
    static bool RemoveFlagFile(const std::string& path);
    static bool WaitForFlagFile(const std::string& path, uint64_t timeout_millis);
    
    // PID file operations
    static bool WritePidFile(const std::string& path, uint64_t pid);
    static std::optional<uint64_t> ReadPidFile(const std::string& path);
    static bool RemovePidFile(const std::string& path);
    
private:
    FileUtils() = delete;
};

} // namespace gh_ost

#endif // GH_OST_FILE_UTILS_HPP