/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_BINLOG_DML_EVENT_HPP
#define GH_OST_BINLOG_DML_EVENT_HPP

#include "gh_ost/types.hpp"
#include "sql/types.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace gh_ost {

// Row data in a binlog DML event
struct BinlogRowData {
    // Column values before change (for UPDATE/DELETE)
    std::vector<std::optional<std::string>> before_values;
    
    // Column values after change (for INSERT/UPDATE)
    std::vector<std::optional<std::string>> after_values;
    
    // Column names (populated by looking up table schema)
    std::vector<std::string> column_names;
    
    // Primary key column name and value
    std::string pk_column;
    std::optional<std::string> pk_value;
    
    // Get value by column name
    std::optional<std::string> GetValue(const std::string& column_name) const;
    std::optional<std::string> GetBeforeValue(const std::string& column_name) const;
    std::optional<std::string> GetAfterValue(const std::string& column_name) const;
    
    // Get all column values as SQL insert format
    std::string ToInsertValuesSQL() const;
    std::string ToUpdateSQL(const std::string& key_column) const;
};

// DML event from binlog (INSERT, UPDATE, DELETE)
class BinlogDMLEvent {
public:
    BinlogDMLEvent();
    ~BinlogDMLEvent();
    
    // Database and table
    std::string Database() const { return database_; }
    void SetDatabase(const std::string& db) { database_ = db; }
    
    std::string Table() const { return table_; }
    void SetTable(const std::string& tbl) { table_ = tbl; }
    
    // DML operation type
    DMLType DML() const { return dml_type_; }
    void SetDML(DMLType type) { dml_type_ = type; }
    
    // Event timestamp
    uint64_t Timestamp() const { return timestamp_; }
    void SetTimestamp(uint64_t ts) { timestamp_ = ts; }
    
    // Rows affected
    size_t RowCount() const { return rows_.size(); }
    
    // Get rows
    const std::vector<BinlogRowData>& Rows() const { return rows_; }
    void AddRow(const BinlogRowData& row) { rows_.push_back(row); }
    
    // Map ID (table identifier in binlog)
    uint64_t TableId() const { return table_id_; }
    void SetTableId(uint64_t id) { table_id_ = id; }
    
    // Transaction boundaries
    uint64_t TransactionId() const { return transaction_id_; }
    void SetTransactionId(uint64_t id) { transaction_id_ = id; }
    
    // Convert to SQL statements
    std::string ToSQL() const;
    std::vector<std::string> ToSQLStatements() const;
    
    // Convert to function that applies changes to ghost table
    using ApplyFunction = std::function<void(const std::string& database, 
                                             const std::string& table,
                                             DMLType type,
                                             const BinlogRowData& row)>;
    
private:
    std::string database_;
    std::string table_;
    DMLType dml_type_;
    uint64_t timestamp_;
    uint64_t table_id_;
    uint64_t transaction_id_;
    std::vector<BinlogRowData> rows_;
    
    // Helper methods for SQL generation
    std::string GenerateInsertSQL(const BinlogRowData& row) const;
    std::string GenerateUpdateSQL(const BinlogRowData& row) const;
    std::string GenerateDeleteSQL(const BinlogRowData& row) const;
};

} // namespace gh_ost

#endif // GH_OST_BINLOG_DML_EVENT_HPP