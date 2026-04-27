/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_MYSQL_UTILS_HPP
#define GH_OST_MYSQL_UTILS_HPP

#include "connection.hpp"
#include "instance_key.hpp"
#include "gh_ost/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace gh_ost {

class MySQLUtils {
public:
    // Server type detection
    static bool IsMaster(Connection& conn);
    static bool IsReplica(Connection& conn);
    static bool IsPercona(Connection& conn);
    static bool IsMariaDB(Connection& conn);
    
    // Server version parsing
    static std::string GetServerVersion(Connection& conn);
    static std::optional<uint64_t> ParseVersion(const std::string& version);
    static bool SupportsGTID(Connection& conn);
    static bool SupportsBinlogChecksum(Connection& conn);
    
    // Master/Replica topology
    static std::optional<InstanceKey> GetMasterKey(Connection& conn);
    static std::vector<InstanceKey> GetReplicaKeys(Connection& conn);
    static uint64_t GetReplicationLagSeconds(Connection& conn);
    
    // Binlog utilities
    static std::string GetCurrentBinlogFile(Connection& conn);
    static uint64_t GetCurrentBinlogPosition(Connection& conn);
    static std::string GetCurrentBinlogCoordinates(Connection& conn);
    static std::optional<uint64_t> ParseBinlogSequence(const std::string& binlog_file);
    
    // Table utilities
    static bool HasPrimaryKey(Connection& conn, const std::string& database, 
                              const std::string& table);
    static bool HasUniqueKey(Connection& conn, const std::string& database,
                             const std::string& table);
    static std::vector<std::string> GetTableColumns(Connection& conn,
                                                    const std::string& database,
                                                    const std::string& table);
    
    // Lock utilities
    static bool GetLock(Connection& conn, const std::string& lock_name,
                        uint64_t timeout_seconds = 0);
    static bool ReleaseLock(Connection& conn, const std::string& lock_name);
    static bool IsLockUsed(Connection& conn, const std::string& lock_name);
    
    // Throttle utilities
    static uint64_t GetThreadsRunning(Connection& conn);
    static uint64_t GetMaxConnections(Connection& conn);
    static uint64_t GetCurrentConnections(Connection& conn);
    
    // Chunk generation
    static std::vector<std::pair<std::string, std::string>> 
        GenerateChunkBoundaries(Connection& conn, const std::string& table,
                                const std::string& key_column, uint64_t chunk_size);
    
    // Checksum utilities
    static std::optional<uint64_t> TableChecksum(Connection& conn,
                                                 const std::string& database,
                                                 const std::string& table);
    
    // SQL generation helpers
    static std::string BuildSelectAllSQL(const std::string& database,
                                         const std::string& table,
                                         const std::string& key_column,
                                         const std::string& min_value,
                                         const std::string& max_value,
                                         uint64_t limit);
    
    static std::string BuildInsertSQL(const std::string& database,
                                      const std::string& table,
                                      const std::vector<std::string>& columns,
                                      const std::vector<std::string>& values);
    
    static std::string BuildUpdateSQL(const std::string& database,
                                      const std::string& table,
                                      const std::string& key_column,
                                      const std::string& key_value,
                                      const std::vector<std::pair<std::string, std::string>>& updates);
    
    static std::string BuildDeleteSQL(const std::string& database,
                                      const std::string& table,
                                      const std::string& key_column,
                                      const std::string& key_value);
    
    // Value formatting
    static std::string FormatValueForSQL(const std::string& value);
    static std::string FormatValueForSQL(int64_t value);
    static std::string FormatValueForSQL(double value);
    static std::string FormatValueForSQLNull();
    
    // Connection string parsing
    static std::optional<ConnectionConfig> ParseConnectionString(const std::string& conn_str);
    static std::string BuildConnectionString(const ConnectionConfig& config);
    
    // Permission checks
    static bool HasReplicationPrivilege(Connection& conn);
    static bool HasSelectPrivilege(Connection& conn, const std::string& database,
                                   const std::string& table);
    static bool HasInsertPrivilege(Connection& conn, const std::string& database,
                                   const std::string& table);
    static bool HasUpdatePrivilege(Connection& conn, const std::string& database,
                                   const std::string& table);
    static bool HasDeletePrivilege(Connection& conn, const std::string& database,
                                   const std::string& table);
    static bool HasAlterPrivilege(Connection& conn, const std::string& database);
    static bool HasCreatePrivilege(Connection& conn, const std::string& database);
    static bool HasDropPrivilege(Connection& conn, const std::string& database);
    
private:
    MySQLUtils() = delete;
};

} // namespace gh_ost

#endif // GH_OST_MYSQL_UTILS_HPP