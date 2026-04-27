/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_SQL_PARSER_HPP
#define GH_OST_SQL_PARSER_HPP

#include "types.hpp"
#include "gh_ost/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace gh_ost {

// ALTER TABLE statement parser
class AlterTableParser {
public:
    AlterTableParser();
    
    // Parse ALTER TABLE statement
    bool Parse(const std::string& alter_statement);
    
    // Get parsed result
    AlterStatement GetAlterStatement() const;
    
    // Get original table name
    std::string GetTableName() const;
    
    // Get database name (if specified)
    std::optional<std::string> GetDatabaseName() const;
    
    // Check if parsed successfully
    bool IsValid() const;
    
    // Get validation errors
    std::vector<std::string> GetValidationErrors() const;
    
    // Check for unsupported operations
    bool HasUnsupportedOperation() const;
    std::vector<std::string> GetUnsupportedOperations() const;
    
    // Check if this is a "trivial" migration (no actual changes)
    bool IsTrivialMigration() const;
    
    // Check for RENAME TABLE operation
    bool HasRenameTable() const;
    
    // Get operations
    std::vector<AlterOperation> GetOperations() const;
    
    // Normalize the ALTER statement (clean up, format)
    std::string Normalize() const;
    
private:
    AlterStatement alter_statement_;
    std::vector<std::string> validation_errors_;
    bool parsed_;
    
    // Parse helpers
    void ParseTableName(const std::string& statement);
    void ParseOperations(const std::string& statement);
    
    AlterOperation ParseAddColumn(const std::string& operation);
    AlterOperation ParseDropColumn(const std::string& operation);
    AlterOperation ParseModifyColumn(const std::string& operation);
    AlterOperation ParseChangeColumn(const std::string& operation);
    AlterOperation ParseAddIndex(const std::string& operation);
    AlterOperation ParseDropIndex(const std::string& operation);
    AlterOperation ParseAddPrimaryKey(const std::string& operation);
    AlterOperation ParseDropPrimaryKey(const std::string& operation);
    AlterOperation ParseEngineChange(const std::string& operation);
    
    // Utility
    std::string TrimParentheses(const std::string& str);
    std::vector<std::string> ParseColumnList(const std::string& columns_str);
    ColumnDefinition ParseColumnDefinition(const std::string& definition);
    IndexDefinition ParseIndexDefinition(const std::string& definition);
};

// SQL utility parser for various queries
class SQLParser {
public:
    // Parse a SELECT query to extract components
    struct SelectComponents {
        std::string columns;
        std::string table;
        std::string where_clause;
        std::string order_by;
        std::string limit;
    };
    
    static std::optional<SelectComponents> ParseSelect(const std::string& query);
    
    // Parse INSERT query
    struct InsertComponents {
        std::string table;
        std::vector<std::string> columns;
        std::vector<std::string> values;
    };
    
    static std::optional<InsertComponents> ParseInsert(const std::string& query);
    
    // Parse UPDATE query
    struct UpdateComponents {
        std::string table;
        std::vector<std::pair<std::string, std::string>> updates;
        std::string where_clause;
    };
    
    static std::optional<UpdateComponents> ParseUpdate(const std::string& query);
    
    // Parse DELETE query
    struct DeleteComponents {
        std::string table;
        std::string where_clause;
    };
    
    static std::optional<DeleteComponents> ParseDelete(const std::string& query);
    
    // Extract table name from various SQL statements
    static std::optional<std::string> ExtractTableName(const std::string& query);
    
    // Check if query is a specific type
    static bool IsSelect(const std::string& query);
    static bool IsInsert(const std::string& query);
    static bool IsUpdate(const std::string& query);
    static bool IsDelete(const std::string& query);
    static bool IsDDL(const std::string& query);
};

} // namespace gh_ost

#endif // GH_OST_SQL_PARSER_HPP