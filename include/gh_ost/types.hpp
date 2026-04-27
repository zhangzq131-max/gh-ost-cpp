/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_TYPES_HPP
#define GH_OST_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <functional>

namespace gh_ost {

// Common type aliases
using String = std::string;
using StringView = std::string_view;
using StringList = std::vector<std::string>;

// Smart pointer aliases
template<typename T>
using Ptr = std::shared_ptr<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Time types
using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::duration<double>;

// Function types
template<typename... Args>
using Callback = std::function<void(Args...)>;

using ErrorCallback = std::function<void(const std::string& error)>;
using SuccessCallback = std::function<void()>;

// Result types
template<typename T>
using Result = std::optional<T>;

// Error type for operations
struct Error {
    int code;
    std::string message;
    
    Error(int c, const std::string& msg) : code(c), message(msg) {}
    Error(const std::string& msg) : code(-1), message(msg) {}
    
    bool Ok() const { return code == 0; }
    bool Failed() const { return code != 0; }
    
    static Error OK() { return Error(0, ""); }
    static Error Generic(const std::string& msg) { return Error(-1, msg); }
};

// Migration status enum
enum class MigrationStatus {
    NotStarted,
    Running,
    Paused,
    Throttled,
    CutOverPhase,
    Completed,
    Failed,
    Cancelled
};

// Migration mode enum
enum class MigrationMode {
    Noop,               // Test mode, no actual migration
    OnMaster,           // Run directly on master
    OnReplica,          // Run on replica, master untouched
    TestOnReplica       // Test on replica, compare tables
};

// Table types
enum class TableType {
    Original,           // The original table being migrated
    Ghost,              // The ghost (shadow) table
    Changelog           // The changelog table for tracking
};

// DML operation types
enum class DMLType {
    Insert,
    Update,
    Delete
};

// Binlog event types
enum class BinlogEventType {
    Unknown,
    WriteRows,          // INSERT
    UpdateRows,         // UPDATE
    DeleteRows,         // DELETE
    Query,              // Query event (DDL)
    Rotate,             // Rotate binlog file
    FormatDescription   // Format description
};

// Throttle reason enum
enum class ThrottleReason {
    None,
    ReplicationLag,
    MaxLoad,
    CriticalLoad,
    UserCommand,
    Heartbeat,
    PostponedCutOver
};

// Column type enum
enum class ColumnType {
    Unknown,
    Integer,
    Float,
    Double,
    Decimal,
    String,
    Text,
    Blob,
    DateTime,
    Date,
    Time,
    Timestamp,
    Year,
    Bool,
    Enum,
    Set,
    Json
};

// Unique key type
enum class UniqueKeyType {
    Primary,
    Unique,
    None
};

// Safe point for exception handling
struct SafePoint {
    bool success;
    std::string error_message;
    
    SafePoint() : success(true), error_message("") {}
    SafePoint(const std::string& err) : success(false), error_message(err) {}
};

// Progress tracking
struct MigrationProgress {
    uint64_t total_rows;
    uint64_t copied_rows;
    uint64_t remaining_rows;
    uint64_t events_applied;
    uint64_t events_queued;
    double percentage_complete;
    Duration estimated_remaining;
    
    MigrationProgress() 
        : total_rows(0)
        , copied_rows(0)
        , remaining_rows(0)
        , events_applied(0)
        , events_queued(0)
        , percentage_complete(0.0)
        , estimated_remaining(Duration(0)) {}
};

// Configuration validation result
struct ValidationResult {
    bool valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    ValidationResult() : valid(true) {}
    
    void AddError(const std::string& error) {
        errors.push_back(error);
        valid = false;
    }
    
    void AddWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

} // namespace gh_ost

#endif // GH_OST_TYPES_HPP