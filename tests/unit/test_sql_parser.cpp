/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "sql/parser.hpp"
#include "utils/string_utils.hpp"

TEST_CASE("AlterTableParser::Parse basic", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    bool result = parser.Parse("ALTER TABLE my_table ADD COLUMN new_col INT");
    REQUIRE(result);
    REQUIRE(parser.IsValid());
    REQUIRE(parser.GetTableName() == "my_table");
}

TEST_CASE("AlterTableParser::Parse with database", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    bool result = parser.Parse("ALTER TABLE my_db.my_table ADD COLUMN col VARCHAR(255)");
    REQUIRE(result);
    REQUIRE(parser.GetTableName() == "my_table");
    REQUIRE(parser.GetDatabaseName());
    REQUIRE(*parser.GetDatabaseName() == "my_db");
}

TEST_CASE("AlterTableParser::Parse multiple operations", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    bool result = parser.Parse(
        "ALTER TABLE my_table "
        "ADD COLUMN new_col INT, "
        "DROP COLUMN old_col, "
        "ADD INDEX idx_name (col1, col2)"
    );
    REQUIRE(result);
    
    auto ops = parser.GetOperations();
    REQUIRE(ops.size() >= 3);
}

TEST_CASE("AlterTableParser::Parse ADD COLUMN", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t ADD COLUMN col INT NOT NULL");
    
    auto add_ops = parser.GetAlterStatement().GetAddColumnOps();
    REQUIRE(!add_ops.empty());
    REQUIRE(add_ops[0].name == "col");
}

TEST_CASE("AlterTableParser::Parse DROP COLUMN", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t DROP COLUMN old_col");
    
    auto drop_ops = parser.GetAlterStatement().GetDropColumnOps();
    REQUIRE(!drop_ops.empty());
    REQUIRE(drop_ops[0].name == "old_col");
}

TEST_CASE("AlterTableParser::Parse MODIFY COLUMN", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t MODIFY COLUMN col VARCHAR(100) NOT NULL");
    
    auto alter = parser.GetAlterStatement();
    REQUIRE(alter.HasModifyColumn());
}

TEST_CASE("AlterTableParser::Parse ADD INDEX", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t ADD INDEX idx_name (col1, col2)");
    
    auto ops = parser.GetAlterStatement().GetIndexOps();
    REQUIRE(!ops.empty());
}

TEST_CASE("AlterTableParser::Parse ENGINE change", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t ENGINE = InnoDB");
    
    auto alter = parser.GetAlterStatement();
    REQUIRE(alter.HasEngineChange());
}

TEST_CASE("AlterTableParser::Reject RENAME TABLE", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t RENAME TO new_t");
    
    REQUIRE(parser.HasUnsupportedOperation());
    REQUIRE(parser.HasRenameTable());
}

TEST_CASE("AlterTableParser::Parse with FIRST", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t ADD COLUMN new_col INT FIRST");
    
    auto ops = parser.GetAlterStatement().GetAddColumnOps();
    REQUIRE(!ops.empty());
    REQUIRE(ops[0].is_first);
}

TEST_CASE("AlterTableParser::Parse with AFTER", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("ALTER TABLE t ADD COLUMN new_col INT AFTER existing_col");
    
    auto ops = parser.GetAlterStatement().GetAddColumnOps();
    REQUIRE(!ops.empty());
    REQUIRE(ops[0].after_column);
    REQUIRE(*ops[0].after_column == "existing_col");
}

TEST_CASE("AlterTableParser::Normalize", "[sql_parser]") {
    gh_ost::AlterTableParser parser;
    
    parser.Parse("  ALTER   TABLE   my_table   ADD   COLUMN   col   INT  ");
    
    std::string normalized = parser.Normalize();
    REQUIRE(gh_ost::StringUtils::Contains(normalized, "ALTER TABLE"));
}

TEST_CASE("SQLParser::IsSelect/Insert/Update/Delete", "[sql_parser]") {
    REQUIRE(gh_ost::SQLParser::IsSelect("SELECT * FROM table"));
    REQUIRE(gh_ost::SQLParser::IsInsert("INSERT INTO table VALUES (1)"));
    REQUIRE(gh_ost::SQLParser::IsUpdate("UPDATE table SET col = 1"));
    REQUIRE(gh_ost::SQLParser::IsDelete("DELETE FROM table WHERE id = 1"));
    REQUIRE(gh_ost::SQLParser::IsDDL("CREATE TABLE t (id INT)"));
    REQUIRE(gh_ost::SQLParser::IsDDL("ALTER TABLE t ADD COLUMN col INT"));
    REQUIRE(gh_ost::SQLParser::IsDDL("DROP TABLE t"));
}

TEST_CASE("SQLParser::ExtractTableName", "[sql_parser]") {
    auto table = gh_ost::SQLParser::ExtractTableName("SELECT * FROM my_table");
    REQUIRE(table);
    REQUIRE(*table == "my_table");
    
    table = gh_ost::SQLParser::ExtractTableName("INSERT INTO my_table VALUES (1)");
    REQUIRE(table);
    REQUIRE(*table == "my_table");
    
    table = gh_ost::SQLParser::ExtractTableName("UPDATE my_table SET col = 1");
    REQUIRE(table);
    REQUIRE(*table == "my_table");
}