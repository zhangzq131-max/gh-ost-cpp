/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "binlog/binlog_entry.hpp"
#include "binlog/binlog_dml_event.hpp"

namespace gh_ost {

BinlogEntry::BinlogEntry()
    : event_type_(BinlogEventType::Unknown)
    , timestamp_(0)
    , event_size_(0)
    , server_id_(0) {}

BinlogEntry::~BinlogEntry() {}

bool BinlogEntry::IsForTable(const std::string& database, const std::string& table) const {
    if (!dml_event_) return false;
    
    return dml_event_->Database() == database && dml_event_->Table() == table;
}

bool BinlogEntry::IsDML() const {
    return event_type_ == BinlogEventType::WriteRows ||
           event_type_ == BinlogEventType::UpdateRows ||
           event_type_ == BinlogEventType::DeleteRows;
}

bool BinlogEntry::IsHeartbeat() const {
    // Heartbeat events are typically changelog events or special markers
    if (!dml_event_) return false;
    return dml_event_->Table().find("_gh_ost_") != std::string::npos;
}

std::string BinlogEntry::Database() const {
    if (dml_event_) return dml_event_->Database();
    return "";
}

std::string BinlogEntry::Table() const {
    if (dml_event_) return dml_event_->Table();
    return "";
}

} // namespace gh_ost