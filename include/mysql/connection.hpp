/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_CONNECTION_HPP
#define GH_OST_CONNECTION_HPP

#include "instance_key.hpp"
#include "gh_ost/types.hpp"
#include "gh_ost/config.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <mutex>

#ifdef HAVE_MYSQL_CPP
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#endif

namespace gh_ost {

// Query result wrapper
struct QueryResult {
    bool success;
    std::string error;
    uint64_t affected_rows;
    uint64_t last_insert_id;
    
    QueryResult() : success(false), affected_rows(0), last_insert_id(0) {}
    QueryResult(bool s) : success(s), affected_rows(0), last_insert_id(0) {}
    
    static QueryResult Success() { return QueryResult(true); }
    static QueryResult Failure(const std::string& err) {
        QueryResult r;
        r.error = err;
        return r;
    }
};

// Row data from query
struct RowData {
    std::vector<std::string> values;
    std::vector<std::string> column_names;
    
    std::optional<std::string> Get(size_t index) const {
        if (index < values.size()) return values[index];
        return std::nullopt;
    }
    
    std::optional<std::string> Get(const std::string& column) const {
        for (size_t i = 0; i < column_names.size(); ++i) {
            if (column_names[i] == column) return values[i];
        }
        return std::nullopt;
    }
};

// Column information
struct ColumnInfo {
    std::string name;
    ColumnType type;
    std::string type_detail;       // Full type definition from MySQL (e.g., "varchar(255)")
    bool nullable;
    std::optional<std::string> default_value;
    bool auto_increment;
    std::string extra;
    
    // For display
    std::string ToString() const;
};

// Key information
struct KeyInfo {
    std::string name;
    UniqueKeyType type;  // PRIMARY, UNIQUE, INDEX
    std::vector<std::string> columns;
    bool is_primary;
    
    std::string ToString() const;
};

// Table structure information
struct TableStructure {
    std::string database;
    std::string name;
    std::vector<ColumnInfo> columns;
    std::vector<KeyInfo> keys;
    
    bool HasPrimaryKey() const;
    bool HasUniqueKey() const;
    std::optional<ColumnInfo> GetColumn(const std::string& name) const;
    std::optional<KeyInfo> GetPrimaryKey() const;
    std::vector<KeyInfo> GetUniqueKeys() const;
};

// MySQL connection wrapper
class Connection {
public:
    Connection();
    ~Connection();
    
    // Connection management
    bool Connect(const ConnectionConfig& config);
    bool Connect(const std::string& host, uint16_t port,
                 const std::string& user, const std::string& password,
                 const std::string& database = "");
    void Disconnect();
    bool IsConnected() const;
    
    // Connection info
    InstanceKey GetInstanceKey() const;
    std::string GetServerVersion() const;
    std::string GetDatabase() const;
    void SetDatabase(const std::string& database);
    
    // Query execution
    QueryResult Execute(const std::string& query);
    QueryResult Execute(const std::string& query, const std::vector<std::string>& params);
    
    // Query with results
    std::vector<RowData> Query(const std::string& query);
    std::optional<RowData> QueryRow(const std::string& query);
    std::optional<std::string> QueryValue(const std::string& query);
    
    // Prepared statement support
    class PreparedStatement {
    public:
        PreparedStatement();
        ~PreparedStatement();
        
        void SetString(size_t index, const std::string& value);
        void SetInt(size_t index, int64_t value);
        void SetDouble(size_t index, double value);
        void SetNull(size_t index);
        
        QueryResult Execute();
        std::vector<RowData> Query();
        
    private:
        friend class Connection;
        PreparedStatement(const std::string& query, Connection* conn);
        std::string query_;
        Connection* connection_;
        std::vector<std::string> params_;
    };
    
    PreparedStatement Prepare(const std::string& query);
    
    // Transaction support
    bool BeginTransaction();
    bool Commit();
    bool Rollback();
    
    // Table operations
    bool CreateTable(const std::string& table_name, const std::string& definition);
    bool DropTable(const std::string& table_name);
    bool DropTableIfExists(const std::string& table_name);
    bool RenameTable(const std::string& old_name, const std::string& new_name);
    bool TruncateTable(const std::string& table_name);
    
    // Table structure queries
    std::optional<TableStructure> GetTableStructure(const std::string& database, 
                                                     const std::string& table);
    std::optional<TableStructure> GetTableStructure(const std::string& table);
    bool TableExists(const std::string& database, const std::string& table);
    bool TableExists(const std::string& table);
    
    // Row operations
    uint64_t GetRowCount(const std::string& table);
    uint64_t GetEstimatedRowCount(const std::string& table);
    std::optional<uint64_t> GetRowCountExact(const std::string& table);
    
    // Binlog operations
    std::string GetCurrentBinlogFile();
    uint64_t GetCurrentBinlogPosition();
    std::string GetCurrentBinlogCoordinates();
    uint64_t GetBinlogSequence(const std::string& binlog_file);
    
    // Server status
    bool IsMaster();
    bool IsReplica();
    uint64_t GetThreadsRunning();
    std::optional<InstanceKey> GetMasterKey();
    std::vector<InstanceKey> GetReplicaKeys();
    
    // Lock operations
    bool LockTablesRead(const std::string& table);
    bool LockTablesWrite(const std::string& table);
    bool UnlockTables();
    
    // Utility
    std::string EscapeString(const std::string& str);
    std::string QuoteIdentifier(const std::string& identifier);
    
    // Connection pool support
    void SetAutoReconnect(bool enable);
    bool Ping();
    
    // Error handling
    std::string GetLastError() const;
    uint64_t GetLastErrorTime() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::mutex mutex_;
    
#ifdef HAVE_MYSQL_CPP
    sql::mysql::MySQL_Driver* driver_;
    std::unique_ptr<sql::Connection> connection_;
#endif
    
    InstanceKey instance_key_;
    std::string database_;
    bool connected_;
};

// Connection pool for managing multiple connections
class ConnectionPool {
public:
    ConnectionPool(size_t max_connections = 10);
    ~ConnectionPool();
    
    bool Initialize(const ConnectionConfig& config);
    void Shutdown();
    
    // Get connection from pool
    std::shared_ptr<Connection> GetConnection();
    void ReturnConnection(std::shared_ptr<Connection> conn);
    
    size_t AvailableConnections() const;
    size_t TotalConnections() const;
    
private:
    ConnectionConfig config_;
    std::vector<std::shared_ptr<Connection>> pool_;
    std::mutex mutex_;
    size_t max_connections_;
};

} // namespace gh_ost

#endif // GH_OST_CONNECTION_HPP