/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_CONFIG_HPP
#define GH_OST_CONFIG_HPP

#include "types.hpp"
#include <string>
#include <optional>
#include <cstdint>
#include <vector>

namespace gh_ost {

// Forward declarations
struct ConnectionConfig;
struct ThrottleConfig;
struct CutOverConfig;
struct HooksConfig;

// MySQL connection configuration
struct ConnectionConfig {
    std::string host;
    uint16_t port;
    std::string user;
    std::string password;
    std::string database;
    
    // Connection options
    uint32_t connect_timeout_seconds = 10;
    uint32_t read_timeout_seconds = 30;
    uint32_t write_timeout_seconds = 30;
    
    // SSL options
    bool use_ssl = false;
    std::optional<std::string> ssl_ca_file;
    std::optional<std::string> ssl_cert_file;
    std::optional<std::string> ssl_key_file;
    
    ConnectionConfig() : port(3306) {}
    
    ConnectionConfig(const std::string& h, uint16_t p, 
                     const std::string& u, const std::string& pwd,
                     const std::string& db)
        : host(h), port(p), user(u), password(pwd), database(db) {}
    
    std::string ToString() const {
        return host + ":" + std::to_string(port);
    }
};

// Throttle configuration
struct ThrottleConfig {
    // Replication lag threshold (seconds)
    uint64_t max_replication_lag = 60;
    
    // Load thresholds
    std::optional<uint64_t> max_load_threads_running;
    std::optional<uint64_t> critical_load_threads_running;
    
    // Query based throttling
    std::optional<std::string> throttle_query;
    std::optional<std::string> critical_query;
    
    // Chunk size control
    uint64_t chunk_size = 1000;
    uint64_t max_lag_millis = 1500;
    
    // Self throttling
    uint64_t throttle_interval_millis = 100;
    
    ThrottleConfig() {}
};

// Cut-over configuration
struct CutOverConfig {
    // Cut-over strategy
    enum class Strategy {
        AtomicSwap,     // Atomic rename tables
        LockTables      // Two-phase with table locking
    };
    
    Strategy strategy = Strategy::AtomicSwap;
    
    // Timing
    uint64_t cut_over_lock_timeout_seconds = 3;
    uint64_t cut_over_exponential_backoff_max_wait_seconds = 64;
    
    // Postpone cut-over
    bool postpone_cut_over = false;
    std::optional<std::string> postpone_flag_file;
    
    // Force cut-over
    std::optional<std::string> force_cut_over_flag_file;
    
    // Graceful cut-over
    bool graceful_cut_over = true;
    uint64_t graceful_cut_over_max_wait_seconds = 60;
    
    CutOverConfig() {}
};

// Hooks configuration
struct HooksConfig {
    // Hook file paths
    std::optional<std::string> hook_before_migration;
    std::optional<std::string> hook_after_migration;
    std::optional<std::string> hook_before_cut_over;
    std::optional<std::string> hook_after_cut_over;
    std::optional<std::string> hook_on_status;
    std::optional<std::string> hook_on_throttle;
    std::optional<std::string> hook_on_unthrottle;
    std::optional<std::string> hook_on_failure;
    std::optional<std::string> hook_on_success;
    
    // Hook execution
    uint64_t hook_timeout_seconds = 30;
    bool hook_fail_on_error = false;
    
    HooksConfig() {}
};

// Main migration configuration
struct MigrationConfig {
    // Database connections
    ConnectionConfig master_connection;
    ConnectionConfig replica_connection;
    
    // Table names
    std::string database_name;
    std::string table_name;
    std::string ghost_table_name;       // Auto-generated if empty
    std::string changelog_table_name;   // Auto-generated if empty
    
    // ALTER statement
    std::string alter_statement;
    
    // Migration mode
    MigrationMode mode = MigrationMode::OnReplica;
    
    // Throttle settings
    ThrottleConfig throttle;
    
    // Cut-over settings
    CutOverConfig cut_over;
    
    // Hooks settings
    HooksConfig hooks;
    
    // Execution flags
    bool execute = false;               // Execute migration
    bool test_on_replica = false;       // Test mode on replica
    bool dry_run = false;               // Dry run, no actual changes
    bool allow_on_master = false;       // Allow running directly on master
    
    // Table management
    bool drop_original_table = true;    // Drop original after migration
    bool drop_ghost_table = true;       // Drop ghost table on failure
    
    // Progress tracking
    bool exact_rowcount = false;        // Get exact row count
    bool progress_percentage = true;    // Show progress percentage
    
    // Output
    std::optional<std::string> status_socket_path;  // Unix socket for status
    std::optional<uint16_t> status_tcp_port;        // TCP port for status
    
    // Logging
    bool verbose = false;
    bool quiet = false;
    std::optional<std::string> log_file;
    
    // Default values
    uint64_t heartbeat_interval_millis = 100;
    uint64_t status_update_interval_millis = 500;
    
    MigrationConfig() {}
    
    // Validate configuration
    ValidationResult Validate() const {
        ValidationResult result;
        
        // Required fields
        if (database_name.empty()) {
            result.AddError("Database name is required");
        }
        if (table_name.empty()) {
            result.AddError("Table name is required");
        }
        if (alter_statement.empty()) {
            result.AddError("ALTER statement is required");
        }
        
        // Connection validation
        if (master_connection.host.empty() && replica_connection.host.empty()) {
            result.AddError("At least one connection (master or replica) is required");
        }
        
        // Mode-specific validation
        if (mode == MigrationMode::OnMaster && !allow_on_master) {
            result.AddWarning("Running on master requires --allow-on-master flag");
        }
        
        // Ghost table name generation
        if (ghost_table_name.empty()) {
            ghost_table_name = "_gh_ost_" + table_name;
        }
        
        return result;
    }
};

// Runtime configuration (can be changed during migration)
struct RuntimeConfig {
    bool paused = false;
    bool throttled = false;
    ThrottleReason throttle_reason = ThrottleReason::None;
    uint64_t current_chunk_size;
    
    RuntimeConfig() : current_chunk_size(1000) {}
    
    void UpdateFrom(const ThrottleConfig& throttle_config) {
        current_chunk_size = throttle_config.chunk_size;
    }
};

} // namespace gh_ost

#endif // GH_OST_CONFIG_HPP