/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_BINLOG_COORDINATES_HPP
#define GH_OST_BINLOG_COORDINATES_HPP

#include <string>
#include <cstdint>
#include <optional>
#include <functional>

namespace gh_ost {

// Binlog position coordinates (file name + position)
class BinlogCoordinates {
public:
    BinlogCoordinates();
    BinlogCoordinates(const std::string& file, uint64_t position);
    
    // Getters
    std::string File() const { return file_; }
    uint64_t Position() const { return position_; }
    
    // String representation
    std::string ToString() const;  // "mysql-bin.000123:456"
    
    // Comparison
    bool IsSameAs(const BinlogCoordinates& other) const;
    bool IsValid() const;
    
    // Ordering (for comparing positions in same file)
    bool operator==(const BinlogCoordinates& other) const;
    bool operator!=(const BinlogCoordinates& other) const;
    bool operator<(const BinlogCoordinates& other) const;
    bool operator<=(const BinlogCoordinates& other) const;
    bool operator>(const BinlogCoordinates& other) const;
    bool operator>=(const BinlogCoordinates& other) const;
    
    // Parse from string
    static std::optional<BinlogCoordinates> Parse(const std::string& str);
    
    // Calculate file sequence number
    uint64_t GetSequence() const;
    
    // Compare file sequences
    bool IsSameFile(const BinlogCoordinates& other) const;
    bool IsLaterFile(const BinlogCoordinates& other) const;
    
private:
    std::string file_;
    uint64_t position_;
    
    static uint64_t ParseSequence(const std::string& file);
};

// GTID coordinates (for GTID-based replication)
class GTIDCoordinates {
public:
    GTIDCoordinates();
    GTIDCoordinates(const std::string& gtid);
    GTIDCoordinates(const std::string& domain, uint64_t sequence);
    
    std::string Domain() const { return domain_; }
    uint64_t Sequence() const { return sequence_; }
    std::string ToString() const;  // "3E11FA47-71CA-11E1-9E33-C80AA9429562:123"
    
    bool IsValid() const;
    bool operator==(const GTIDCoordinates& other) const;
    bool operator<(const GTIDCoordinates& other) const;
    
    static std::optional<GTIDCoordinates> Parse(const std::string& str);
    
private:
    std::string domain_;
    uint64_t sequence_;
};

// Set of GTID coordinates
class GTIDSet {
public:
    GTIDSet();
    
    void Add(const GTIDCoordinates& gtid);
    void Add(const std::string& domain, uint64_t sequence);
    bool Contains(const GTIDCoordinates& gtid) const;
    
    std::string ToString() const;
    bool IsEmpty() const;
    
    void Parse(const std::string& gtid_str);
    
private:
    std::vector<GTIDCoordinates> gtids_;
};

} // namespace gh_ost

// Hash function
namespace std {
    template<>
    struct hash<gh_ost::BinlogCoordinates> {
        size_t operator()(const gh_ost::BinlogCoordinates& coords) const {
            return hash<string>()(coords.File()) ^ (hash<uint64_t>()(coords.Position()) << 1);
        }
    };
}

#endif // GH_OST_BINLOG_COORDINATES_HPP