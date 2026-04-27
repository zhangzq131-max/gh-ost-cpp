/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_MIGRATION_CONTEXT_HPP
#define GH_OST_MIGRATION_CONTEXT_HPP

#include "gh_ost/types.hpp"
#include "gh_ost/config.hpp"
#include "mysql/connection.hpp"
#include "mysql/instance_key.hpp"
#include "sql/parser.hpp"
#include "sql/builder.hpp"
#include "binlog/binlog_coordinates.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <mutex>

namespace gh_ost {

// Migration context holding all state for a migration
class MigrationContext {
public:
    MigrationContext();
    ~MigrationContext();
    
    // Configuration
    void SetConfig(const MigrationConfig& config);
    const MigrationConfig& GetConfig() const { return config_; }
    MigrationConfig& GetConfig() { return config_; }
    
    // Connections
    void SetMasterConnection(std::shared_ptr<Connection> conn);
    void SetReplicaConnection(std::shared_ptr<Connection> conn);
    std::shared_ptr<Connection> MasterConnection() const { return master_connection_; }
    std::shared_ptr<Connection> ReplicaConnection() const { return replica_connection_; }
    
    // Instance keys
    void SetMasterKey(const InstanceKey& key);
    void SetReplicaKey(const InstanceKey& key);
    InstanceKey MasterKey() const { return master_key_; }
    InstanceKey ReplicaKey() const { return replica_key_; }
    
    // Database and table names
    std::string DatabaseName() const { return config_.database_name; }
    std::string OriginalTableName() const { return config_.table_name; }
    std::string GhostTableName() const { return ghost_table_name_; }
    std::string ChangelogTableName() const { return changelog_table_name_; }
    
    // ALTER statement
    void SetAlterStatement(const AlterStatement& alter);
    AlterStatement GetAlterStatement() const { return alter_statement_; }
    bool HasAlterStatement() const { return alter_statement_.IsValid(); }
    
    // SQL builder
    SQLBuilder& GetSQLBuilder() { return sql_builder_; }
    
    // Table structures
    void SetOriginalTableStructure(const TableStructure& structure);
    void SetGhostTableStructure(const TableStructure& structure);
    TableStructure OriginalTableStructure() const { return original_structure_; }
    TableStructure GhostTableStructure() const { return ghost_structure_; }
    
    // Unique key for migration
    void SetUniqueKey(const KeyInfo& key);
    KeyInfo GetUniqueKey() const { return unique_key_; }
    std::string GetUniqueKeyName() const { return unique_key_.name; }
    
    // Column pairs (shared columns between original and ghost)
    void SetColumnPairs(const ColumnPairs& pairs);
    ColumnPairs GetColumnPairs() const { return column_pairs_; }
    
    // Row range
    void SetMinKey(const std::string& key);
    void SetMaxKey(const std::string& key);
    std::string MinKey() const { return min_key_; }
    std::string MaxKey() const { return max_key_; }
    
    // Row counts
    void SetTotalRowCount(uint64_t count);
    uint64_t TotalRowCount() const { return total_row_count_; }
    
    std::atomic<uint64_t>& CopiedRowCount() { return copied_row_count_; }
    std::atomic<uint64_t>& AppliedEventCount() { return applied_event_count_; }
    
    // Progress
    MigrationProgress GetProgress() const;
    double GetProgressPercentage() const;
    
    // Migration status
    MigrationStatus GetStatus() const { return status_; }
    void SetStatus(MigrationStatus status);
    
    // Runtime state
    RuntimeConfig& GetRuntimeConfig() { return runtime_config_; }
    
    // Throttle state
    bool IsThrottled() const { return throttled_; }
    void SetThrottled(bool throttled);
    ThrottleReason GetThrottleReason() const { return throttle_reason_; }
    void SetThrottleReason(ThrottleReason reason);
    
    // Cut-over state
    bool IsCutOverPhase() const { return cut_over_phase_; }
    void SetCutOverPhase(bool phase);
    bool HasPostponedCutOver() const { return config_.cut_over.postpone_cut_over; }
    
    // Binlog coordinates
    void SetInitialBinlogCoordinates(const BinlogCoordinates& coords);
    void SetCurrentBinlogCoordinates(const BinlogCoordinates& coords);
    BinlogCoordinates InitialBinlogCoordinates() const { return initial_coordinates_; }
    BinlogCoordinates CurrentBinlogCoordinates() const { return current_coordinates_; }
    
    // Timing
    void SetStartTime();
    TimePoint StartTime() const { return start_time_; }
    Duration ElapsedDuration() const;
    Duration EstimatedRemaining() const;
    
    // Phase tracking
    enum class Phase {
        NotStarted,
        InitialSetup,
        RowCopy,
        AllEventsUpToLockProcessed,
        CutOver,
        Cleanup,
        Completed,
        Failed
    };
    
    Phase GetCurrentPhase() const { return current_phase_; }
    void SetCurrentPhase(Phase phase);
    std::string PhaseToString(Phase phase) const;
    
    // Error handling
    void SetLastError(const std::string& error);
    std::string GetLastError() const { return last_error_; }
    bool HasError() const { return !last_error_.empty(); }
    
    // Heartbeat
    uint64_t HeartbeatInterval() const { return config_.heartbeat_interval_millis; }
    
private:
    MigrationConfig config_;
    RuntimeConfig runtime_config_;
    
    std::shared_ptr<Connection> master_connection_;
    std::shared_ptr<Connection> replica_connection_;
    
    InstanceKey master_key_;
    InstanceKey replica_key_;
    
    std::string ghost_table_name_;
    std::string changelog_table_name_;
    
    AlterStatement alter_statement_;
    SQLBuilder sql_builder_;
    
    TableStructure original_structure_;
    TableStructure ghost_structure_;
    
    KeyInfo unique_key_;
    ColumnPairs column_pairs_;
    
    std::string min_key_;
    std::string max_key_;
    
    uint64_t total_row_count_;
    std::atomic<uint64_t> copied_row_count_;
    std::atomic<uint64_t> applied_event_count_;
    std::atomic<uint64_t> queued_event_count_;
    
    MigrationStatus status_;
    Phase current_phase_;
    
    std::atomic<bool> throttled_;
    ThrottleReason throttle_reason_;
    
    bool cut_over_phase_;
    
    BinlogCoordinates initial_coordinates_;
    BinlogCoordinates current_coordinates_;
    
    TimePoint start_time_;
    
    std::string last_error_;
    
    std::mutex mutex_;
    
    void GenerateGhostTableName();
    void GenerateChangelogTableName();
};

} // namespace gh_ost

#endif // GH_OST_MIGRATION_CONTEXT_HPP