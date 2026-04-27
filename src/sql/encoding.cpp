/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "sql/encoding.hpp"
#include "utils/string_utils.hpp"
#include <sstream>
#include <cstdint>

namespace gh_ost {

// Static constants
const std::string Encoding::UTF8 = "utf8";
const std::string Encoding::UTF8MB4 = "utf8mb4";
const std::string Encoding::LATIN1 = "latin1";
const std::string Encoding::BINARY = "binary";

std::string Encoding::EncodeForUTF8(const std::string& str) {
    // Assume input is valid UTF-8; just validate
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        if (c < 0x80) {
            result += c;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (i + 1 < str.length()) {
                result += c;
                result += str[++i];
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (i + 2 < str.length()) {
                result += c;
                result += str[++i];
                result += str[++i];
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            if (i + 3 < str.length()) {
                result += c;
                result += str[++i];
                result += str[++i];
                result += str[++i];
            }
        }
    }
    return result;
}

std::string Encoding::DecodeFromUTF8(const std::string& str) {
    // Return as-is for now
    return str;
}

bool Encoding::IsValidUTF8(const std::string& str) {
    size_t i = 0;
    while (i < str.length()) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        if (c < 0x80) {
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (i + 1 >= str.length()) return false;
            unsigned char c2 = static_cast<unsigned char>(str[i + 1]);
            if ((c2 & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (i + 2 >= str.length()) return false;
            unsigned char c2 = static_cast<unsigned char>(str[i + 1]);
            unsigned char c3 = static_cast<unsigned char>(str[i + 2]);
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            if (i + 3 >= str.length()) return false;
            unsigned char c2 = static_cast<unsigned char>(str[i + 1]);
            unsigned char c3 = static_cast<unsigned char>(str[i + 2]);
            unsigned char c4 = static_cast<unsigned char>(str[i + 3]);
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

std::optional<uint16_t> Encoding::GetCharsetId(const std::string& charset_name) {
    std::string lower = StringUtils::ToLower(charset_name);
    
    if (lower == "utf8mb4") return MySQLEncoding::UTF8MB4_CS;
    if (lower == "utf8") return MySQLEncoding::UTF8_CS;
    if (lower == "latin1") return MySQLEncoding::LATIN1_CS;
    if (lower == "binary") return MySQLEncoding::BINARY_CS;
    
    return std::nullopt;
}

std::string Encoding::GetCharsetById(uint16_t charset_id) {
    switch (charset_id) {
        case MySQLEncoding::UTF8MB4_CS: return UTF8MB4;
        case MySQLEncoding::UTF8_CS: return UTF8;
        case MySQLEncoding::LATIN1_CS: return LATIN1;
        case MySQLEncoding::BINARY_CS: return BINARY;
        default: return "";
    }
}

std::string Encoding::DefaultCollationForCharset(const std::string& charset) {
    std::string lower = StringUtils::ToLower(charset);
    
    if (lower == "utf8mb4") return "utf8mb4_general_ci";
    if (lower == "utf8") return "utf8_general_ci";
    if (lower == "latin1") return "latin1_swedish_ci";
    if (lower == "binary") return "binary";
    
    return "";
}

std::string Encoding::EncodeString(const std::string& str) {
    return StringUtils::EscapeForSQL(str);
}

std::string Encoding::DecodeString(const std::string& encoded) {
    return StringUtils::UnescapeFromJSON(encoded);
}

std::vector<uint8_t> Encoding::StringToBytes(const std::string& str) {
    std::vector<uint8_t> bytes;
    for (char c : str) {
        bytes.push_back(static_cast<uint8_t>(c));
    }
    return bytes;
}

std::string Encoding::BytesToString(const std::vector<uint8_t>& bytes) {
    std::string str;
    for (uint8_t b : bytes) {
        str.push_back(static_cast<char>(b));
    }
    return str;
}

} // namespace gh_ost