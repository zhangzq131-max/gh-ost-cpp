/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "logic/applier.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"
#include <sstream>

namespace gh_ost {

Applier::Applier(MigrationContext& context)
    : context_(context)
    , applied_count_(0)
    , row_copy_count_(0)
    , running_(false)
    , apply_queue_(10000) {
}

Applier::~Applier() {
    if (running_) {
        running_ = false;
        if (apply_thread_.joinable()) {
            apply_thread_.join();
        }
    }
}

bool Applier::Initialize() {
    // Use master connection for applying changes
    connection_ = context_.MasterConnection();
    
    if (!connection_ || !connection_->IsConnected()) {
        LOG_ERROR("Applier connection not available");
        return false;
    }
    
    return true;
}

void Applier::SetConnection(std::shared_ptr<Connection> connection) {
    connection_ = connection;
}

bool Applier::CreateGhostTable() {
    LOG_INFO_FMT("Creating ghost table: {}", context_.GhostTableName());
    
    // Create ghost table like original
    std::string sql = "CREATE TABLE " + 
        connection_->QuoteIdentifier(context_.DatabaseName()) + "." +
        connection_->QuoteIdentifier(context_.GhostTableName()) +
        " LIKE " +
        connection_->QuoteIdentifier(context_.DatabaseName()) + "." +
        connection_->QuoteIdentifier(context_.OriginalTableName());
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Failed to create ghost table: {}", result.error);
        return false;
    }
    
    LOG_INFO("Ghost table created");
    return true;
}

bool Applier::CreateChangelogTable() {
    LOG_INFO_FMT("Creating changelog table: {}", context_.ChangelogTableName());
    
    std::string sql = context_.GetSQLBuilder().BuildCreateChangelogTableSQL();
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Failed to create changelog table: {}", result.error);
        return false;
    }
    
    LOG_INFO("Changelog table created");
    return true;
}

bool Applier::AlterGhostTable() {
    LOG_INFO("Altering ghost table");
    
    auto alter = context_.GetAlterStatement();
    std::string sql = "ALTER TABLE " +
        connection_->QuoteIdentifier(context_.DatabaseName()) + "." +
        connection_->QuoteIdentifier(context_.GhostTableName()) +
        " " + StringUtils::Join(alter.operations, 
            [](const AlterOperation& op) { return op.value; }, ", ");
    
    LOG_DEBUG_FMT("Alter SQL: {}", sql);
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Failed to alter ghost table: {}", result.error);
        return false;
    }
    
    LOG_INFO("Ghost table altered");
    return true;
}

bool Applier::CopyRows(const std::string& min_key, const std::string& max_key, 
                       uint64_t chunk_size) {
    auto key = context_.GetUniqueKey();
    auto column_pairs = context_.GetColumnPairs();
    
    std::string select_sql = context_.GetSQLBuilder().BuildRowCopySQL(
        key.columns[0], min_key, max_key, chunk_size, column_pairs);
    
    // Query rows from original table
    auto rows = connection_->Query(select_sql);
    
    if (rows.empty()) {
        return true;  // No rows to copy in this chunk
    }
    
    // Insert rows into ghost table
    for (const auto& row : rows) {
        std::vector<std::string> values;
        for (size_t i = 0; i < row.values.size(); ++i) {
            values.push_back("'" + StringUtils::EscapeForSQL(row.values[i]) + "'");
        }
        
        std::string insert_sql = context_.GetSQLBuilder().BuildInsertIntoGhostSQL(
            values, column_pairs);
        
        auto result = connection_->Execute(insert_sql);
        if (!result.success) {
            LOG_ERROR_FMT("Insert failed: {}", result.error);
            return false;
        }
        
        row_copy_count_++;
        context_.CopiedRowCount()++;
    }
    
    return true;
}

uint64_t Applier::GetRowCountEstimate() {
    auto result = connection_->QueryValue(
        "SELECT TABLE_ROWS FROM information_schema.TABLES "
        "WHERE TABLE_SCHEMA = '" + context_.DatabaseName() + "' "
        "AND TABLE_NAME = '" + context_.OriginalTableName() + "'");
    
    if (result) {
        return StringUtils::ParseUInt64(*result).value_or(0);
    }
    return 0;
}

bool Applier::ApplyDMLEvent(std::shared_ptr<BinlogDMLEvent> event) {
    if (!event) return false;
    
    switch (event->DML()) {
        case DMLType::Insert:
            return ApplyInsert(*event);
        case DMLType::Update:
            return ApplyUpdate(*event);
        case DMLType::Delete:
            return ApplyDelete(*event);
        default:
            return false;
    }
}

bool Applier::ApplyInsert(const BinlogDMLEvent& event) {
    auto column_pairs = context_.GetColumnPairs();
    
    for (const auto& row : event.Rows()) {
        std::string sql = BuildInsertSQL(row);
        
        auto result = connection_->Execute(sql);
        if (!result.success) {
            LOG_ERROR_FMT("Apply INSERT failed: {}", result.error);
            return false;
        }
        
        applied_count_++;
    }
    
    return true;
}

bool Applier::ApplyUpdate(const BinlogDMLEvent& event) {
    for (const auto& row : event.Rows()) {
        std::string sql = BuildUpdateSQL(row);
        
        auto result = connection_->Execute(sql);
        if (!result.success) {
            LOG_ERROR_FMT("Apply UPDATE failed: {}", result.error);
            return false;
        }
        
        applied_count_++;
    }
    
    return true;
}

bool Applier::ApplyDelete(const BinlogDMLEvent& event) {
    for (const auto& row : event.Rows()) {
        std::string sql = BuildDeleteSQL(row);
        
        auto result = connection_->Execute(sql);
        if (!result.success) {
            LOG_ERROR_FMT("Apply DELETE failed: {}", result.error);
            return false;
        }
        
        applied_count_++;
    }
    
    return true;
}

bool Applier::ApplyBatch(const std::vector<std::shared_ptr<BinlogEntry>>& entries) {
    // Apply multiple events in a transaction
    if (!connection_->BeginTransaction()) {
        return false;
    }
    
    for (const auto& entry : entries) {
        if (entry->DmlEvent()) {
            if (!ApplyDMLEvent(entry->DmlEvent())) {
                connection_->Rollback();
                return false;
            }
        }
    }
    
    return connection_->Commit();
}

bool Applier::WriteChangelog(const std::string& hint, const std::string& value) {
    std::string sql = context_.GetSQLBuilder().BuildInsertChangelogSQL(hint, value);
    
    auto result = connection_->Execute(sql);
    return result.success;
}

bool Applier::ReadChangelog(const std::string& hint) {
    std::string sql = context_.GetSQLBuilder().BuildSelectChangelogSQL(hint);
    
    auto row = connection_->QueryRow(sql);
    return row;
}

bool Applier::ClearChangelog() {
    std::string sql = context_.GetSQLBuilder().BuildDeleteChangelogSQL();
    
    auto result = connection_->Execute(sql);
    return result.success;
}

bool Applier::WriteHeartbeat() {
    return WriteChangelog("heartbeat", StringUtils::ToString(TimeUtils::NowUnixMillis()));
}

bool Applier::LockOriginalTable() {
    LOG_INFO("Locking original table for cut-over");
    
    std::string sql = context_.GetSQLBuilder().BuildLockOriginalTableSQL();
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Lock failed: {}", result.error);
        return false;
    }
    
    LOG_INFO("Original table locked");
    return true;
}

bool Applier::UnlockTables() {
    LOG_INFO("Unlocking tables");
    
    std::string sql = "UNLOCK TABLES";
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Unlock failed: {}", result.error);
        return false;
    }
    
    LOG_INFO("Tables unlocked");
    return true;
}

bool Applier::RenameTables() {
    LOG_INFO("Renaming tables (cut-over)");
    
    // Use atomic rename if possible
    return AtomicRenameTables();
}

bool Applier::AtomicRenameTables() {
    LOG_INFO("Atomic rename tables");
    
    std::string sql = context_.GetSQLBuilder().BuildRenameTablesSQL();
    
    LOG_DEBUG_FMT("Rename SQL: {}", sql);
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Rename failed: {}", result.error);
        return false;
    }
    
    LOG_INFO("Tables renamed successfully");
    return true;
}

bool Applier::DropOldTable() {
    std::string sql = context_.GetSQLBuilder().BuildDropOldTableSQL();
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Drop old table failed: {}", result.error);
        return false;
    }
    
    LOG_INFO("Old table dropped");
    return true;
}

bool Applier::DropGhostTable() {
    std::string sql = context_.GetSQLBuilder().BuildDropGhostTableSQL();
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_ERROR_FMT("Drop ghost table failed: {}", result.error);
        return false;
    }
    
    LOG_INFO("Ghost table dropped");
    return true;
}

bool Applier::DropChangelogTable() {
    std::string sql = "DROP TABLE IF EXISTS " +
        connection_->QuoteIdentifier(context_.DatabaseName()) + "." +
        connection_->QuoteIdentifier(context_.ChangelogTableName());
    
    auto result = connection_->Execute(sql);
    if (!result.success) {
        LOG_WARN_FMT("Drop changelog table failed: {}", result.error);
        return false;
    }
    
    LOG_INFO("Changelog table dropped");
    return true;
}

std::string Applier::BuildInsertSQL(const BinlogRowData& row) {
    std::ostringstream oss;
    oss << "INSERT INTO " <<
        connection_->QuoteIdentifier(context_.DatabaseName()) << "." <<
        connection_->QuoteIdentifier(context_.GhostTableName()) <<
        " (" << StringUtils::Join(row.column_names,
            [](const std::string& col) { 
                return StringUtils::QuoteIdentifier(col); 
            }, ", ") << ") VALUES ";
    
    oss << row.ToInsertValuesSQL();
    
    return oss.str();
}

std::string Applier::BuildUpdateSQL(const BinlogRowData& row) {
    auto key = context_.GetUniqueKey();
    
    std::ostringstream oss;
    oss << "UPDATE " <<
        connection_->QuoteIdentifier(context_.DatabaseName()) << "." <<
        connection_->QuoteIdentifier(context_.GhostTableName()) <<
        " " << row.ToUpdateSQL(key.columns[0]);
    
    return oss.str();
}

std::string Applier::BuildDeleteSQL(const BinlogRowData& row) {
    auto key = context_.GetUniqueKey();
    
    std::ostringstream oss;
    oss << "DELETE FROM " <<
        connection_->QuoteIdentifier(context_.DatabaseName()) << "." <<
        connection_->QuoteIdentifier(context_.GhostTableName()) <<
        " WHERE " << StringUtils::QuoteIdentifier(key.columns[0]) << " = ";
    
    if (row.pk_value) {
        oss << "'" << StringUtils::EscapeForSQL(*row.pk_value) << "'";
    } else {
        oss << "NULL";
    }
    
    return oss.str();
}

bool Applier::ExecuteQuery(const std::string& query) {
    auto result = connection_->Execute(query);
    return result.success;
}

} // namespace gh_ost