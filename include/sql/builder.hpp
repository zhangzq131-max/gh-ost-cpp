/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_SQL_BUILDER_HPP
#define GH_OST_SQL_BUILDER_HPP

#include "types.hpp"
#include "parser.hpp"
#include "gh_ost/types.hpp"
#include "mysql/connection.hpp"
#include <string>
#include <vector>
#include <optional>

namespace gh_ost {

// SQL statement builder for gh-ost operations
class SQLBuilder {
public:
    SQLBuilder(const std::string& database, const std::string& table,
               const std::string& ghost_table, const std::string& changelog_table);
    
    // Table creation
    std::string BuildCreateGhostTableSQL(const TableStructure& original_structure,
                                         const AlterStatement& alter) const;
    
    std::string BuildCreateChangelogTableSQL() const;
    
    // Row copy SQL
    std::string BuildRowCopySQL(const std::string& key_column,
                                const std::string& min_value,
                                const std::string& max_value,
                                uint64_t chunk_size,
                                const ColumnPairs& column_pairs) const;
    
    // Row copy range SQL
    std::string BuildMinKeySQL(const std::string& key_column) const;
    std::string BuildMaxKeySQL(const std::string& key_column) const;
    
    // Apply DML to ghost table
    std::string BuildInsertIntoGhostSQL(const std::vector<std::string>& values,
                                        const ColumnPairs& column_pairs) const;
    
    std::string BuildUpdateGhostSQL(const std::string& key_column,
                                    const std::string& key_value,
                                    const std::vector<std::pair<std::string, std::string>>& values,
                                    const ColumnPairs& column_pairs) const;
    
    std::string BuildDeleteFromGhostSQL(const std::string& key_column,
                                        const std::string& key_value) const;
    
    // Changelog operations
    std::string BuildInsertChangelogSQL(const std::string& hint,
                                        const std::string& value = "") const;
    
    std::string BuildSelectChangelogSQL(const std::string& hint) const;
    
    std::string BuildSelectLatestChangelogSQL() const;
    
    std::string BuildDeleteChangelogSQL() const;
    
    // Cut-over SQL
    std::string BuildLockOriginalTableSQL() const;
    std::string BuildUnlockTablesSQL() const;
    std::string BuildRenameTablesSQL() const;
    std::string BuildRenameOriginalToOldSQL() const;
    std::string BuildRenameGhostToOriginalSQL() const;
    
    // Drop table SQL
    std::string BuildDropOldTableSQL() const;
    std::string BuildDropGhostTableSQL() const;
    
    // Row count SQL
    std::string BuildSelectRowCountSQL(bool exact = false) const;
    
    // Status queries
    std::string BuildGetNextChunkSQL(const std::string& key_column,
                                     const std::string& last_value,
                                     uint64_t chunk_size) const;
    
    // Identify shared columns
    ColumnPairs BuildColumnPairs(const TableStructure& original,
                                 const AlterStatement& alter) const;
    
    // Utility
    std::string QuoteIdentifier(const std::string& identifier) const;
    std::string BuildTableFullName(const std::string& table) const;
    
private:
    std::string database_;
    std::string original_table_;
    std::string ghost_table_;
    std::string changelog_table_;
    
    // Helper
    std::string BuildColumnListSQL(const std::vector<std::string>& columns) const;
    std::string BuildColumnPairsSQL(const ColumnPairs& pairs, 
                                    bool use_ghost_names = false) const;
};

} // namespace gh_ost

#endif // GH_OST_SQL_BUILDER_HPP