/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_INSPECTOR_HPP
#define GH_OST_INSPECTOR_HPP

#include "context/migration_context.hpp"
#include "mysql/connection.hpp"
#include <memory>
#include <string>

namespace gh_ost {

// Table structure inspector
class Inspector {
public:
    Inspector(MigrationContext& context);
    ~Inspector();
    
    // Initialize connections
    bool Initialize();
    
    // Detect topology (master/replica)
    bool DetectMaster();
    bool DetectReplica();
    
    // Table inspection
    bool InspectOriginalTable();
    bool InspectGhostTable();
    bool CompareTables();
    
    // Structure validation
    bool ValidateOriginalTable();
    bool ValidateGhostTable();
    
    // Get shared unique key for migration
    bool DetermineSharedUniqueKey();
    
    // Get shared columns
    bool DetermineColumnPairs();
    
    // Row range
    bool ReadRowRange();
    
    // Validate migration is possible
    ValidationResult ValidateMigration();
    
    // Check privileges
    bool CheckPrivileges();
    
    // Table exists checks
    bool OriginalTableExists();
    bool GhostTableExists();
    bool ChangelogTableExists();
    
    // Get estimated row count
    uint64_t GetEstimatedRowCount();
    uint64_t GetExactRowCount();
    
private:
    MigrationContext& context_;
    std::shared_ptr<Connection> inspector_connection_;
    
    // Helper methods
    TableStructure ReadTableStructure(const std::string& database, 
                                      const std::string& table);
    
    KeyInfo FindBestUniqueKey(const TableStructure& original,
                              const TableStructure& ghost);
    
    bool ColumnsMatch(const ColumnInfo& col1, const ColumnInfo& col2);
    
    bool HasForeignKeys(const TableStructure& structure);
    
    std::string GetColumnTypeSQL(const ColumnInfo& col);
};

} // namespace gh_ost

#endif // GH_OST_INSPECTOR_HPP