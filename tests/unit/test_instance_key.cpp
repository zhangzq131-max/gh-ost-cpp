/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "mysql/instance_key.hpp"

TEST_CASE("InstanceKey::Basic", "[instance_key]") {
    gh_ost::InstanceKey key("localhost", 3306);
    
    REQUIRE(key.Host() == "localhost");
    REQUIRE(key.Port() == 3306);
    REQUIRE(key.IsValid());
}

TEST_CASE("InstanceKey::ToString", "[instance_key]") {
    gh_ost::InstanceKey key("192.168.1.1", 3307);
    
    REQUIRE(key.ToString() == "192.168.1.1:3307");
}

TEST_CASE("InstanceKey::Parse", "[instance_key]") {
    auto parsed = gh_ost::InstanceKey::Parse("localhost:3306");
    REQUIRE(parsed);
    REQUIRE(parsed->Host() == "localhost");
    REQUIRE(parsed->Port() == 3306);
    
    parsed = gh_ost::InstanceKey::Parse("invalid");
    REQUIRE(!parsed);
    
    parsed = gh_ost::InstanceKey::Parse("localhost:abc");
    REQUIRE(!parsed);
}

TEST_CASE("InstanceKey::Equality", "[instance_key]") {
    gh_ost::InstanceKey key1("localhost", 3306);
    gh_ost::InstanceKey key2("localhost", 3306);
    gh_ost::InstanceKey key3("localhost", 3307);
    gh_ost::InstanceKey key4("otherhost", 3306);
    
    REQUIRE(key1 == key2);
    REQUIRE(key1 != key3);
    REQUIRE(key1 != key4);
}

TEST_CASE("InstanceKey::Ordering", "[instance_key]") {
    gh_ost::InstanceKey key1("a", 3306);
    gh_ost::InstanceKey key2("b", 3306);
    gh_ost::InstanceKey key3("a", 3307);
    
    REQUIRE(key1 < key2);  // Different hosts
    REQUIRE(key1 < key3);  // Same host, different ports
}

TEST_CASE("InstanceKey::Empty", "[instance_key]") {
    gh_ost::InstanceKey empty = gh_ost::InstanceKey::Empty();
    
    REQUIRE(!empty.IsValid());
    REQUIRE(empty.ToString() == "");
}

TEST_CASE("InstanceKey::Localhost", "[instance_key]") {
    gh_ost::InstanceKey local = gh_ost::InstanceKey::Localhost(3306);
    
    REQUIRE(local.Host() == "127.0.0.1");
    REQUIRE(local.Port() == 3306);
    REQUIRE(local.IsValid());
}

TEST_CASE("InstanceKeyMap::AddRemove", "[instance_key]") {
    gh_ost::InstanceKeyMap map;
    
    gh_ost::InstanceKey key1("host1", 3306);
    gh_ost::InstanceKey key2("host2", 3307);
    
    map.Add(key1);
    REQUIRE(map.Size() == 1);
    REQUIRE(map.Contains(key1));
    
    map.Add(key2);
    REQUIRE(map.Size() == 2);
    
    map.Add(key1);  // Duplicate
    REQUIRE(map.Size() == 2);  // Should not increase
    
    map.Remove(key1);
    REQUIRE(map.Size() == 1);
    REQUIRE(!map.Contains(key1));
}

TEST_CASE("InstanceKeyMap::FindByHost", "[instance_key]") {
    gh_ost::InstanceKeyMap map;
    
    gh_ost::InstanceKey key1("host1", 3306);
    gh_ost::InstanceKey key2("host2", 3307);
    
    map.Add(key1);
    map.Add(key2);
    
    auto found = map.FindByHost("host1");
    REQUIRE(found);
    REQUIRE(found->Port() == 3306);
    
    found = map.FindByHost("unknown");
    REQUIRE(!found);
}