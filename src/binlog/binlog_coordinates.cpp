/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "binlog/binlog_coordinates.hpp"
#include "utils/string_utils.hpp"
#include <sstream>

namespace gh_ost {

BinlogCoordinates::BinlogCoordinates()
    : file_(""), position_(0) {}

BinlogCoordinates::BinlogCoordinates(const std::string& file, uint64_t position)
    : file_(file), position_(position) {}

std::string BinlogCoordinates::ToString() const {
    if (IsValid()) {
        return file_ + ":" + std::to_string(position_);
    }
    return "";
}

bool BinlogCoordinates::IsSameAs(const BinlogCoordinates& other) const {
    return file_ == other.file_ && position_ == other.position_;
}

bool BinlogCoordinates::IsValid() const {
    return !file_.empty() && position_ > 0;
}

bool BinlogCoordinates::operator==(const BinlogCoordinates& other) const {
    return IsSameAs(other);
}

bool BinlogCoordinates::operator!=(const BinlogCoordinates& other) const {
    return !IsSameAs(other);
}

bool BinlogCoordinates::operator<(const BinlogCoordinates& other) const {
    uint64_t seq1 = GetSequence();
    uint64_t seq2 = other.GetSequence();
    
    if (seq1 != seq2) return seq1 < seq2;
    return position_ < other.position_;
}

bool BinlogCoordinates::operator<=(const BinlogCoordinates& other) const {
    return *this == other || *this < other;
}

bool BinlogCoordinates::operator>(const BinlogCoordinates& other) const {
    return !(*this <= other);
}

bool BinlogCoordinates::operator>=(const BinlogCoordinates& other) const {
    return !(*this < other);
}

std::optional<BinlogCoordinates> BinlogCoordinates::Parse(const std::string& str) {
    auto parts = StringUtils::Split(str, ':');
    if (parts.size() != 2) return std::nullopt;
    
    auto pos = StringUtils::ParseUInt64(parts[1]);
    if (!pos) return std::nullopt;
    
    return BinlogCoordinates(parts[0], *pos);
}

uint64_t BinlogCoordinates::GetSequence() const {
    return ParseSequence(file_);
}

bool BinlogCoordinates::IsSameFile(const BinlogCoordinates& other) const {
    return file_ == other.file_;
}

bool BinlogCoordinates::IsLaterFile(const BinlogCoordinates& other) const {
    return GetSequence() > other.GetSequence();
}

uint64_t BinlogCoordinates::ParseSequence(const std::string& file) {
    // Extract sequence from binlog file name like "mysql-bin.000123"
    size_t last_dot = file.rfind('.');
    if (last_dot != std::string::npos && last_dot + 1 < file.length()) {
        std::string seq_str = file.substr(last_dot + 1);
        return StringUtils::ParseUInt64(seq_str).value_or(0);
    }
    return 0;
}

// GTIDCoordinates implementation
GTIDCoordinates::GTIDCoordinates()
    : domain_(""), sequence_(0) {}

GTIDCoordinates::GTIDCoordinates(const std::string& gtid) {
    auto parsed = Parse(gtid);
    if (parsed) {
        domain_ = parsed->domain_;
        sequence_ = parsed->sequence_;
    }
}

GTIDCoordinates::GTIDCoordinates(const std::string& domain, uint64_t sequence)
    : domain_(domain), sequence_(sequence) {}

std::string GTIDCoordinates::ToString() const {
    return domain_ + ":" + std::to_string(sequence_);
}

bool GTIDCoordinates::IsValid() const {
    return !domain_.empty() && sequence_ > 0;
}

bool GTIDCoordinates::operator==(const GTIDCoordinates& other) const {
    return domain_ == other.domain_ && sequence_ == other.sequence_;
}

bool GTIDCoordinates::operator<(const GTIDCoordinates& other) const {
    if (domain_ != other.domain_) return domain_ < other.domain_;
    return sequence_ < other.sequence_;
}

std::optional<GTIDCoordinates> GTIDCoordinates::Parse(const std::string& str) {
    // Format: UUID:sequence (e.g., "3E11FA47-71CA-11E1-9E33-C80AA9429562:123")
    size_t colon_pos = str.find(':');
    if (colon_pos == std::string::npos) return std::nullopt;
    
    std::string domain = str.substr(0, colon_pos);
    std::string seq_str = str.substr(colon_pos + 1);
    
    auto seq = StringUtils::ParseUInt64(seq_str);
    if (!seq) return std::nullopt;
    
    return GTIDCoordinates(domain, *seq);
}

// GTIDSet implementation
GTIDSet::GTIDSet() {}

void GTIDSet::Add(const GTIDCoordinates& gtid) {
    if (!gtid.IsValid()) return;
    
    // Check if domain already exists
    for (auto& existing : gtids_) {
        if (existing.Domain() == gtid.Domain()) {
            if (existing.Sequence() < gtid.Sequence()) {
                existing = gtid;
            }
            return;
        }
    }
    
    gtids_.push_back(gtid);
}

void GTIDSet::Add(const std::string& domain, uint64_t sequence) {
    Add(GTIDCoordinates(domain, sequence));
}

bool GTIDSet::Contains(const GTIDCoordinates& gtid) const {
    for (const auto& existing : gtids_) {
        if (existing.Domain() == gtid.Domain() && 
            existing.Sequence() >= gtid.Sequence()) {
            return true;
        }
    }
    return false;
}

std::string GTIDSet::ToString() const {
    if (gtids_.empty()) return "";
    
    std::ostringstream oss;
    for (size_t i = 0; i < gtids_.size(); ++i) {
        oss << gtids_[i].ToString();
        if (i < gtids_.size() - 1) oss << ",";
    }
    return oss.str();
}

bool GTIDSet::IsEmpty() const {
    return gtids_.empty();
}

void GTIDSet::Parse(const std::string& gtid_str) {
    // Parse GTID set string (comma-separated)
    auto parts = StringUtils::Split(gtid_str, ',');
    for (const auto& part : parts) {
        auto gtid = GTIDCoordinates::Parse(StringUtils::Trim(part));
        if (gtid) {
            Add(*gtid);
        }
    }
}

} // namespace gh_ost