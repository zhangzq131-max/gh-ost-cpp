/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "sql/builder.hpp"
#include "utils/string_utils.hpp"
#include "utils/logger.hpp"
#include <sstream>

namespace gh_ost {

SQLBuilder::SQLBuilder(const std::string& database, const std::string& table,
                       const std::string& ghost_table, const std::string& changelog_table)
    : database_(database)
    , original_table_(table)
    , ghost_table_(ghost_table)
    , changelog_table_(changelog_table) {}

std::string SQLBuilder::BuildCreateGhostTableSQL(const TableStructure& original_structure,
                                                  const AlterStatement& alter) const {
    std::ostringstream oss;
    oss << "CREATE TABLE " << BuildTableFullName(ghost_table_);
    
    // Build column definitions
    oss << " (" << "\n";
    
    for (size_t i = 0; i < original_structure.columns.size(); ++i) {
        const auto& col = original_structure.columns[i];
        
        // Check if column is being dropped
        bool is_dropped = false;
        for (const auto& op : alter.GetDropColumnOps()) {
            if (op.name == col.name) {
                is_dropped = true;
                break;
            }
        }
        
        if (is_dropped) continue;
        
        oss << "  " << col.ToString();
        
        // Add comma if not last column
        if (i < original_structure.columns.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    
    // Add new columns from ALTER
    for (const auto& op : alter.GetAddColumnOps()) {
        oss << ", " << op.definition;
    }
    
    // Add primary key
    auto pk = original_structure.GetPrimaryKey();
    if (pk) {
        oss << ", " << pk->ToString();
    }
    
    // Add keys
    for (const auto& key : original_structure.keys) {
        if (!key.is_primary) {
            oss << ", " << key.ToString();
        }
    }
    
    oss << ")";
    
    return oss.str();
}

std::string SQLBuilder::BuildCreateChangelogTableSQL() const {
    std::ostringstream oss;
    oss << "CREATE TABLE " << BuildTableFullName(changelog_table_);
    oss << " ("
        << "id bigint auto_increment, "
        << "hint varchar(64) not null, "
        << "value varchar(255) not null, "
        << "ts timestamp not null default current_timestamp, "
        << "primary key(id)"
        << ")";
    
    return oss.str();
}

std::string SQLBuilder::BuildRowCopySQL(const std::string& key_column,
                                        const std::string& min_value,
                                        const std::string& max_value,
                                        uint64_t chunk_size,
                                        const ColumnPairs& column_pairs) const {
    std::ostringstream oss;
    oss << "SELECT /*+ gh-ost-copy-row */ ";
    
    // Use shared columns for copy
    oss << column_pairs.GetSharedColumnsSQL();
    
    oss << " FROM " << BuildTableFullName(original_table_);
    oss << " WHERE " << QuoteIdentifier(key_column) << " > " << StringUtils::QuoteForSQL(min_value);
    oss << " AND " << QuoteIdentifier(key_column) << " <= " << StringUtils::QuoteForSQL(max_value);
    oss << " ORDER BY " << QuoteIdentifier(key_column);
    oss << " LIMIT " << chunk_size;
    
    return oss.str();
}

std::string SQLBuilder::BuildMinKeySQL(const std::string& key_column) const {
    std::ostringstream oss;
    oss << "SELECT MIN(" << QuoteIdentifier(key_column) << ")";
    oss << " FROM " << BuildTableFullName(original_table_);
    
    return oss.str();
}

std::string SQLBuilder::BuildMaxKeySQL(const std::string& key_column) const {
    std::ostringstream oss;
    oss << "SELECT MAX(" << QuoteIdentifier(key_column) << ")";
    oss << " FROM " << BuildTableFullName(original_table_);
    
    return oss.str();
}

std::string SQLBuilder::BuildInsertIntoGhostSQL(const std::vector<std::string>& values,
                                                 const ColumnPairs& column_pairs) const {
    std::ostringstream oss;
    oss << "INSERT /*+ gh-ost-write */ INTO " << BuildTableFullName(ghost_table_);
    
    // Use shared columns
    std::vector<std::string> ghost_columns;
    for (const auto& pair : column_pairs.shared_columns) {
        ghost_columns.push_back(pair.second);  // Use ghost table column names
    }
    
    oss << " (" << StringUtils::Join(ghost_columns, ", ") << ")";
    oss << " VALUES (" << StringUtils::Join(values, ", ") << ")";
    
    return oss.str();
}

std::string SQLBuilder::BuildUpdateGhostSQL(const std::string& key_column,
                                            const std::string& key_value,
                                            const std::vector<std::pair<std::string, std::string>>& values,
                                            const ColumnPairs& column_pairs) const {
    std::ostringstream oss;
    oss << "UPDATE /*+ gh-ost-write */ " << BuildTableFullName(ghost_table_);
    
    // Build SET clause
    oss << " SET ";
    for (size_t i = 0; i < values.size(); ++i) {
        // Use ghost table column name
        std::string ghost_col = values[i].first;
        oss << QuoteIdentifier(ghost_col) << " = " << values[i].second;
        if (i < values.size() - 1) oss << ", ";
    }
    
    oss << " WHERE " << QuoteIdentifier(key_column) << " = " << StringUtils::QuoteForSQL(key_value);
    
    return oss.str();
}

std::string SQLBuilder::BuildDeleteFromGhostSQL(const std::string& key_column,
                                                 const std::string& key_value) const {
    std::ostringstream oss;
    oss << "DELETE /*+ gh-ost-write */ FROM " << BuildTableFullName(ghost_table_);
    oss << " WHERE " << QuoteIdentifier(key_column) << " = " << StringUtils::QuoteForSQL(key_value);
    
    return oss.str();
}

std::string SQLBuilder::BuildInsertChangelogSQL(const std::string& hint,
                                                 const std::string& value) const {
    std::ostringstream oss;
    oss << "INSERT INTO " << BuildTableFullName(changelog_table_);
    oss << " (hint, value) VALUES ('" << StringUtils::EscapeForSQL(hint) << "', '";
    oss << StringUtils::EscapeForSQL(value) << "')";
    
    return oss.str();
}

std::string SQLBuilder::BuildSelectChangelogSQL(const std::string& hint) const {
    std::ostringstream oss;
    oss << "SELECT * FROM " << BuildTableFullName(changelog_table_);
    oss << " WHERE hint = '" << StringUtils::EscapeForSQL(hint) << "'";
    oss << " ORDER BY id DESC LIMIT 1";
    
    return oss.str();
}

std::string SQLBuilder::BuildSelectLatestChangelogSQL() const {
    std::ostringstream oss;
    oss << "SELECT * FROM " << BuildTableFullName(changelog_table_);
    oss << " ORDER BY id DESC LIMIT 1";
    
    return oss.str();
}

std::string SQLBuilder::BuildDeleteChangelogSQL() const {
    std::ostringstream oss;
    oss << "DELETE FROM " << BuildTableFullName(changelog_table_);
    
    return oss.str();
}

std::string SQLBuilder::BuildLockOriginalTableSQL() const {
    std::ostringstream oss;
    oss << "LOCK TABLES " << BuildTableFullName(original_table_) << " WRITE";
    
    return oss.str();
}

std::string SQLBuilder::BuildUnlockTablesSQL() const {
    return "UNLOCK TABLES";
}

std::string SQLBuilder::BuildRenameTablesSQL() const {
    std::ostringstream oss;
    oss << "RENAME TABLE " << BuildTableFullName(original_table_);
    oss << " TO " << BuildTableFullName(original_table_ + "_old");
    oss << ", " << BuildTableFullName(ghost_table_);
    oss << " TO " << BuildTableFullName(original_table_);
    
    return oss.str();
}

std::string SQLBuilder::BuildRenameOriginalToOldSQL() const {
    std::ostringstream oss;
    oss << "RENAME TABLE " << BuildTableFullName(original_table_);
    oss << " TO " << BuildTableFullName(original_table_ + "_old");
    
    return oss.str();
}

std::string SQLBuilder::BuildRenameGhostToOriginalSQL() const {
    std::ostringstream oss;
    oss << "RENAME TABLE " << BuildTableFullName(ghost_table_);
    oss << " TO " << BuildTableFullName(original_table_);
    
    return oss.str();
}

std::string SQLBuilder::BuildDropOldTableSQL() const {
    std::ostringstream oss;
    oss << "DROP TABLE IF EXISTS " << BuildTableFullName(original_table_ + "_old");
    
    return oss.str();
}

std::string SQLBuilder::BuildDropGhostTableSQL() const {
    std::ostringstream oss;
    oss << "DROP TABLE IF EXISTS " << BuildTableFullName(ghost_table_);
    
    return oss.str();
}

std::string SQLBuilder::BuildSelectRowCountSQL(bool exact) const {
    std::ostringstream oss;
    
    if (exact) {
        oss << "SELECT COUNT(*) FROM " << BuildTableFullName(original_table_);
    } else {
        // Use approximate row count from table stats
        oss << "SELECT TABLE_ROWS FROM information_schema.TABLES ";
        oss << "WHERE TABLE_SCHEMA = '" << database_ << "' ";
        oss << "AND TABLE_NAME = '" << original_table_ << "'";
    }
    
    return oss.str();
}

std::string SQLBuilder::BuildGetNextChunkSQL(const std::string& key_column,
                                             const std::string& last_value,
                                             uint64_t chunk_size) const {
    std::ostringstream oss;
    oss << "SELECT " << QuoteIdentifier(key_column);
    oss << " FROM " << BuildTableFullName(original_table_);
    oss << " WHERE " << QuoteIdentifier(key_column) << " > " << StringUtils::QuoteForSQL(last_value);
    oss << " ORDER BY " << QuoteIdentifier(key_column);
    oss << " LIMIT 1";
    
    return oss.str();
}

ColumnPairs SQLBuilder::BuildColumnPairs(const TableStructure& original,
                                         const AlterStatement& alter) const {
    ColumnPairs pairs;
    
    // Get dropped columns
    for (const auto& op : alter.GetDropColumnOps()) {
        pairs.dropped_columns.push_back(op.name);
    }
    
    // Get added columns
    for (const auto& op : alter.GetAddColumnOps()) {
        pairs.added_columns.push_back(op.name);
    }
    
    // Get modified columns
    for (const auto& op : alter.operations) {
        if (op.type == AlterOperationType::ModifyColumn ||
            op.type == AlterOperationType::ChangeColumn) {
            pairs.modified_columns.push_back(op.name);
        }
    }
    
    // Build shared columns (columns in original that remain in ghost)
    for (const auto& col : original.columns) {
        if (std::find(pairs.dropped_columns.begin(), pairs.dropped_columns.end(), col.name) ==
            pairs.dropped_columns.end()) {
            // Column name stays the same (unless renamed)
            pairs.shared_columns.push_back({col.name, col.name});
        }
    }
    
    return pairs;
}

std::string SQLBuilder::QuoteIdentifier(const std::string& identifier) const {
    return StringUtils::QuoteIdentifier(identifier);
}

std::string SQLBuilder::BuildTableFullName(const std::string& table) const {
    return QuoteIdentifier(database_) + "." + QuoteIdentifier(table);
}

std::string SQLBuilder::BuildColumnListSQL(const std::vector<std::string>& columns) const {
    std::vector<std::string> quoted;
    for (const auto& col : columns) {
        quoted.push_back(QuoteIdentifier(col));
    }
    return StringUtils::Join(quoted, ", ");
}

} // namespace gh_ost