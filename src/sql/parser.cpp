/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "sql/parser.hpp"
#include "utils/string_utils.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <regex>

namespace gh_ost {

AlterTableParser::AlterTableParser()
    : parsed_(false) {}

bool AlterTableParser::Parse(const std::string& alter_statement) {
    alter_statement_.original_statement = StringUtils::RemoveSQLComments(alter_statement);
    alter_statement_.operations.clear();
    validation_errors_.clear();
    
    std::string statement = StringUtils::Trim(alter_statement_.original_statement);
    
    // Remove ALTER TABLE prefix
    statement = StringUtils::Replace(statement, "ALTER TABLE ", "");
    
    // Parse table name (could be database.table)
    ParseTableName(statement);
    
    // Find the operations part after table name
    size_t pos = statement.find_first_of(" \t\n\r");
    if (pos != std::string::npos) {
        std::string operations_str = StringUtils::Trim(statement.substr(pos));
        ParseOperations(operations_str);
    }
    
    // Validate
    validation_errors_ = alter_statement_.GetValidationErrors();
    
    parsed_ = !alter_statement_.operations.empty() && validation_errors_.empty();
    
    if (!parsed_) {
        LOG_WARN_FMT("ALTER parsing failed: {}", StringUtils::Join(validation_errors_, "; "));
    }
    
    return parsed_;
}

void AlterTableParser::ParseTableName(const std::string& statement) {
    std::regex table_regex("([a-zA-Z_][a-zA-Z0-9_]*(?:\\.[a-zA-Z_][a-zA-Z0-9_]*)?)");
    std::smatch match;
    
    if (std::regex_search(statement, match, table_regex)) {
        std::string table_part = match[1].str();
        
        // Check if it contains database.table
        auto parts = StringUtils::Split(table_part, '.');
        if (parts.size() == 2) {
            alter_statement_.database = StringUtils::NormalizeTableName(parts[0]);
            alter_statement_.table_name = StringUtils::NormalizeTableName(parts[1]);
        } else {
            alter_statement_.table_name = StringUtils::NormalizeTableName(table_part);
        }
    }
}

void AlterTableParser::ParseOperations(const std::string& statement) {
    std::string clean_statement = StringUtils::Trim(statement);
    
    // Split by comma for multiple operations
    std::vector<std::string> operation_parts;
    
    // Need to handle commas inside parentheses carefully
    std::string current;
    int paren_depth = 0;
    for (size_t i = 0; i < clean_statement.length(); ++i) {
        char c = clean_statement[i];
        
        if (c == '(') paren_depth++;
        else if (c == ')') paren_depth--;
        else if (c == ',' && paren_depth == 0) {
            if (!current.empty()) {
                operation_parts.push_back(StringUtils::Trim(current));
            }
            current.clear();
            continue;
        }
        current += c;
    }
    
    if (!current.empty()) {
        operation_parts.push_back(StringUtils::Trim(current));
    }
    
    // Parse each operation
    for (const auto& op_str : operation_parts) {
        AlterOperation op;
        std::string upper_op = StringUtils::ToUpper(op_str);
        
        if (StringUtils::StartsWith(upper_op, "ADD COLUMN ")) {
            op = ParseAddColumn(op_str);
        } else if (StringUtils::StartsWith(upper_op, "ADD ")) {
            // Could be ADD INDEX, ADD KEY, ADD PRIMARY KEY, etc.
            if (StringUtils::Contains(upper_op, "PRIMARY KEY")) {
                op = ParseAddPrimaryKey(op_str);
            } else if (StringUtils::Contains(upper_op, "UNIQUE") ||
                       StringUtils::Contains(upper_op, "KEY") ||
                       StringUtils::Contains(upper_op, "INDEX")) {
                op = ParseAddIndex(op_str);
            } else {
                // Assume ADD COLUMN without COLUMN keyword
                op = ParseAddColumn("ADD COLUMN " + op_str.substr(4));
            }
        } else if (StringUtils::StartsWith(upper_op, "DROP COLUMN ")) {
            op = ParseDropColumn(op_str);
        } else if (StringUtils::StartsWith(upper_op, "DROP ")) {
            // Could be DROP INDEX, DROP KEY, DROP PRIMARY KEY, etc.
            if (StringUtils::Contains(upper_op, "PRIMARY KEY")) {
                op = ParseDropPrimaryKey(op_str);
            } else if (StringUtils::Contains(upper_op, "KEY") ||
                       StringUtils::Contains(upper_op, "INDEX")) {
                op = ParseDropIndex(op_str);
            } else {
                // Assume DROP COLUMN without COLUMN keyword
                op = ParseDropColumn("DROP COLUMN " + op_str.substr(5));
            }
        } else if (StringUtils::StartsWith(upper_op, "MODIFY COLUMN ")) {
            op = ParseModifyColumn(op_str);
        } else if (StringUtils::StartsWith(upper_op, "MODIFY ")) {
            op = ParseModifyColumn("MODIFY COLUMN " + op_str.substr(7));
        } else if (StringUtils::StartsWith(upper_op, "CHANGE COLUMN ")) {
            op = ParseChangeColumn(op_str);
        } else if (StringUtils::StartsWith(upper_op, "CHANGE ")) {
            op = ParseChangeColumn("CHANGE COLUMN " + op_str.substr(7));
        } else if (StringUtils::StartsWith(upper_op, "RENAME ") ||
                   StringUtils::StartsWith(upper_op, "RENAME TO ")) {
            op.type = AlterOperationType::RenameTable;
            op.value = op_str;
        } else if (StringUtils::StartsWith(upper_op, "ENGINE ")) {
            op = ParseEngineChange(op_str);
        } else {
            // Unknown operation
            op.type = AlterOperationType::Unknown;
            op.value = op_str;
            LOG_WARN_FMT("Unknown ALTER operation: {}", op_str);
        }
        
        alter_statement_.operations.push_back(op);
    }
}

AlterOperation AlterTableParser::ParseAddColumn(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::AddColumn;
    op.value = operation;
    
    // Remove "ADD COLUMN " prefix
    std::string def = StringUtils::Replace(operation, "ADD COLUMN ", "");
    def = StringUtils::Trim(def);
    
    // Parse column definition
    ColumnDefinition col = ParseColumnDefinition(def);
    op.name = col.name;
    op.definition = def;
    
    if (StringUtils::Contains(def, "FIRST")) {
        op.is_first = true;
    } else if (StringUtils::Contains(def, "AFTER ")) {
        size_t after_pos = def.find("AFTER ");
        op.after_column = StringUtils::Trim(def.substr(after_pos + 6));
    }
    
    return op;
}

AlterOperation AlterTableParser::ParseDropColumn(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::DropColumn;
    op.value = operation;
    
    // Remove "DROP COLUMN " prefix
    std::string name = StringUtils::Replace(operation, "DROP COLUMN ", "");
    name = StringUtils::Trim(name);
    
    // Remove any quotes
    op.name = StringUtils::NormalizeTableName(name);
    
    return op;
}

AlterOperation AlterTableParser::ParseModifyColumn(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::ModifyColumn;
    op.value = operation;
    
    // Remove "MODIFY COLUMN " prefix
    std::string def = StringUtils::Replace(operation, "MODIFY COLUMN ", "");
    def = StringUtils::Trim(def);
    
    ColumnDefinition col = ParseColumnDefinition(def);
    op.name = col.name;
    op.definition = def;
    
    if (StringUtils::Contains(def, "FIRST")) {
        op.is_first = true;
    } else if (StringUtils::Contains(def, "AFTER ")) {
        size_t after_pos = def.find("AFTER ");
        op.after_column = StringUtils::Trim(def.substr(after_pos + 6));
    }
    
    return op;
}

AlterOperation AlterTableParser::ParseChangeColumn(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::ChangeColumn;
    op.value = operation;
    
    // Remove "CHANGE COLUMN " prefix
    std::string def = StringUtils::Replace(operation, "CHANGE COLUMN ", "");
    def = StringUtils::Trim(def);
    
    // Parse: old_name new_name definition
    auto parts = StringUtils::Split(def, ' ');
    if (parts.size() >= 2) {
        op.old_name = StringUtils::NormalizeTableName(parts[0]);
        op.name = StringUtils::NormalizeTableName(parts[1]);
        // Rest is definition
        op.definition = StringUtils::Join(
            std::vector<std::string>(parts.begin() + 2, parts.end()), " "
        );
    }
    
    return op;
}

AlterOperation AlterTableParser::ParseAddIndex(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::AddIndex;
    op.value = operation;
    
    // Parse: ADD INDEX name (columns) or ADD KEY name (columns)
    std::string upper = StringUtils::ToUpper(operation);
    std::string def = operation;
    
    // Remove "ADD INDEX " or "ADD KEY " or "ADD UNIQUE INDEX "
    if (StringUtils::Contains(upper, "UNIQUE")) {
        op.type = AlterOperationType::AddUniqueKey;
    }
    
    // Find index name and columns
    std::regex index_regex("ADD\\s+(?:UNIQUE\\s+)?(?:INDEX|KEY)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^)]+)\\)");
    std::smatch match;
    
    if (std::regex_search(operation, match, index_regex)) {
        op.name = match[1].str();
        op.definition = match[2].str();
    } else {
        // Index without name
        size_t paren_pos = operation.find('(');
        if (paren_pos != std::string::npos) {
            op.definition = TrimParentheses(operation.substr(paren_pos));
        }
    }
    
    return op;
}

AlterOperation AlterTableParser::ParseDropIndex(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::DropIndex;
    op.value = operation;
    
    // Parse: DROP INDEX name or DROP KEY name
    std::regex drop_regex("DROP\\s+(?:INDEX|KEY)\\s+([a-zA-Z_][a-zA-Z0-9_]*)");
    std::smatch match;
    
    if (std::regex_search(operation, match, drop_regex)) {
        op.name = match[1].str();
    }
    
    return op;
}

AlterOperation AlterTableParser::ParseAddPrimaryKey(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::AddPrimaryKey;
    op.value = operation;
    
    // Parse: ADD PRIMARY KEY (columns)
    size_t paren_pos = operation.find('(');
    if (paren_pos != std::string::npos) {
        op.definition = TrimParentheses(operation.substr(paren_pos));
    }
    
    return op;
}

AlterOperation AlterTableParser::ParseDropPrimaryKey(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::DropPrimaryKey;
    op.value = operation;
    
    // DROP PRIMARY KEY doesn't need additional parsing
    return op;
}

AlterOperation AlterTableParser::ParseEngineChange(const std::string& operation) {
    AlterOperation op;
    op.type = AlterOperationType::EngineChange;
    op.value = operation;
    
    // Parse: ENGINE = name
    std::regex engine_regex("ENGINE\\s*=\\s*([a-zA-Z]+)");
    std::smatch match;
    
    if (std::regex_search(operation, match, engine_regex)) {
        op.name = match[1].str();
    }
    
    return op;
}

std::string AlterTableParser::TrimParentheses(const std::string& str) {
    std::string result = StringUtils::Trim(str);
    if (result.front() == '(' && result.back() == ')') {
        result = result.substr(1, result.length() - 2);
    }
    return StringUtils::Trim(result);
}

std::vector<std::string> AlterTableParser::ParseColumnList(const std::string& columns_str) {
    std::vector<std::string> columns;
    auto parts = StringUtils::Split(columns_str, ',');
    for (const auto& part : parts) {
        columns.push_back(StringUtils::NormalizeTableName(part));
    }
    return columns;
}

ColumnDefinition AlterTableParser::ParseColumnDefinition(const std::string& definition) {
    ColumnDefinition col;
    
    // Parse column name and type
    std::regex col_regex("([a-zA-Z_][a-zA-Z0-9_]*)\\s+([a-zA-Z]+(?:\\([^)]+\\))?)");
    std::smatch match;
    
    if (std::regex_search(definition, match, col_regex)) {
        col.name = match[1].str();
        col.type_detail = match[2].str();
        
        // Determine type
        std::string type_upper = StringUtils::ToUpper(col.type_detail);
        if (type_upper.find("INT") != std::string::npos) col.type = ColumnType::Integer;
        else if (type_upper.find("FLOAT") != std::string::npos) col.type = ColumnType::Float;
        else if (type_upper.find("DOUBLE") != std::string::npos) col.type = ColumnType::Double;
        else if (type_upper.find("DECIMAL") != std::string::npos) col.type = ColumnType::Decimal;
        else if (type_upper.find("VARCHAR") != std::string::npos) col.type = ColumnType::String;
        else if (type_upper.find("CHAR") != std::string::npos) col.type = ColumnType::String;
        else if (type_upper.find("TEXT") != std::string::npos) col.type = ColumnType::Text;
        else if (type_upper.find("BLOB") != std::string::npos) col.type = ColumnType::Blob;
        else if (type_upper.find("DATETIME") != std::string::npos) col.type = ColumnType::DateTime;
        else if (type_upper.find("DATE") != std::string::npos) col.type = ColumnType::Date;
        else if (type_upper.find("TIME") != std::string::npos) col.type = ColumnType::Time;
        else if (type_upper.find("TIMESTAMP") != std::string::npos) col.type = ColumnType::Timestamp;
        else if (type_upper.find("YEAR") != std::string::npos) col.type = ColumnType::Year;
        else if (type_upper.find("BOOL") != std::string::npos) col.type = ColumnType::Bool;
        else if (type_upper.find("ENUM") != std::string::npos) col.type = ColumnType::Enum;
        else if (type_upper.find("SET") != std::string::npos) col.type = ColumnType::Set;
        else if (type_upper.find("JSON") != std::string::npos) col.type = ColumnType::Json;
    }
    
    // Check for NOT NULL
    col.nullable = !StringUtils::Contains(StringUtils::ToUpper(definition), "NOT NULL");
    
    // Check for AUTO_INCREMENT
    col.auto_increment = StringUtils::Contains(StringUtils::ToUpper(definition), "AUTO_INCREMENT");
    
    // Check for UNSIGNED
    col.unsigned_type = StringUtils::Contains(StringUtils::ToUpper(definition), "UNSIGNED");
    
    // Check for position
    if (StringUtils::Contains(definition, "FIRST")) col.is_first = true;
    else if (StringUtils::Contains(definition, "AFTER ")) {
        size_t after_pos = definition.find("AFTER ");
        col.after_column = StringUtils::Trim(definition.substr(after_pos + 6));
    }
    
    return col;
}

AlterStatement AlterTableParser::GetAlterStatement() const {
    return alter_statement_;
}

std::string AlterTableParser::GetTableName() const {
    return alter_statement_.table_name;
}

std::optional<std::string> AlterTableParser::GetDatabaseName() const {
    if (alter_statement_.database.empty()) return std::nullopt;
    return alter_statement_.database;
}

bool AlterTableParser::IsValid() const {
    return parsed_;
}

std::vector<std::string> AlterTableParser::GetValidationErrors() const {
    return validation_errors_;
}

bool AlterTableParser::HasUnsupportedOperation() const {
    return alter_statement_.HasRenameTable();
}

std::vector<std::string> AlterTableParser::GetUnsupportedOperations() const {
    std::vector<std::string> unsupported;
    for (const auto& op : alter_statement_.operations) {
        if (op.type == AlterOperationType::RenameTable) {
            unsupported.push_back(op.value);
        }
    }
    return unsupported;
}

bool AlterTableParser::IsTrivialMigration() const {
    // Trivial migration has no actual changes
    return alter_statement_.operations.empty();
}

bool AlterTableParser::HasRenameTable() const {
    return alter_statement_.HasRenameTable();
}

std::vector<AlterOperation> AlterTableParser::GetOperations() const {
    return alter_statement_.operations;
}

std::string AlterTableParser::Normalize() const {
    std::ostringstream oss;
    oss << "ALTER TABLE " << alter_statement_.table_name << " ";
    
    std::vector<std::string> op_strings;
    for (const auto& op : alter_statement_.operations) {
        op_strings.push_back(op.value);
    }
    oss << StringUtils::Join(op_strings, ", ");
    
    return oss.str();
}

// SQLParser static methods
bool SQLParser::IsSelect(const std::string& query) {
    return StringUtils::StartsWith(StringUtils::ToUpper(StringUtils::Trim(query)), "SELECT");
}

bool SQLParser::IsInsert(const std::string& query) {
    return StringUtils::StartsWith(StringUtils::ToUpper(StringUtils::Trim(query)), "INSERT");
}

bool SQLParser::IsUpdate(const std::string& query) {
    return StringUtils::StartsWith(StringUtils::ToUpper(StringUtils::Trim(query)), "UPDATE");
}

bool SQLParser::IsDelete(const std::string& query) {
    return StringUtils::StartsWith(StringUtils::ToUpper(StringUtils::Trim(query)), "DELETE");
}

bool SQLParser::IsDDL(const std::string& query) {
    std::string upper = StringUtils::ToUpper(StringUtils::Trim(query));
    return StringUtils::StartsWith(upper, "CREATE") ||
           StringUtils::StartsWith(upper, "ALTER") ||
           StringUtils::StartsWith(upper, "DROP") ||
           StringUtils::StartsWith(upper, "TRUNCATE") ||
           StringUtils::StartsWith(upper, "RENAME");
}

std::optional<std::string> SQLParser::ExtractTableName(const std::string& query) {
    std::string upper = StringUtils::ToUpper(query);
    
    // Handle different statement types
    if (IsSelect(query)) {
        std::regex from_regex("FROM\\s+([a-zA-Z_][a-zA-Z0-9_]*(?:\\.[a-zA-Z_][a-zA-Z0-9_]*)?)");
        std::smatch match;
        if (std::regex_search(query, match, from_regex)) {
            return match[1].str();
        }
    } else if (IsInsert(query)) {
        std::regex into_regex("INSERT\\s+INTO\\s+([a-zA-Z_][a-zA-Z0-9_]*(?:\\.[a-zA-Z_][a-zA-Z0-9_]*)?)");
        std::smatch match;
        if (std::regex_search(query, match, into_regex)) {
            return match[1].str();
        }
    } else if (IsUpdate(query)) {
        std::regex update_regex("UPDATE\\s+([a-zA-Z_][a-zA-Z0-9_]*(?:\\.[a-zA-Z_][a-zA-Z0-9_]*)?)");
        std::smatch match;
        if (std::regex_search(query, match, update_regex)) {
            return match[1].str();
        }
    } else if (IsDelete(query)) {
        std::regex delete_regex("DELETE\\s+FROM\\s+([a-zA-Z_][a-zA-Z0-9_]*(?:\\.[a-zA-Z_][a-zA-Z0-9_]*)?)");
        std::smatch match;
        if (std::regex_search(query, match, delete_regex)) {
            return match[1].str();
        }
    }
    
    return std::nullopt;
}

} // namespace gh_ost