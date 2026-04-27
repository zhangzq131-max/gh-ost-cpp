/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "logic/inspector.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "sql/parser.hpp"

namespace gh_ost {

Inspector::Inspector(MigrationContext& context)
    : context_(context) {
}

Inspector::~Inspector() {}

bool Inspector::Initialize() {
    // Use master connection for inspection
    inspector_connection_ = context_.MasterConnection();
    
    if (!inspector_connection_ || !inspector_connection_->IsConnected()) {
        LOG_ERROR("Inspector connection not available");
        return false;
    }
    
    return true;
}

bool Inspector::DetectMaster() {
    if (!inspector_connection_->IsConnected()) {
        LOG_ERROR("Not connected");
        return false;
    }
    
    // Get master key from current connection
    InstanceKey key(inspector_connection_->GetInstanceKey());
    context_.SetMasterKey(key);
    
    // Check if this is actually a replica
    if (inspector_connection_->IsReplica()) {
        auto master_key = inspector_connection_->GetMasterKey();
        if (master_key) {
            context_.SetMasterKey(*master_key);
            LOG_INFO_FMT("Detected replica, master is at {}", master_key->ToString());
        }
    }
    
    LOG_INFO_FMT("Master identified: {}", context_.MasterKey().ToString());
    return true;
}

bool Inspector::DetectReplica() {
    // If already connected to a replica, use that
    if (context_.ReplicaConnection() && context_.ReplicaConnection()->IsConnected()) {
        context_.SetReplicaKey(context_.ReplicaConnection()->GetInstanceKey());
        return true;
    }
    
    // Try to find a replica from master's process list
    auto replicas = inspector_connection_->GetReplicaKeys();
    if (replicas.empty()) {
        LOG_WARN("No replicas found");
        return false;
    }
    
    // Use first available replica
    context_.SetReplicaKey(replicas[0]);
    LOG_INFO_FMT("Replica identified: {}", context_.ReplicaKey().ToString());
    
    return true;
}

bool Inspector::InspectOriginalTable() {
    std::string database = context_.DatabaseName();
    std::string table = context_.OriginalTableName();
    
    LOG_INFO_FMT("Inspecting original table: {}.{}", database, table);
    
    auto structure = ReadTableStructure(database, table);
    if (!structure.columns.empty()) {
        context_.SetOriginalTableStructure(structure);
        
        LOG_INFO_FMT("Original table has {} columns, {} keys", 
                     structure.columns.size(), structure.keys.size());
        
        return true;
    }
    
    LOG_ERROR_FMT("Failed to read structure for table {}.{}", database, table);
    return false;
}

bool Inspector::InspectGhostTable() {
    std::string database = context_.DatabaseName();
    std::string table = context_.GhostTableName();
    
    LOG_INFO_FMT("Inspecting ghost table: {}.{}", database, table);
    
    auto structure = ReadTableStructure(database, table);
    if (!structure.columns.empty()) {
        context_.SetGhostTableStructure(structure);
        
        LOG_INFO_FMT("Ghost table has {} columns, {} keys", 
                     structure.columns.size(), structure.keys.size());
        
        return true;
    }
    
    LOG_ERROR_FMT("Failed to read structure for table {}.{}", database, table);
    return false;
}

bool Inspector::CompareTables() {
    // Compare original and ghost table structures
    auto original = context_.OriginalTableStructure();
    auto ghost = context_.GhostTableStructure();
    
    // Check shared columns
    for (const auto& orig_col : original.columns) {
        auto ghost_col = ghost.GetColumn(orig_col.name);
        if (ghost_col && ColumnsMatch(orig_col, *ghost_col)) {
            LOG_DEBUG_FMT("Column {} matches", orig_col.name);
        }
    }
    
    return true;
}

bool Inspector::ValidateOriginalTable() {
    auto structure = context_.OriginalTableStructure();
    
    // Check for primary key
    if (!structure.HasPrimaryKey()) {
        LOG_ERROR("Original table must have a primary key");
        return false;
    }
    
    // Check for foreign keys
    if (HasForeignKeys(structure)) {
        LOG_WARN("Original table has foreign keys. Migration may be affected.");
    }
    
    return true;
}

bool Inspector::ValidateGhostTable() {
    auto structure = context_.GhostTableStructure();
    
    if (!structure.HasPrimaryKey()) {
        LOG_ERROR("Ghost table must have a primary key");
        return false;
    }
    
    return true;
}

bool Inspector::DetermineSharedUniqueKey() {
    auto original = context_.OriginalTableStructure();
    auto ghost = context_.GhostTableStructure();
    
    KeyInfo best_key = FindBestUniqueKey(original, ghost);
    
    if (best_key.name.empty()) {
        LOG_ERROR("No suitable shared unique key found");
        return false;
    }
    
    context_.SetUniqueKey(best_key);
    
    LOG_INFO_FMT("Using unique key: {} ({})", best_key.name, 
                 StringUtils::Join(best_key.columns, ", "));
    
    return true;
}

bool Inspector::DetermineColumnPairs() {
    auto alter = context_.GetAlterStatement();
    auto original = context_.OriginalTableStructure();
    
    ColumnPairs pairs;
    
    // Get dropped columns from ALTER
    for (const auto& op : alter.GetDropColumnOps()) {
        pairs.dropped_columns.push_back(op.name);
    }
    
    // Get added columns from ALTER
    for (const auto& op : alter.GetAddColumnOps()) {
        pairs.added_columns.push_back(op.name);
    }
    
    // Build shared columns
    for (const auto& col : original.columns) {
        // Skip dropped columns
        if (std::find(pairs.dropped_columns.begin(), pairs.dropped_columns.end(), 
                      col.name) != pairs.dropped_columns.end()) {
            continue;
        }
        
        // Shared column (same name in both tables)
        pairs.shared_columns.push_back({col.name, col.name});
    }
    
    context_.SetColumnPairs(pairs);
    
    LOG_INFO_FMT("Shared columns: {}, Dropped: {}, Added: {}",
                 pairs.shared_columns.size(),
                 pairs.dropped_columns.size(),
                 pairs.added_columns.size());
    
    return true;
}

bool Inspector::ReadRowRange() {
    auto key = context_.GetUniqueKey();
    if (key.columns.empty()) {
        return false;
    }
    
    std::string key_column = key.columns[0];
    
    // Get min key
    auto min_result = inspector_connection_->QueryValue(
        context_.GetSQLBuilder().BuildMinKeySQL(key_column));
    
    if (min_result) {
        context_.SetMinKey(*min_result);
    } else {
        context_.SetMinKey("0");  // Default
    }
    
    // Get max key
    auto max_result = inspector_connection_->QueryValue(
        context_.GetSQLBuilder().BuildMaxKeySQL(key_column));
    
    if (max_result) {
        context_.SetMaxKey(*max_result);
    } else {
        context_.SetMaxKey("0");  // Default
    }
    
    return true;
}

ValidationResult Inspector::ValidateMigration() {
    ValidationResult result;
    
    // Check ALTER statement
    auto alter = context_.GetAlterStatement();
    if (!alter.IsValid()) {
        for (const auto& error : alter.GetValidationErrors()) {
            result.AddError(error);
        }
    }
    
    // Check for unsupported RENAME
    if (alter.HasRenameTable()) {
        result.AddError("ALTER TABLE RENAME is not supported");
    }
    
    // Check table has unique key
    if (!context_.OriginalTableStructure().HasUniqueKey()) {
        result.AddError("Original table must have a unique key");
    }
    
    // Check ghost table exists
    if (!GhostTableExists()) {
        result.AddError("Ghost table not found");
    }
    
    // Compare structures
    if (!CompareTables()) {
        result.AddWarning("Table structure comparison failed");
    }
    
    return result;
}

bool Inspector::CheckPrivileges() {
    // Check required privileges
    // SELECT, INSERT, UPDATE, DELETE, ALTER, CREATE, DROP, REPLICATION CLIENT
    
    // For now, just check if we can query
    auto result = inspector_connection_->QueryValue("SELECT 1");
    return result && *result == "1";
}

bool Inspector::OriginalTableExists() {
    return inspector_connection_->TableExists(
        context_.DatabaseName(), context_.OriginalTableName());
}

bool Inspector::GhostTableExists() {
    return inspector_connection_->TableExists(
        context_.DatabaseName(), context_.GhostTableName());
}

bool Inspector::ChangelogTableExists() {
    return inspector_connection_->TableExists(
        context_.DatabaseName(), context_.ChangelogTableName());
}

uint64_t Inspector::GetEstimatedRowCount() {
    auto result = inspector_connection_->QueryValue(
        context_.GetSQLBuilder().BuildSelectRowCountSQL(false));
    
    if (result) {
        return StringUtils::ParseUInt64(*result).value_or(0);
    }
    return 0;
}

uint64_t Inspector::GetExactRowCount() {
    auto result = inspector_connection_->QueryValue(
        context_.GetSQLBuilder().BuildSelectRowCountSQL(true));
    
    if (result) {
        return StringUtils::ParseUInt64(*result).value_or(0);
    }
    return 0;
}

TableStructure Inspector::ReadTableStructure(const std::string& database, 
                                             const std::string& table) {
    TableStructure structure;
    structure.database = database;
    structure.name = table;
    
    // Read columns
    auto columns_result = inspector_connection_->Query(
        "SHOW COLUMNS FROM " + inspector_connection_->QuoteIdentifier(database) + 
        "." + inspector_connection_->QuoteIdentifier(table));
    
    for (const auto& row : columns_result) {
        ColumnInfo col;
        col.name = row.Get("Field").value_or("");
        col.type_detail = row.Get("Type").value_or("");
        col.nullable = row.Get("Null").value_or("") == "YES";
        col.default_value = row.Get("Default");
        col.extra = row.Get("Extra").value_or("");
        col.auto_increment = StringUtils::ContainsIgnoreCase(col.extra, "auto_increment");
        
        // Determine type
        std::string type_upper = StringUtils::ToUpper(col.type_detail);
        if (type_upper.find("INT") != std::string::npos) col.type = ColumnType::Integer;
        else if (type_upper.find("VARCHAR") != std::string::npos || 
                 type_upper.find("CHAR") != std::string::npos) col.type = ColumnType::String;
        else if (type_upper.find("TEXT") != std::string::npos) col.type = ColumnType::Text;
        else if (type_upper.find("DATETIME") != std::string::npos) col.type = ColumnType::DateTime;
        else if (type_upper.find("TIMESTAMP") != std::string::npos) col.type = ColumnType::Timestamp;
        else if (type_upper.find("DECIMAL") != std::string::npos) col.type = ColumnType::Decimal;
        else col.type = ColumnType::Unknown;
        
        structure.columns.push_back(col);
    }
    
    // Read keys
    auto keys_result = inspector_connection_->Query(
        "SHOW KEYS FROM " + inspector_connection_->QuoteIdentifier(database) + 
        "." + inspector_connection_->QuoteIdentifier(table));
    
    std::unordered_map<std::string, KeyInfo> key_map;
    
    for (const auto& row : keys_result) {
        std::string key_name = row.Get("Key_name").value_or("");
        std::string column_name = row.Get("Column_name").value_or("");
        
        if (key_map.find(key_name) == key_map.end()) {
            KeyInfo key;
            key.name = key_name;
            key.is_primary = key_name == "PRIMARY";
            
            if (key.is_primary) {
                key.type = UniqueKeyType::Primary;
            } else if (row.Get("Non_unique").value_or("1") == "0") {
                key.type = UniqueKeyType::Unique;
            } else {
                key.type = UniqueKeyType::None;
            }
            
            key_map[key_name] = key;
        }
        
        key_map[key_name].columns.push_back(column_name);
    }
    
    for (const auto& pair : key_map) {
        structure.keys.push_back(pair.second);
    }
    
    return structure;
}

KeyInfo Inspector::FindBestUniqueKey(const TableStructure& original,
                                     const TableStructure& ghost) {
    // Prefer primary key
    auto pk = original.GetPrimaryKey();
    if (pk) {
        // Verify ghost table has same primary key columns
        auto ghost_pk = ghost.GetPrimaryKey();
        if (ghost_pk && ghost_pk->columns == pk->columns) {
            return *pk;
        }
    }
    
    // Then look for unique keys
    auto unique_keys = original.GetUniqueKeys();
    for (const auto& key : unique_keys) {
        if (key.is_primary) continue;
        
        // Verify ghost has this key
        for (const auto& ghost_key : ghost.keys) {
            if (ghost_key.columns == key.columns) {
                return key;
            }
        }
    }
    
    // Return empty if no suitable key found
    return KeyInfo();
}

bool Inspector::ColumnsMatch(const ColumnInfo& col1, const ColumnInfo& col2) {
    // Check basic type compatibility
    return col1.type == col2.type;
}

bool Inspector::HasForeignKeys(const TableStructure& structure) {
    // Query foreign key constraints
    auto result = inspector_connection_->Query(
        "SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS "
        "WHERE TABLE_SCHEMA = '" + structure.database + "' "
        "AND TABLE_NAME = '" + structure.name + "' "
        "AND CONSTRAINT_TYPE = 'FOREIGN KEY'");
    
    if (!result.empty()) {
        auto count = result[0].Get(0);
        if (count) {
            return StringUtils::ParseUInt64(*count).value_or(0) > 0;
        }
    }
    return false;
}

} // namespace gh_ost