/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "binlog/binlog_entry.hpp"
#include "binlog/binlog_dml_event.hpp"
#include "binlog/binlog_coordinates.hpp"
#include "utils/string_utils.hpp"

TEST_CASE("BinlogCoordinates::Basic", "[binlog]") {
    gh_ost::BinlogCoordinates coords("mysql-bin.000123", 456);
    
    REQUIRE(coords.File() == "mysql-bin.000123");
    REQUIRE(coords.Position() == 456);
    REQUIRE(coords.IsValid());
}

TEST_CASE("BinlogCoordinates::ToString", "[binlog]") {
    gh_ost::BinlogCoordinates coords("mysql-bin.000123", 456);
    
    REQUIRE(coords.ToString() == "mysql-bin.000123:456");
}

TEST_CASE("BinlogCoordinates::Parse", "[binlog]") {
    auto parsed = gh_ost::BinlogCoordinates::Parse("mysql-bin.000123:456");
    REQUIRE(parsed);
    REQUIRE(parsed->File() == "mysql-bin.000123");
    REQUIRE(parsed->Position() == 456);
    
    parsed = gh_ost::BinlogCoordinates::Parse("invalid");
    REQUIRE(!parsed);
}

TEST_CASE("BinlogCoordinates::GetSequence", "[binlog]") {
    gh_ost::BinlogCoordinates coords("mysql-bin.000123", 456);
    
    REQUIRE(coords.GetSequence() == 123);
    
    gh_ost::BinlogCoordinates coords2("binlog.00001", 100);
    REQUIRE(coords2.GetSequence() == 1);
}

TEST_CASE("BinlogCoordinates::Comparison", "[binlog]") {
    gh_ost::BinlogCoordinates coords1("mysql-bin.000123", 100);
    gh_ost::BinlogCoordinates coords2("mysql-bin.000123", 200);
    gh_ost::BinlogCoordinates coords3("mysql-bin.000124", 100);
    
    REQUIRE(coords1 < coords2);  // Same file, different position
    REQUIRE(coords1 < coords3);  // Different file (higher sequence)
    REQUIRE(coords1 != coords2);
    REQUIRE(coords1 == gh_ost::BinlogCoordinates("mysql-bin.000123", 100));
}

TEST_CASE("BinlogDMLEvent::Basic", "[binlog]") {
    gh_ost::BinlogDMLEvent event;
    
    event.SetDatabase("test_db");
    event.SetTable("test_table");
    event.SetDML(gh_ost::DMLType::Insert);
    
    REQUIRE(event.Database() == "test_db");
    REQUIRE(event.Table() == "test_table");
    REQUIRE(event.DML() == gh_ost::DMLType::Insert);
}

TEST_CASE("BinlogRowData::GetValue", "[binlog]") {
    gh_ost::BinlogRowData row;
    
    row.column_names = {"id", "name", "value"};
    row.after_values = {"1", "test", "100"};
    
    auto id = row.GetValue("id");
    REQUIRE(id);
    REQUIRE(*id == "1");
    
    auto name = row.GetValue("name");
    REQUIRE(name);
    REQUIRE(*name == "test");
    
    auto unknown = row.GetValue("unknown");
    REQUIRE(!unknown);
}

TEST_CASE("BinlogEntry::IsDML", "[binlog]") {
    gh_ost::BinlogEntry entry;
    
    entry.SetEventType(gh_ost::BinlogEventType::WriteRows);
    REQUIRE(entry.IsDML());
    
    entry.SetEventType(gh_ost::BinlogEventType::UpdateRows);
    REQUIRE(entry.IsDML());
    
    entry.SetEventType(gh_ost::BinlogEventType::DeleteRows);
    REQUIRE(entry.IsDML());
    
    entry.SetEventType(gh_ost::BinlogEventType::Query);
    REQUIRE(!entry.IsDML());
}

TEST_CASE("BinlogEntry::IsForTable", "[binlog]") {
    gh_ost::BinlogEntry entry;
    
    auto dml = std::make_shared<gh_ost::BinlogDMLEvent>();
    dml->SetDatabase("test_db");
    dml->SetTable("test_table");
    
    entry.SetDmlEvent(dml);
    
    REQUIRE(entry.IsForTable("test_db", "test_table"));
    REQUIRE(!entry.IsForTable("other_db", "test_table"));
    REQUIRE(!entry.IsForTable("test_db", "other_table"));
}

TEST_CASE("GTIDCoordinates::Basic", "[binlog]") {
    gh_ost::GTIDCoordinates gtid("3E11FA47-71CA-11E1-9E33-C80AA9429562", 123);
    
    REQUIRE(gtid.Domain() == "3E11FA47-71CA-11E1-9E33-C80AA9429562");
    REQUIRE(gtid.Sequence() == 123);
    REQUIRE(gtid.IsValid());
}

TEST_CASE("GTIDCoordinates::ToString", "[binlog]") {
    gh_ost::GTIDCoordinates gtid("3E11FA47-71CA-11E1-9E33-C80AA9429562", 123);
    
    REQUIRE(gtid.ToString() == "3E11FA47-71CA-11E1-9E33-C80AA9429562:123");
}

TEST_CASE("GTIDSet::Add", "[binlog]") {
    gh_ost::GTIDSet set;
    
    set.Add("domain1", 1);
    set.Add("domain1", 2);  // Same domain, higher sequence
    set.Add("domain2", 5);
    
    REQUIRE(!set.IsEmpty());
    
    std::string str = set.ToString();
    REQUIRE(gh_ost::StringUtils::Contains(str, "domain1:2"));  // Updated
    REQUIRE(gh_ost::StringUtils::Contains(str, "domain2:5"));
}