/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

TEST_CASE("StringUtils::Trim", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::Trim("  hello  ") == "hello");
    REQUIRE(gh_ost::StringUtils::Trim("\t\nhello\n\t") == "hello");
    REQUIRE(gh_ost::StringUtils::Trim("") == "");
    REQUIRE(gh_ost::StringUtils::Trim("   ") == "");
}

TEST_CASE("StringUtils::ToUpper/Lower", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::ToUpper("hello") == "HELLO");
    REQUIRE(gh_ost::StringUtils::ToLower("HELLO") == "hello");
    REQUIRE(gh_ost::StringUtils::ToUpper("HeLLo") == "HELLO");
    REQUIRE(gh_ost::StringUtils::ToLower("HeLLo") == "hello");
}

TEST_CASE("StringUtils::Split", "[string_utils]") {
    auto parts = gh_ost::StringUtils::Split("a,b,c", ',');
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
}

TEST_CASE("StringUtils::Join", "[string_utils]") {
    std::vector<std::string> parts = {"a", "b", "c"};
    REQUIRE(gh_ost::StringUtils::Join(parts, ",") == "a,b,c");
    REQUIRE(gh_ost::StringUtils::Join(parts, " ") == "a b c");
}

TEST_CASE("StringUtils::Contains", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::Contains("hello world", "world"));
    REQUIRE(!gh_ost::StringUtils::Contains("hello world", "xyz"));
    REQUIRE(gh_ost::StringUtils::Contains("hello", 'l'));
}

TEST_CASE("StringUtils::StartsWith/EndsWith", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::StartsWith("hello world", "hello"));
    REQUIRE(!gh_ost::StringUtils::StartsWith("hello world", "world"));
    REQUIRE(gh_ost::StringUtils::EndsWith("hello world", "world"));
    REQUIRE(!gh_ost::StringUtils::EndsWith("hello world", "hello"));
}

TEST_CASE("StringUtils::Replace", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::ReplaceAll("hello world", "o", "0") == "hell0 w0rld");
    REQUIRE(gh_ost::StringUtils::ReplaceFirst("hello hello", "hello", "hi") == "hi hello");
}

TEST_CASE("StringUtils::ParseInt", "[string_utils]") {
    auto val = gh_ost::StringUtils::ParseInt64("12345");
    REQUIRE(val);
    REQUIRE(*val == 12345);
    
    val = gh_ost::StringUtils::ParseInt64("-12345");
    REQUIRE(val);
    REQUIRE(*val == -12345);
    
    val = gh_ost::StringUtils::ParseInt64("abc");
    REQUIRE(!val);
}

TEST_CASE("StringUtils::EscapeForSQL", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::EscapeForSQL("hello") == "hello");
    REQUIRE(gh_ost::StringUtils::EscapeForSQL("it's") == "it''s");
    REQUIRE(gh_ost::StringUtils::EscapeForSQL("hello\nworld") == "hello\\nworld");
}

TEST_CASE("StringUtils::QuoteIdentifier", "[string_utils]") {
    REQUIRE(gh_ost::StringUtils::QuoteIdentifier("table") == "`table`");
    REQUIRE(gh_ost::StringUtils::QuoteIdentifier("table`name") == "`table``name`");
}