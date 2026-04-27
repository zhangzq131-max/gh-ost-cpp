/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_BINLOG_READER_HPP
#define GH_OST_BINLOG_READER_HPP

#include "binlog_coordinates.hpp"
#include "binlog_entry.hpp"
#include "binlog_dml_event.hpp"
#include "mysql/connection.hpp"
#include "mysql/instance_key.hpp"
#include "gh_ost/types.hpp"
#include "utils/safe_queue.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

namespace gh_ost {

// Binlog reader interface
class BinlogReader {
public:
    // Callback for binlog events
    using EventCallback = std::function<void(std::shared_ptr<BinlogEntry>)>;
    
    BinlogReader();
    virtual ~BinlogReader();
    
    // Configuration
    void SetReplicaKey(const InstanceKey& replica);
    void SetReplicaConnection(std::shared_ptr<Connection> connection);
    
    void SetDatabase(const std::string& database);
    void SetTable(const std::string& table);
    
    void SetStartingCoordinates(const BinlogCoordinates& coords);
    void SetEventCallback(EventCallback callback);
    
    // Control
    virtual bool Start();
    void Stop();
    bool IsRunning() const { return running_; }
    
    // Status
    BinlogCoordinates CurrentCoordinates() const { return current_coordinates_; }
    uint64_t EventsProcessed() const { return events_processed_; }
    uint64_t DMLEventsProcessed() const { return dml_events_processed_; }
    
    // Error handling
    std::string GetLastError() const { return last_error_; }
    
protected:
    InstanceKey replica_key_;
    std::shared_ptr<Connection> connection_;
    std::string database_;
    std::string table_;
    BinlogCoordinates starting_coordinates_;
    BinlogCoordinates current_coordinates_;
    EventCallback event_callback_;
    
    std::atomic<bool> running_;
    std::atomic<bool> stopped_;
    std::thread reader_thread_;
    
    uint64_t events_processed_;
    uint64_t dml_events_processed_;
    std::string last_error_;
    
    // Internal processing
    virtual void ReadLoop();
    virtual bool ReadNextEvent();
    virtual std::shared_ptr<BinlogEntry> ParseEvent(const std::string& event_data);
};

// Go-MySQL binlog reader implementation (using go-mysql style protocol)
class GoMySQLBinlogReader : public BinlogReader {
public:
    GoMySQLBinlogReader();
    ~GoMySQLBinlogReader() override;
    
    bool Start() override;
    
private:
    // Connect as a replication slave
    bool ConnectAsReplica();
    bool RegisterAsSlave();
    bool RequestBinlogStream();
    
    void ReadLoop() override;
    std::shared_ptr<BinlogEntry> ParseEvent(const std::string& event_data) override;
    
    // Event parsing helpers
    std::shared_ptr<BinlogEntry> ParseRotateEvent(const std::string& data);
    std::shared_ptr<BinlogEntry> ParseFormatDescriptionEvent(const std::string& data);
    std::shared_ptr<BinlogEntry> ParseQueryEvent(const std::string& data);
    std::shared_ptr<BinlogEntry> ParseTableMapEvent(const std::string& data);
    std::shared_ptr<BinlogEntry> ParseWriteRowsEvent(const std::string& data);   // INSERT
    std::shared_ptr<BinlogEntry> ParseUpdateRowsEvent(const std::string& data);  // UPDATE
    std::shared_ptr<BinlogEntry> ParseDeleteRowsEvent(const std::string& data);  // DELETE
    
    // Table map cache (table_id -> table_name)
    std::unordered_map<uint64_t, std::pair<std::string, std::string>> table_map_cache_;
    
    // Current binlog file being read
    std::string current_file_;
    uint64_t current_position_;
    
    // Server ID for slave registration
    uint32_t slave_server_id_;
};

} // namespace gh_ost

#endif // GH_OST_BINLOG_READER_HPP