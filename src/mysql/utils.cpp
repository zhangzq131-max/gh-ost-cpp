/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "mysql/utils.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include <sstream>

namespace gh_ost {

bool MySQLUtils::IsMaster(Connection& conn) {
    auto row = conn.QueryRow("SHOW SLAVE STATUS");
    return !row;  // No slave status means master or standalone
}

bool MySQLUtils::IsReplica(Connection& conn) {
    auto row = conn.QueryRow("SHOW SLAVE STATUS");
    return row;
}

bool MySQLUtils::IsPercona(Connection& conn) {
    auto version = GetServerVersion(conn);
    return StringUtils::ContainsIgnoreCase(version, "percona");
}

bool MySQLUtils::IsMariaDB(Connection& conn) {
    auto version = GetServerVersion(conn);
    return StringUtils::ContainsIgnoreCase(version, "mariadb");
}

std::string MySQLUtils::GetServerVersion(Connection& conn) {
    return conn.GetServerVersion();
}

std::optional<uint64_t> MySQLUtils::ParseVersion(const std::string& version) {
    // Parse version like "5.7.32" or "10.5.8-MariaDB"
    std::string clean_version = StringUtils::Trim(version);
    
    // Find first non-digit, non-dot character
    size_t end = 0;
    while (end < clean_version.length() && 
           (isdigit(clean_version[end]) || clean_version[end] == '.')) {
        end++;
    }
    
    std::string version_part = clean_version.substr(0, end);
    auto parts = StringUtils::Split(version_part, '.');
    
    if (parts.empty()) return std::nullopt;
    
    // Convert to numeric version (major * 10000 + minor * 100 + patch)
    uint64_t major = StringUtils::ParseUInt64(parts[0]).value_or(0);
    uint64_t minor = parts.size() > 1 ? StringUtils::ParseUInt64(parts[1]).value_or(0) : 0;
    uint64_t patch = parts.size() > 2 ? StringUtils::ParseUInt64(parts[2]).value_or(0) : 0;
    
    return major * 10000 + minor * 100 + patch;
}

bool MySQLUtils::SupportsGTID(Connection& conn) {
    auto val = conn.QueryValue("SELECT @@gtid_mode");
    if (val) {
        return StringUtils::ToLower(*val) == "on";
    }
    return false;
}

bool MySQLUtils::SupportsBinlogChecksum(Connection& conn) {
    auto val = conn.QueryValue("SELECT @@binlog_checksum");
    return val && StringUtils::ToLower(*val) != "none";
}

std::optional<InstanceKey> MySQLUtils::GetMasterKey(Connection& conn) {
    auto row = conn.QueryRow("SHOW SLAVE STATUS");
    if (row) {
        auto host = row->Get("Master_Host");
        auto port = row->Get("Master_Port");
        if (host && port) {
            auto port_num = StringUtils::ParseUInt16(*port);
            if (port_num) {
                return InstanceKey(*host, *port_num);
            }
        }
    }
    return std::nullopt;
}

std::vector<InstanceKey> MySQLUtils::GetReplicaKeys(Connection& conn) {
    std::vector<InstanceKey> replicas;
    
    auto results = conn.Query(
        "SHOW PROCESSLIST WHERE Command = 'Binlog Dump' OR Command = 'Binlog Dump GTID'"
    );
    
    for (const auto& row : results) {
        auto host = row.Get("Host");
        if (host) {
            // Host format is usually "host:port"
            auto key = InstanceKey::Parse(*host);
            if (key) {
                replicas.push_back(*key);
            }
        }
    }
    
    return replicas;
}

uint64_t MySQLUtils::GetReplicationLagSeconds(Connection& conn) {
    auto row = conn.QueryRow("SHOW SLAVE STATUS");
    if (row) {
        auto lag = row->Get("Seconds_Behind_Master");
        if (lag) {
            return StringUtils::ParseUInt64(*lag).value_or(0);
        }
    }
    return 0;
}

std::string MySQLUtils::GetCurrentBinlogFile(Connection& conn) {
    auto val = conn.QueryValue("SHOW MASTER STATUS");
    auto row = conn.QueryRow("SHOW MASTER STATUS");
    if (row) {
        return row->Get("File").value_or("");
    }
    return "";
}

uint64_t MySQLUtils::GetCurrentBinlogPosition(Connection& conn) {
    auto row = conn.QueryRow("SHOW MASTER STATUS");
    if (row) {
        auto pos = row->Get("Position");
        if (pos) {
            return StringUtils::ParseUInt64(*pos).value_or(0);
        }
    }
    return 0;
}

std::string MySQLUtils::GetCurrentBinlogCoordinates(Connection& conn) {
    std::ostringstream oss;
    oss << GetCurrentBinlogFile(conn) << ":" << GetCurrentBinlogPosition(conn);
    return oss.str();
}

std::optional<uint64_t> MySQLUtils::ParseBinlogSequence(const std::string& binlog_file) {
    // Extract sequence from binlog file name like "mysql-bin.000123"
    size_t last_dot = binlog_file.rfind('.');
    if (last_dot != std::string::npos && last_dot + 1 < binlog_file.length()) {
        std::string seq_str = binlog_file.substr(last_dot + 1);
        return StringUtils::ParseUInt64(seq_str);
    }
    return std::nullopt;
}

bool MySQLUtils::HasPrimaryKey(Connection& conn, const std::string& database,
                               const std::string& table) {
    auto row = conn.QueryRow(
        "SELECT COUNT(*) FROM information_schema.table_constraints "
        "WHERE table_schema = '" + conn.EscapeString(database) + "' "
        "AND table_name = '" + conn.EscapeString(table) + "' "
        "AND constraint_type = 'PRIMARY KEY'"
    );
    
    if (row) {
        auto count = row->Get(0);
        if (count) {
            return StringUtils::ParseUInt64(*count).value_or(0) > 0;
        }
    }
    return false;
}

bool MySQLUtils::HasUniqueKey(Connection& conn, const std::string& database,
                              const std::string& table) {
    auto row = conn.QueryRow(
        "SELECT COUNT(*) FROM information_schema.table_constraints "
        "WHERE table_schema = '" + conn.EscapeString(database) + "' "
        "AND table_name = '" + conn.EscapeString(table) + "' "
        "AND (constraint_type = 'PRIMARY KEY' OR constraint_type = 'UNIQUE')"
    );
    
    if (row) {
        auto count = row->Get(0);
        if (count) {
            return StringUtils::ParseUInt64(*count).value_or(0) > 0;
        }
    }
    return false;
}

std::vector<std::string> MySQLUtils::GetTableColumns(Connection& conn,
                                                     const std::string& database,
                                                     const std::string& table) {
    std::vector<std::string> columns;
    
    auto results = conn.Query(
        "SELECT COLUMN_NAME FROM information_schema.columns "
        "WHERE table_schema = '" + conn.EscapeString(database) + "' "
        "AND table_name = '" + conn.EscapeString(table) + "' "
        "ORDER BY ordinal_position"
    );
    
    for (const auto& row : results) {
        auto col = row.Get(0);
        if (col) {
            columns.push_back(*col);
        }
    }
    
    return columns;
}

bool MySQLUtils::GetLock(Connection& conn, const std::string& lock_name,
                         uint64_t timeout_seconds) {
    auto val = conn.QueryValue(
        "SELECT GET_LOCK('" + conn.EscapeString(lock_name) + "', " +
        std::to_string(timeout_seconds) + ")"
    );
    
    return val && *val == "1";
}

bool MySQLUtils::ReleaseLock(Connection& conn, const std::string& lock_name) {
    auto val = conn.QueryValue(
        "SELECT RELEASE_LOCK('" + conn.EscapeString(lock_name) + "')"
    );
    
    return val && *val == "1";
}

bool MySQLUtils::IsLockUsed(Connection& conn, const std::string& lock_name) {
    auto val = conn.QueryValue(
        "SELECT IS_USED_LOCK('" + conn.EscapeString(lock_name) + "')"
    );
    
    return val && *val != "NULL" && *val != "0";
}

uint64_t MySQLUtils::GetThreadsRunning(Connection& conn) {
    return conn.GetThreadsRunning();
}

uint64_t MySQLUtils::GetMaxConnections(Connection& conn) {
    auto val = conn.QueryValue("SELECT @@max_connections");
    if (val) {
        return StringUtils::ParseUInt64(*val).value_or(0);
    }
    return 0;
}

uint64_t MySQLUtils::GetCurrentConnections(Connection& conn) {
    auto val = conn.QueryValue(
        "SELECT COUNT(*) FROM information_schema.processlist"
    );
    if (val) {
        return StringUtils::ParseUInt64(*val).value_or(0);
    }
    return 0;
}

std::optional<uint64_t> MySQLUtils::TableChecksum(Connection& conn,
                                                  const std::string& database,
                                                  const std::string& table) {
    auto val = conn.QueryValue(
        "CHECKSUM TABLE " + conn.QuoteIdentifier(database) + "." + conn.QuoteIdentifier(table)
    );
    
    if (val) {
        return StringUtils::ParseUInt64(*val);
    }
    return std::nullopt;
}

std::string MySQLUtils::BuildSelectAllSQL(const std::string& database,
                                          const std::string& table,
                                          const std::string& key_column,
                                          const std::string& min_value,
                                          const std::string& max_value,
                                          uint64_t limit) {
    std::ostringstream oss;
    oss << "SELECT * FROM " << conn.QuoteIdentifier(database) << "." << conn.QuoteIdentifier(table);
    
    if (!min_value.empty() && !max_value.empty()) {
        oss << " WHERE " << conn.QuoteIdentifier(key_column)
            << " > " << FormatValueForSQL(min_value)
            << " AND " << conn.QuoteIdentifier(key_column)
            << " <= " << FormatValueForSQL(max_value);
    }
    
    oss << " ORDER BY " << conn.QuoteIdentifier(key_column);
    oss << " LIMIT " << limit;
    
    return oss.str();
}

std::string MySQLUtils::BuildInsertSQL(const std::string& database,
                                       const std::string& table,
                                       const std::vector<std::string>& columns,
                                       const std::vector<std::string>& values) {
    std::ostringstream oss;
    oss << "INSERT INTO " << conn.QuoteIdentifier(database) << "." << conn.QuoteIdentifier(table);
    
    oss << " (" << StringUtils::Join(columns, ", ") << ")";
    oss << " VALUES (" << StringUtils::Join(values, ", ") << ")";
    
    return oss.str();
}

std::string MySQLUtils::BuildUpdateSQL(const std::string& database,
                                       const std::string& table,
                                       const std::string& key_column,
                                       const std::string& key_value,
                                       const std::vector<std::pair<std::string, std::string>>& updates) {
    std::ostringstream oss;
    oss << "UPDATE " << conn.QuoteIdentifier(database) << "." << conn.QuoteIdentifier(table);
    
    oss << " SET ";
    for (size_t i = 0; i < updates.size(); ++i) {
        oss << conn.QuoteIdentifier(updates[i].first) << " = " << updates[i].second;
        if (i < updates.size() - 1) oss << ", ";
    }
    
    oss << " WHERE " << conn.QuoteIdentifier(key_column) << " = " << FormatValueForSQL(key_value);
    
    return oss.str();
}

std::string MySQLUtils::BuildDeleteSQL(const std::string& database,
                                       const std::string& table,
                                       const std::string& key_column,
                                       const std::string& key_value) {
    std::ostringstream oss;
    oss << "DELETE FROM " << conn.QuoteIdentifier(database) << "." << conn.QuoteIdentifier(table);
    oss << " WHERE " << conn.QuoteIdentifier(key_column) << " = " << FormatValueForSQL(key_value);
    
    return oss.str();
}

std::string MySQLUtils::FormatValueForSQL(const std::string& value) {
    return "'" + StringUtils::EscapeForSQL(value) + "'";
}

std::string MySQLUtils::FormatValueForSQL(int64_t value) {
    return std::to_string(value);
}

std::string MySQLUtils::FormatValueForSQL(double value) {
    return std::to_string(value);
}

std::string MySQLUtils::FormatValueForSQLNull() {
    return "NULL";
}

} // namespace gh_ost