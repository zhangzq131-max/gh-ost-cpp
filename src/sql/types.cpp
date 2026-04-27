/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "sql/types.hpp"
#include "utils/string_utils.hpp"
#include <sstream>
#include <algorithm>

namespace gh_ost {

// ColumnDefinition methods
std::string ColumnDefinition::ToSQL() const {
    std::ostringstream oss;
    oss << name << " " << type_detail;
    
    if (!nullable) oss << " NOT NULL";
    if (default_value) oss << " DEFAULT '" << *default_value << "'";
    if (auto_increment) oss << " AUTO_INCREMENT";
    
    if (is_first) oss << " FIRST";
    else if (after_column) oss << " AFTER " << *after_column;
    
    return oss.str();
}

// IndexDefinition methods
std::string IndexDefinition::ToSQL() const {
    std::ostringstream oss;
    
    if (type == UniqueKeyType::Primary) {
        oss << "PRIMARY KEY";
    } else if (type == UniqueKeyType::Unique) {
        oss << "UNIQUE KEY " << name;
    } else {
        oss << "INDEX " << name;
    }
    
    oss << " (" << StringUtils::Join(columns, ", ") << ")";
    
    if (method != "BTREE") {
        oss << " USING " << method;
    }
    
    return oss.str();
}

// AlterStatement methods
bool AlterStatement::HasAddColumn() const {
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::AddColumn) return true;
    }
    return false;
}

bool AlterStatement::HasDropColumn() const {
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::DropColumn) return true;
    }
    return false;
}

bool AlterStatement::HasModifyColumn() const {
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::ModifyColumn) return true;
    }
    return false;
}

bool AlterStatement::HasRenameTable() const {
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::RenameTable) return true;
    }
    return false;
}

bool AlterStatement::HasEngineChange() const {
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::EngineChange) return true;
    }
    return false;
}

std::vector<AlterOperation> AlterStatement::GetAddColumnOps() const {
    std::vector<AlterOperation> ops;
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::AddColumn) ops.push_back(op);
    }
    return ops;
}

std::vector<AlterOperation> AlterStatement::GetDropColumnOps() const {
    std::vector<AlterOperation> ops;
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::DropColumn) ops.push_back(op);
    }
    return ops;
}

std::vector<AlterOperation> AlterStatement::GetIndexOps() const {
    std::vector<AlterOperation> ops;
    for (const auto& op : operations) {
        if (op.type == AlterOperationType::AddIndex ||
            op.type == AlterOperationType::DropIndex ||
            op.type == AlterOperationType::AddPrimaryKey ||
            op.type == AlterOperationType::DropPrimaryKey) {
            ops.push_back(op);
        }
    }
    return ops;
}

bool AlterStatement::IsValid() const {
    if (HasRenameTable()) return false;
    return !GetValidationErrors().empty() == false;
}

std::vector<std::string> AlterStatement::GetValidationErrors() const {
    std::vector<std::string> errors;
    
    if (table_name.empty()) {
        errors.push_back("Table name is required");
    }
    
    if (operations.empty()) {
        errors.push_back("No ALTER operations specified");
    }
    
    if (HasRenameTable()) {
        errors.push_back("ALTER TABLE ... RENAME is not supported by gh-ost");
    }
    
    return errors;
}

// ColumnPairs methods
std::string ColumnPairs::GetSharedColumnsSQL() const {
    std::vector<std::string> columns;
    for (const auto& pair : shared_columns) {
        columns.push_back(pair.first);
    }
    return StringUtils::Join(columns, ", ");
}

} // namespace gh_ost