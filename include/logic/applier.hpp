/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_APPLIER_HPP
#define GH_OST_APPLIER_HPP

#include "context/migration_context.hpp"
#include "binlog/binlog_entry.hpp"
#include "binlog/binlog_dml_event.hpp"
#include "mysql/connection.hpp"
#include "utils/safe_queue.hpp"
#include <memory>
#include <atomic>
#include <thread>

namespace gh_ost {

// Applies changes to ghost table (row copy and binlog events)
class Applier {
public:
    Applier(MigrationContext& context);
    ~Applier();
    
    // Initialize
    bool Initialize();
    
    // Connection
    void SetConnection(std::shared_ptr<Connection> connection);
    std::shared_ptr<Connection> GetConnection() const { return connection_; }
    
    // Table operations
    bool CreateGhostTable();
    bool CreateChangelogTable();
    bool AlterGhostTable();
    
    // Row copy
    bool CopyRows(const std::string& min_key, const std::string& max_key, uint64_t chunk_size);
    uint64_t GetRowCountEstimate();
    
    // Apply DML events
    bool ApplyDMLEvent(std::shared_ptr<BinlogDMLEvent> event);
    bool ApplyInsert(const BinlogDMLEvent& event);
    bool ApplyUpdate(const BinlogDMLEvent& event);
    bool ApplyDelete(const BinlogDMLEvent& event);
    
    // Apply batched events
    bool ApplyBatch(const std::vector<std::shared_ptr<BinlogEntry>>& entries);
    
    // Changelog operations
    bool WriteChangelog(const std::string& hint, const std::string& value = "");
    bool ReadChangelog(const std::string& hint);
    bool ClearChangelog();
    
    // Heartbeat
    bool WriteHeartbeat();
    
    // Cut-over operations
    bool LockOriginalTable();
    bool UnlockTables();
    bool RenameTables();
    bool AtomicRenameTables();
    
    // Cleanup
    bool DropOldTable();
    bool DropGhostTable();
    bool DropChangelogTable();
    
    // Queue processing
    SafeQueue<std::function<void()>>& GetApplyQueue() { return apply_queue_; }
    
    // Statistics
    uint64_t AppliedEventCount() const { return applied_count_; }
    uint64_t RowCopyCount() const { return row_copy_count_; }
    
private:
    MigrationContext& context_;
    std::shared_ptr<Connection> connection_;
    
    SafeQueue<std::function<void()>> apply_queue_;
    
    std::atomic<uint64_t> applied_count_;
    std::atomic<uint64_t> row_copy_count_;
    std::atomic<bool> running_;
    
    std::thread apply_thread_;
    
    // Apply loop
    void ApplyLoop();
    
    // Helper methods
    std::string BuildInsertSQL(const BinlogRowData& row);
    std::string BuildUpdateSQL(const BinlogRowData& row);
    std::string BuildDeleteSQL(const BinlogRowData& row);
    
    bool ExecuteQuery(const std::string& query);
};

} // namespace gh_ost

#endif // GH_OST_APPLIER_HPP