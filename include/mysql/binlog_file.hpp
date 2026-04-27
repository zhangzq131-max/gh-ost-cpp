/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_MYSQL_BINLOG_FILE_HPP
#define GH_OST_MYSQL_BINLOG_FILE_HPP

#include <string>
#include <cstdint>
#include <vector>

namespace gh_ost {

/**
 * BinlogFile represents a MySQL binary log file with its metadata
 */
class BinlogFile {
public:
    BinlogFile();
    BinlogFile(const std::string& filename, uint64_t start_position = 4);
    ~BinlogFile();

    // Getters
    const std::string& Filename() const { return filename_; }
    uint64_t StartPosition() const { return start_position_; }
    uint64_t EndPosition() const { return end_position_; }
    
    // Setters
    void SetFilename(const std::string& filename) { filename_ = filename; }
    void SetStartPosition(uint64_t pos) { start_position_ = pos; }
    void SetEndPosition(uint64_t pos) { end_position_ = pos; }

    // Utility methods
    std::string FullPath(const std::string& log_dir) const;
    bool IsValid() const { return !filename_.empty(); }
    bool ContainsPosition(uint64_t position) const;
    
    // Parse binlog filename to extract sequence number
    static uint64_t ExtractSequenceNumber(const std::string& filename);
    static std::string GenerateFilename(uint64_t sequence_number);

    // Comparison operators for sorting
    bool operator<(const BinlogFile& other) const;
    bool operator==(const BinlogFile& other) const;

private:
    std::string filename_;       // e.g., "mysql-bin.000001"
    uint64_t start_position_;    // Start position in file (usually 4)
    uint64_t end_position_;      // End position in file
};

/**
 * BinlogFileSet represents a collection of binlog files
 */
class BinlogFileSet {
public:
    BinlogFileSet();
    ~BinlogFileSet();

    // File management
    void AddFile(const BinlogFile& file);
    void RemoveFile(const std::string& filename);
    void Clear();
    
    // Queries
    bool Empty() const { return files_.empty(); }
    size_t Size() const { return files_.size(); }
    
    const BinlogFile* FindFile(const std::string& filename) const;
    const BinlogFile* FindFileContaining(uint64_t position) const;
    
    // Get ordered files
    std::vector<BinlogFile> GetOrderedFiles() const;
    
    // Get next/previous file
    const BinlogFile* GetNextFile(const std::string& filename) const;
    const BinlogFile* GetPreviousFile(const std::string& filename) const;

private:
    std::vector<BinlogFile> files_;
};

} // namespace gh_ost

#endif // GH_OST_MYSQL_BINLOG_FILE_HPP