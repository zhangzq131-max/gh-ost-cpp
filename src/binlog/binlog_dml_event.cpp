/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "binlog/binlog_dml_event.hpp"
#include "utils/string_utils.hpp"
#include "utils/logger.hpp"
#include <sstream>

namespace gh_ost {

// BinlogRowData methods
std::optional<std::string> BinlogRowData::GetValue(const std::string& column_name) const {
    return GetAfterValue(column_name);
}

std::optional<std::string> BinlogRowData::GetBeforeValue(const std::string& column_name) const {
    for (size_t i = 0; i < column_names.size(); ++i) {
        if (column_names[i] == column_name) {
            if (i < before_values.size()) return before_values[i];
        }
    }
    return std::nullopt;
}

std::optional<std::string> BinlogRowData::GetAfterValue(const std::string& column_name) const {
    for (size_t i = 0; i < column_names.size(); ++i) {
        if (column_names[i] == column_name) {
            if (i < after_values.size()) return after_values[i];
        }
    }
    return std::nullopt;
}

std::string BinlogRowData::ToInsertValuesSQL() const {
    std::ostringstream oss;
    oss << "(";
    
    for (size_t i = 0; i < after_values.size(); ++i) {
        if (after_values[i]) {
            oss << "'" << StringUtils::EscapeForSQL(*after_values[i]) << "'";
        } else {
            oss << "NULL";
        }
        if (i < after_values.size() - 1) oss << ", ";
    }
    
    oss << ")";
    return oss.str();
}

std::string BinlogRowData::ToUpdateSQL(const std::string& key_column) const {
    std::ostringstream oss;
    oss << "SET ";
    
    size_t update_count = 0;
    for (size_t i = 0; i < column_names.size(); ++i) {
        if (column_names[i] == key_column) continue;  // Skip key column
        
        if (update_count > 0) oss << ", ";
        
        oss << StringUtils::QuoteIdentifier(column_names[i]) << " = ";
        
        if (i < after_values.size() && after_values[i]) {
            oss << "'" << StringUtils::EscapeForSQL(*after_values[i]) << "'";
        } else {
            oss << "NULL";
        }
        update_count++;
    }
    
    oss << " WHERE " << StringUtils::QuoteIdentifier(key_column) << " = ";
    if (pk_value) {
        oss << "'" << StringUtils::EscapeForSQL(*pk_value) << "'";
    } else {
        oss << "NULL";
    }
    
    return oss.str();
}

// BinlogDMLEvent implementation
BinlogDMLEvent::BinlogDMLEvent()
    : dml_type_(DMLType::Insert)
    , timestamp_(0)
    , table_id_(0)
    , transaction_id_(0) {}

BinlogDMLEvent::~BinlogDMLEvent() {}

std::string BinlogDMLEvent::ToSQL() const {
    auto statements = ToSQLStatements();
    if (statements.empty()) return "";
    return statements[0];
}

std::vector<std::string> BinlogDMLEvent::ToSQLStatements() const {
    std::vector<std::string> statements;
    
    for (const auto& row : rows_) {
        std::string sql;
        
        switch (dml_type_) {
            case DMLType::Insert:
                sql = GenerateInsertSQL(row);
                break;
            case DMLType::Update:
                sql = GenerateUpdateSQL(row);
                break;
            case DMLType::Delete:
                sql = GenerateDeleteSQL(row);
                break;
        }
        
        if (!sql.empty()) {
            statements.push_back(sql);
        }
    }
    
    return statements;
}

std::string BinlogDMLEvent::GenerateInsertSQL(const BinlogRowData& row) const {
    std::ostringstream oss;
    oss << "INSERT INTO " << StringUtils::QuoteIdentifier(database_) 
        << "." << StringUtils::QuoteIdentifier(table_);
    
    // Column names
    oss << " (" << StringUtils::Join(row.column_names, 
        [](const std::string& col) { return StringUtils::QuoteIdentifier(col); },
        ", ") << ")";
    
    // Values
    oss << " VALUES " << row.ToInsertValuesSQL();
    
    return oss.str();
}

std::string BinlogDMLEvent::GenerateUpdateSQL(const BinlogRowData& row) const {
    std::ostringstream oss;
    oss << "UPDATE " << StringUtils::QuoteIdentifier(database_) 
        << "." << StringUtils::QuoteIdentifier(table_);
    
    oss << " " << row.ToUpdateSQL(row.pk_column);
    
    return oss.str();
}

std::string BinlogDMLEvent::GenerateDeleteSQL(const BinlogRowData& row) const {
    std::ostringstream oss;
    oss << "DELETE FROM " << StringUtils::QuoteIdentifier(database_) 
        << "." << StringUtils::QuoteIdentifier(table_);
    
    oss << " WHERE " << StringUtils::QuoteIdentifier(row.pk_column) << " = ";
    if (row.pk_value) {
        oss << "'" << StringUtils::EscapeForSQL(*row.pk_value) << "'";
    } else {
        oss << "NULL";
    }
    
    return oss.str();
}

} // namespace gh_ost