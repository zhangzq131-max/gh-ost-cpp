/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_SQL_TYPES_HPP
#define GH_OST_SQL_TYPES_HPP

#include "gh_ost/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace gh_ost {

// ALTER operation types
enum class AlterOperationType {
    AddColumn,
    DropColumn,
    ModifyColumn,
    ChangeColumn,
    RenameColumn,
    AddIndex,
    DropIndex,
    RenameIndex,
    AddPrimaryKey,
    DropPrimaryKey,
    AddForeignKey,
    DropForeignKey,
    AddUniqueKey,
    AlterColumn,
    RenameTable,        // Not supported by gh-ost
    EngineChange,
    AutoIncrementChange,
    CharsetChange,
    CollationChange,
    CommentChange,
    Unknown
};

// ALTER operation structure
struct AlterOperation {
    AlterOperationType type;
    std::string value;          // The original SQL text for this operation
    std::string name;           // Name of column/index being modified
    std::string definition;     // Full definition (for ADD operations)
    std::string new_name;       // New name (for CHANGE/RENAME operations)
    std::string old_name;       // Old name (for CHANGE/RENAME operations)
    bool is_first;              // FIRST position
    std::optional<std::string> after_column; // AFTER column position
    
    AlterOperation() : type(AlterOperationType::Unknown) {}
};

// Column definition structure
struct ColumnDefinition {
    std::string name;
    ColumnType type;
    std::string type_detail;    // Full type string (e.g., "VARCHAR(255)")
    bool nullable;
    std::optional<std::string> default_value;
    bool auto_increment;
    bool unsigned_type;
    bool zerofill;
    std::string charset;
    std::string collation;
    std::string comment;
    bool is_first;
    std::optional<std::string> after_column;
    
    ColumnDefinition() 
        : nullable(true)
        , auto_increment(false)
        , unsigned_type(false)
        , zerofill(false)
        , is_first(false) {}
    
    std::string ToSQL() const;  // Generate SQL representation
};

// Index/Key definition
struct IndexDefinition {
    std::string name;
    UniqueKeyType type;
    std::vector<std::string> columns;
    std::string method;         // BTREE, HASH, etc.
    std::string comment;
    
    IndexDefinition() 
        : type(UniqueKeyType::None)
        , method("BTREE") {}
    
    std::string ToSQL() const;
};

// Full ALTER TABLE statement representation
struct AlterStatement {
    std::string database;
    std::string table_name;
    std::vector<AlterOperation> operations;
    std::string original_statement;
    
    bool HasAddColumn() const;
    bool HasDropColumn() const;
    bool HasModifyColumn() const;
    bool HasRenameTable() const;
    bool HasEngineChange() const;
    
    // Get specific operations
    std::vector<AlterOperation> GetAddColumnOps() const;
    std::vector<AlterOperation> GetDropColumnOps() const;
    std::vector<AlterOperation> GetIndexOps() const;
    
    // Validation
    bool IsValid() const;
    std::vector<std::string> GetValidationErrors() const;
};

// Table columns pair (for original and ghost table comparison)
struct ColumnPairs {
    std::vector<std::pair<std::string, std::string>> shared_columns;  // Columns in both tables
    std::vector<std::string> dropped_columns;                        // Columns being dropped
    std::vector<std::string> added_columns;                          // Columns being added
    std::vector<std::string> modified_columns;                       // Columns being modified
    
    // Generate column list for SELECT/INSERT
    std::string GetSharedColumnsSQL() const;
};

} // namespace gh_ost

#endif // GH_OST_SQL_TYPES_HPP