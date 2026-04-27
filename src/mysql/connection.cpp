/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "mysql/connection.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include <sstream>
#include <algorithm>

#ifdef HAVE_MYSQL_CPP
// Use MySQL Connector/C++
#else
// Fallback implementation without MySQL library
#endif

namespace gh_ost {

// TableStructure methods
bool TableStructure::HasPrimaryKey() const {
    for (const auto& key : keys) {
        if (key.is_primary) return true;
    }
    return false;
}

bool TableStructure::HasUniqueKey() const {
    for (const auto& key : keys) {
        if (key.type == UniqueKeyType::Unique) return true;
    }
    return false;
}

std::optional<ColumnInfo> TableStructure::GetColumn(const std::string& name) const {
    for (const auto& col : columns) {
        if (StringUtils::ToLower(col.name) == StringUtils::ToLower(name)) {
            return col;
        }
    }
    return std::nullopt;
}

std::optional<KeyInfo> TableStructure::GetPrimaryKey() const {
    for (const auto& key : keys) {
        if (key.is_primary) return key;
    }
    return std::nullopt;
}

std::vector<KeyInfo> TableStructure::GetUniqueKeys() const {
    std::vector<KeyInfo> unique_keys;
    for (const auto& key : keys) {
        if (key.is_primary || key.type == UniqueKeyType::Unique) {
            unique_keys.push_back(key);
        }
    }
    return unique_keys;
}

// ColumnInfo methods
std::string ColumnInfo::ToString() const {
    std::ostringstream oss;
    oss << name;
    // Add type string
    switch (type) {
        case ColumnType::Integer: oss << " INT"; break;
        case ColumnType::Float: oss << " FLOAT"; break;
        case ColumnType::Double: oss << " DOUBLE"; break;
        case ColumnType::Decimal: oss << " DECIMAL"; break;
        case ColumnType::String: oss << " VARCHAR"; break;
        case ColumnType::Text: oss << " TEXT"; break;
        case ColumnType::Blob: oss << " BLOB"; break;
        case ColumnType::DateTime: oss << " DATETIME"; break;
        case ColumnType::Date: oss << " DATE"; break;
        case ColumnType::Time: oss << " TIME"; break;
        case ColumnType::Timestamp: oss << " TIMESTAMP"; break;
        case ColumnType::Year: oss << " YEAR"; break;
        case ColumnType::Bool: oss << " BOOL"; break;
        case ColumnType::Enum: oss << " ENUM"; break;
        case ColumnType::Set: oss << " SET"; break;
        case ColumnType::Json: oss << " JSON"; break;
        default: oss << " UNKNOWN"; break;
    }
    if (!nullable) oss << " NOT NULL";
    if (auto_increment) oss << " AUTO_INCREMENT";
    return oss.str();
}

// KeyInfo methods
std::string KeyInfo::ToString() const {
    std::ostringstream oss;
    if (is_primary) {
        oss << "PRIMARY KEY";
    } else if (type == UniqueKeyType::Unique) {
        oss << "UNIQUE KEY " << name;
    } else {
        oss << "INDEX " << name;
    }
    oss << " (" << StringUtils::Join(columns, ", ") << ")";
    return oss.str();
}

// Connection implementation
class Connection::Impl {
public:
    Impl() : connected(false) {}
    
    bool connected;
    std::string last_error;
    uint64_t last_error_time;
    std::string server_version;
    bool in_transaction;
    
#ifdef HAVE_MYSQL_CPP
    std::unique_ptr<sql::Connection> mysql_conn;
#endif
};

Connection::Connection()
    : impl_(std::make_unique<Impl>())
#ifdef HAVE_MYSQL_CPP
    , driver_(nullptr)
#endif
    , connected_(false) {
#ifdef HAVE_MYSQL_CPP
    try {
        driver_ = sql::mysql::get_mysql_driver_instance();
    } catch (sql::SQLException& e) {
        LOG_ERROR_FMT("Failed to get MySQL driver: {}", e.what());
    }
#endif
}

Connection::~Connection() {
    Disconnect();
}

bool Connection::Connect(const ConnectionConfig& config) {
    return Connect(config.host, config.port, config.user, config.password, config.database);
}

bool Connection::Connect(const std::string& host, uint16_t port,
                         const std::string& user, const std::string& password,
                         const std::string& database) {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef HAVE_MYSQL_CPP
    if (!driver_) {
        impl_->last_error = "MySQL driver not available";
        return false;
    }
    
    try {
        std::string url = host + ":" + std::to_string(port);
        
        sql::ConnectOptionsMap options;
        options["hostName"] = url;
        options["userName"] = user;
        options["password"] = password;
        
        if (!database.empty()) {
            options["schema"] = database;
            database_ = database;
        }
        
        // Connection timeouts
        options["OPT_CONNECT_TIMEOUT"] = 10;
        options["OPT_READ_TIMEOUT"] = 30;
        options["OPT_WRITE_TIMEOUT"] = 30;
        
        // SSL options (if enabled)
        // options["OPT_SSL_CA"] = ...
        
        connection_ = driver_->connect(options);
        connected_ = connection_->isValid();
        impl_->connected = connected_;
        
        instance_key_ = InstanceKey(host, port);
        
        // Get server version
        std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT VERSION()"));
        if (res->next()) {
            impl_->server_version = res->getString(1);
        }
        
        LOG_INFO_FMT("Connected to MySQL server {} at {}:{}", 
                     impl_->server_version, host, port);
        return true;
        
    } catch (sql::SQLException& e) {
        impl_->last_error = e.what();
        impl_->last_error_time = TimeUtils::NowUnix();
        LOG_ERROR_FMT("Connection failed: {}", e.what());
        return false;
    }
#else
    impl_->last_error = "MySQL Connector/C++ not available";
    LOG_ERROR("MySQL library not linked");
    return false;
#endif
}

void Connection::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef HAVE_MYSQL_CPP
    if (connection_) {
        connection_->close();
        connection_.reset();
    }
#endif
    connected_ = false;
    impl_->connected = false;
}

bool Connection::IsConnected() const {
    return connected_ && impl_->connected;
}

InstanceKey Connection::GetInstanceKey() const {
    return instance_key_;
}

std::string Connection::GetServerVersion() const {
    return impl_->server_version;
}

std::string Connection::GetDatabase() const {
    return database_;
}

void Connection::SetDatabase(const std::string& database) {
#ifdef HAVE_MYSQL_CPP
    if (connection_) {
        try {
            connection_->setSchema(database);
            database_ = database;
        } catch (sql::SQLException& e) {
            impl_->last_error = e.what();
            LOG_ERROR_FMT("Failed to set schema: {}", e.what());
        }
    }
#else
    database_ = database;
#endif
}

QueryResult Connection::Execute(const std::string& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!IsConnected()) {
        return QueryResult::Failure("Not connected");
    }
    
#ifdef HAVE_MYSQL_CPP
    try {
        std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
        stmt->executeUpdate(query);
        QueryResult result(QueryResult::Success());
        result.affected_rows = stmt->getUpdateCount();
        return result;
        
    } catch (sql::SQLException& e) {
        impl_->last_error = e.what();
        impl_->last_error_time = TimeUtils::NowUnix();
        LOG_ERROR_FMT("Execute failed: {} - Query: {}", e.what(), query);
        return QueryResult::Failure(e.what());
    }
#else
    return QueryResult::Failure("MySQL library not available");
#endif
}

std::vector<RowData> Connection::Query(const std::string& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RowData> results;
    
    if (!IsConnected()) {
        return results;
    }
    
#ifdef HAVE_MYSQL_CPP
    try {
        std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));
        
        sql::ResultSetMetaData* meta = res->getMetaData();
        int num_columns = meta->getColumnCount();
        
        while (res->next()) {
            RowData row;
            for (int i = 1; i <= num_columns; ++i) {
                row.column_names.push_back(meta->getColumnName(i));
                row.values.push_back(res->getString(i));
            }
            results.push_back(row);
        }
        
    } catch (sql::SQLException& e) {
        impl_->last_error = e.what();
        impl_->last_error_time = TimeUtils::NowUnix();
        LOG_ERROR_FMT("Query failed: {} - Query: {}", e.what(), query);
    }
#endif
    return results;
}

std::optional<RowData> Connection::QueryRow(const std::string& query) {
    auto results = Query(query);
    if (results.empty()) return std::nullopt;
    return results[0];
}

std::optional<std::string> Connection::QueryValue(const std::string& query) {
    auto row = QueryRow(query);
    if (!row) return std::nullopt;
    return row->Get(0);
}

bool Connection::BeginTransaction() {
#ifdef HAVE_MYSQL_CPP
    if (connection_) {
        try {
            connection_->setAutoCommit(false);
            impl_->in_transaction = true;
            return true;
        } catch (sql::SQLException& e) {
            impl_->last_error = e.what();
            return false;
        }
    }
#endif
    return false;
}

bool Connection::Commit() {
#ifdef HAVE_MYSQL_CPP
    if (connection_ && impl_->in_transaction) {
        try {
            connection_->commit();
            connection_->setAutoCommit(true);
            impl_->in_transaction = false;
            return true;
        } catch (sql::SQLException& e) {
            impl_->last_error = e.what();
            return false;
        }
    }
#endif
    return false;
}

bool Connection::Rollback() {
#ifdef HAVE_MYSQL_CPP
    if (connection_ && impl_->in_transaction) {
        try {
            connection_->rollback();
            connection_->setAutoCommit(true);
            impl_->in_transaction = false;
            return true;
        } catch (sql::SQLException& e) {
            impl_->last_error = e.what();
            return false;
        }
    }
#endif
    return false;
}

std::string Connection::GetCurrentBinlogFile() {
    return QueryValue("SHOW MASTER STATUS LIKE 'File'").value_or("");
}

uint64_t Connection::GetCurrentBinlogPosition() {
    auto pos = QueryValue("SHOW MASTER STATUS LIKE 'Position'");
    if (pos) {
        return StringUtils::ParseUInt64(*pos).value_or(0);
    }
    return 0;
}

std::string Connection::GetCurrentBinlogCoordinates() {
    std::ostringstream oss;
    oss << GetCurrentBinlogFile() << ":" << GetCurrentBinlogPosition();
    return oss.str();
}

uint64_t Connection::GetBinlogSequence(const std::string& binlog_file) {
    // Extract sequence from binlog file name like "mysql-bin.000123"
    size_t last_dot = binlog_file.rfind('.');
    if (last_dot != std::string::npos && last_dot + 1 < binlog_file.length()) {
        std::string seq_str = binlog_file.substr(last_dot + 1);
        auto seq = StringUtils::ParseUInt64(seq_str);
        if (seq) return *seq;
    }
    return 0;
}

bool Connection::IsMaster() {
    auto row = QueryRow("SHOW SLAVE STATUS");
    // If no slave status, this is a master (or standalone)
    return !row;
}

bool Connection::IsReplica() {
    auto row = QueryRow("SHOW SLAVE STATUS");
    return row;
}

uint64_t Connection::GetThreadsRunning() {
    auto val = QueryValue("SELECT COUNT(*) FROM information_schema.processlist WHERE COMMAND != 'Sleep'");
    if (val) {
        return StringUtils::ParseUInt64(*val).value_or(0);
    }
    return 0;
}

std::optional<InstanceKey> Connection::GetMasterKey() {
    auto row = QueryRow("SHOW SLAVE STATUS");
    if (row) {
        auto host = row->Get("Master_Host");
        auto port = row->Get("Master_Port");
        if (host && port) {
            return InstanceKey(*host, StringUtils::ParseUInt16(*port).value_or(3306));
        }
    }
    return std::nullopt;
}

std::string Connection::EscapeString(const std::string& str) {
    return StringUtils::EscapeForSQL(str);
}

std::string Connection::QuoteIdentifier(const std::string& identifier) {
    return StringUtils::QuoteIdentifier(identifier);
}

bool Connection::Ping() {
#ifdef HAVE_MYSQL_CPP
    if (connection_) {
        try {
            // Execute a simple query to check connection
            auto val = QueryValue("SELECT 1");
            return val && *val == "1";
        } catch (...) {
            return false;
        }
    }
#endif
    return false;
}

std::string Connection::GetLastError() const {
    return impl_->last_error;
}

uint64_t Connection::GetLastErrorTime() const {
    return impl_->last_error_time;
}

} // namespace gh_ost