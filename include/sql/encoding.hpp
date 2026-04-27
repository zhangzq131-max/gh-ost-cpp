/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_SQL_ENCODING_HPP
#define GH_OST_SQL_ENCODING_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace gh_ost {

// MySQL encoding utilities
class Encoding {
public:
    // Character set names
    static const std::string UTF8;
    static const std::string UTF8MB4;
    static const std::string LATIN1;
    static const std::string BINARY;
    
    // Encoding conversion
    static std::string EncodeForUTF8(const std::string& str);
    static std::string DecodeFromUTF8(const std::string& str);
    
    // Check if string is valid UTF-8
    static bool IsValidUTF8(const std::string& str);
    
    // Get character set ID by name
    static std::optional<uint16_t> GetCharsetId(const std::string& charset_name);
    static std::string GetCharsetById(uint16_t charset_id);
    
    // Collation handling
    static std::string DefaultCollationForCharset(const std::string& charset);
    
    // Value encoding for MySQL
    static std::string EncodeString(const std::string& str);
    static std::string DecodeString(const std::string& encoded);
    
    // Binary data handling
    static std::vector<uint8_t> StringToBytes(const std::string& str);
    static std::string BytesToString(const std::vector<uint8_t>& bytes);
    
private:
    Encoding() = delete;
};

// MySQL string types and encoding constants
namespace MySQLEncoding {
    // Character set IDs (from MySQL charset_number)
    constexpr uint16_t UTF8MB4_CS = 255;
    constexpr uint16_t UTF8_CS = 33;
    constexpr uint16_t LATIN1_CS = 8;
    constexpr uint16_t BINARY_CS = 63;
    
    // Column type IDs (from MySQL protocol)
    constexpr uint8_t TYPE_DECIMAL = 0;
    constexpr uint8_t TYPE_TINY = 1;
    constexpr uint8_t TYPE_SHORT = 2;
    constexpr uint8_t TYPE_LONG = 3;
    constexpr uint8_t TYPE_FLOAT = 4;
    constexpr uint8_t TYPE_DOUBLE = 5;
    constexpr uint8_t TYPE_NULL = 6;
    constexpr uint8_t TYPE_TIMESTAMP = 7;
    constexpr uint8_t TYPE_LONGLONG = 8;
    constexpr uint8_t TYPE_INT24 = 9;
    constexpr uint8_t TYPE_DATE = 10;
    constexpr uint8_t TYPE_TIME = 11;
    constexpr uint8_t TYPE_DATETIME = 12;
    constexpr uint8_t TYPE_YEAR = 13;
    constexpr uint8_t TYPE_NEWDATE = 14;
    constexpr uint8_t TYPE_VARCHAR = 15;
    constexpr uint8_t TYPE_BIT = 16;
    constexpr uint8_t TYPE_NEWDECIMAL = 246;
    constexpr uint8_t TYPE_ENUM = 247;
    constexpr uint8_t TYPE_SET = 248;
    constexpr uint8_t TYPE_TINY_BLOB = 249;
    constexpr uint8_t TYPE_MEDIUM_BLOB = 250;
    constexpr uint8_t TYPE_LONG_BLOB = 251;
    constexpr uint8_t TYPE_BLOB = 252;
    constexpr uint8_t TYPE_VAR_STRING = 253;
    constexpr uint8_t TYPE_STRING = 254;
    constexpr uint8_t TYPE_GEOMETRY = 255;
}

} // namespace gh_ost

#endif // GH_OST_SQL_ENCODING_HPP