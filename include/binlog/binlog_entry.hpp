/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_BINLOG_ENTRY_HPP
#define GH_OST_BINLOG_ENTRY_HPP

#include "binlog_coordinates.hpp"
#include "binlog_dml_event.hpp"
#include "gh_ost/types.hpp"
#include <memory>
#include <string>
#include <cstdint>

namespace gh_ost {

// Binlog entry representing a single event from the binary log
class BinlogEntry {
public:
    BinlogEntry();
    ~BinlogEntry();
    
    // Coordinates
    BinlogCoordinates Coordinates() const { return coordinates_; }
    void SetCoordinates(const BinlogCoordinates& coords) { coordinates_ = coords; }
    
    // DML event (for data events)
    std::shared_ptr<BinlogDMLEvent> DmlEvent() const { return dml_event_; }
    void SetDmlEvent(std::shared_ptr<BinlogDMLEvent> event) { dml_event_ = event; }
    
    // Event type
    BinlogEventType EventType() const { return event_type_; }
    void SetEventType(BinlogEventType type) { event_type_ = type; }
    
    // Event timestamp
    uint64_t Timestamp() const { return timestamp_; }
    void SetTimestamp(uint64_t ts) { timestamp_ = ts; }
    
    // Is this event for the target table?
    bool IsForTable(const std::string& database, const std::string& table) const;
    
    // Is this a DML event?
    bool IsDML() const;
    
    // Is this a heartbeat/changelog event?
    bool IsHeartbeat() const;
    
    // Database and table name
    std::string Database() const;
    std::string Table() const;
    
    // Raw event data
    std::string RawData() const { return raw_data_; }
    void SetRawData(const std::string& data) { raw_data_ = data; }
    
    // Event size
    uint64_t EventSize() const { return event_size_; }
    void SetEventSize(uint64_t size) { event_size_ = size; }
    
    // Server ID (from binlog header)
    uint32_t ServerId() const { return server_id_; }
    void SetServerId(uint32_t id) { server_id_ = id; }
    
private:
    BinlogCoordinates coordinates_;
    std::shared_ptr<BinlogDMLEvent> dml_event_;
    BinlogEventType event_type_;
    uint64_t timestamp_;
    uint64_t event_size_;
    uint32_t server_id_;
    std::string raw_data_;
};

} // namespace gh_ost

#endif // GH_OST_BINLOG_ENTRY_HPP