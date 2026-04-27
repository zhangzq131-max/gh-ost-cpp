/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "utils/string_utils.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <charconv>
#include <stdexcept>

namespace gh_ost {

// Trim functions
std::string StringUtils::Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::string StringUtils::TrimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    return str.substr(start);
}

std::string StringUtils::TrimRight(const std::string& str) {
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos) return "";
    return str.substr(0, end + 1);
}

std::string StringUtils::Trim(const std::string& str, char ch) {
    size_t start = str.find_first_not_of(ch);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(ch);
    return str.substr(start, end - start + 1);
}

std::string StringUtils::Trim(const std::string& str, const std::string& chars) {
    size_t start = str.find_first_not_of(chars);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(chars);
    return str.substr(start, end - start + 1);
}

// Case conversion
std::string StringUtils::ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string StringUtils::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string StringUtils::Capitalize(const std::string& str) {
    if (str.empty()) return "";
    std::string result = str;
    result[0] = static_cast<char>(::toupper(result[0]));
    std::transform(result.begin() + 1, result.end(), result.begin() + 1, ::tolower);
    return result;
}

// Split functions
std::vector<std::string> StringUtils::Split(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(str);
    std::string part;
    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

std::vector<std::string> StringUtils::Split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> parts;
    if (delimiter.empty()) {
        for (char c : str) {
            parts.push_back(std::string(1, c));
        }
        return parts;
    }
    
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        parts.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    parts.push_back(str.substr(start));
    return parts;
}

std::vector<std::string> StringUtils::SplitLines(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line)) {
        // Remove trailing newline characters
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

// Join function
std::string StringUtils::Join(const std::vector<std::string>& parts, const std::string& delimiter) {
    if (parts.empty()) return "";
    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter + parts[i];
    }
    return result;
}

std::string StringUtils::Join(const std::vector<std::string_view>& parts, const std::string& delimiter) {
    if (parts.empty()) return "";
    std::string result(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter;
        result += parts[i];
    }
    return result;
}

// Contains and find functions
bool StringUtils::Contains(const std::string& str, const std::string& substring) {
    return str.find(substring) != std::string::npos;
}

bool StringUtils::Contains(const std::string& str, char ch) {
    return str.find(ch) != std::string::npos;
}

bool StringUtils::ContainsIgnoreCase(const std::string& str, const std::string& substring) {
    return FindIgnoreCase(str, substring) != std::string::npos;
}

size_t StringUtils::Find(const std::string& str, const std::string& substring) {
    return str.find(substring);
}

size_t StringUtils::FindIgnoreCase(const std::string& str, const std::string& substring) {
    std::string lower_str = ToLower(str);
    std::string lower_sub = ToLower(substring);
    return lower_str.find(lower_sub);
}

// Starts with / Ends with
bool StringUtils::StartsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) return false;
    return str.compare(0, prefix.length(), prefix) == 0;
}

bool StringUtils::StartsWithIgnoreCase(const std::string& str, const std::string& prefix) {
    return StartsWith(ToLower(str), ToLower(prefix));
}

bool StringUtils::EndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool StringUtils::EndsWithIgnoreCase(const std::string& str, const std::string& suffix) {
    return EndsWith(ToLower(str), ToLower(suffix));
}

// Replace functions
std::string StringUtils::Replace(const std::string& str, const std::string& from, const std::string& to) {
    return ReplaceFirst(str, from, to);
}

std::string StringUtils::ReplaceAll(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string StringUtils::ReplaceFirst(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result = str;
    size_t pos = result.find(from);
    if (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
    }
    return result;
}

std::string StringUtils::ReplaceLast(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result = str;
    size_t pos = result.rfind(from);
    if (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
    }
    return result;
}

// String manipulation
std::string StringUtils::PadLeft(const std::string& str, size_t length, char pad) {
    if (str.length() >= length) return str;
    return std::string(length - str.length(), pad) + str;
}

std::string StringUtils::PadRight(const std::string& str, size_t length, char pad) {
    if (str.length() >= length) return str;
    return str + std::string(length - str.length(), pad);
}

std::string StringUtils::Repeat(const std::string& str, size_t count) {
    if (count == 0 || str.empty()) return "";
    std::string result;
    for (size_t i = 0; i < count; ++i) {
        result += str;
    }
    return result;
}

// Substring
std::string StringUtils::Substring(const std::string& str, size_t start, size_t length) {
    if (start >= str.length()) return "";
    if (length == std::string::npos) return str.substr(start);
    return str.substr(start, length);
}

std::string StringUtils::Take(const std::string& str, size_t n) {
    return str.substr(0, std::min(n, str.length()));
}

std::string StringUtils::Drop(const std::string& str, size_t n) {
    if (n >= str.length()) return "";
    return str.substr(n);
}

// Is empty / blank
bool StringUtils::IsEmpty(const std::string& str) {
    return str.empty();
}

bool StringUtils::IsBlank(const std::string& str) {
    return Trim(str).empty();
}

// Number conversion
std::optional<int32_t> StringUtils::ParseInt32(const std::string& str) {
    try {
        size_t pos = 0;
        int32_t value = std::stoi(str, &pos);
        if (pos == str.length() || Trim(str.substr(pos)).empty()) {
            return value;
        }
    } catch (...) {}
    return std::nullopt;
}

std::optional<int64_t> StringUtils::ParseInt64(const std::string& str) {
    try {
        size_t pos = 0;
        int64_t value = std::stoll(str, &pos);
        if (pos == str.length() || Trim(str.substr(pos)).empty()) {
            return value;
        }
    } catch (...) {}
    return std::nullopt;
}

std::optional<uint32_t> StringUtils::ParseUInt32(const std::string& str) {
    try {
        size_t pos = 0;
        uint32_t value = static_cast<uint32_t>(std::stoul(str, &pos));
        if (pos == str.length() || Trim(str.substr(pos)).empty()) {
            return value;
        }
    } catch (...) {}
    return std::nullopt;
}

std::optional<uint64_t> StringUtils::ParseUInt64(const std::string& str) {
    try {
        size_t pos = 0;
        uint64_t value = std::stoull(str, &pos);
        if (pos == str.length() || Trim(str.substr(pos)).empty()) {
            return value;
        }
    } catch (...) {}
    return std::nullopt;
}

std::optional<double> StringUtils::ParseDouble(const std::string& str) {
    try {
        size_t pos = 0;
        double value = std::stod(str, &pos);
        if (pos == str.length() || Trim(str.substr(pos)).empty()) {
            return value;
        }
    } catch (...) {}
    return std::nullopt;
}

std::optional<float> StringUtils::ParseFloat(const std::string& str) {
    try {
        size_t pos = 0;
        float value = std::stof(str, &pos);
        if (pos == str.length() || Trim(str.substr(pos)).empty()) {
            return value;
        }
    } catch (...) {}
    return std::nullopt;
}

std::string StringUtils::ToString(int32_t value) {
    return std::to_string(value);
}

std::string StringUtils::ToString(int64_t value) {
    return std::to_string(value);
}

std::string StringUtils::ToString(uint32_t value) {
    return std::to_string(value);
}

std::string StringUtils::ToString(uint64_t value) {
    return std::to_string(value);
}

std::string StringUtils::ToString(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::string StringUtils::ToString(float value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::string StringUtils::ToString(bool value) {
    return value ? "true" : "false";
}

// Hex conversion
std::string StringUtils::ToHex(const std::string& str) {
    std::ostringstream oss;
    for (unsigned char c : str) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return oss.str();
}

std::string StringUtils::ToHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t c : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return oss.str();
}

std::string StringUtils::FromHex(const std::string& hex) {
    std::string result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoul(byte_str, nullptr, 16));
        result.push_back(byte);
    }
    return result;
}

std::vector<uint8_t> StringUtils::HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Escape/Unescape for SQL
std::string StringUtils::EscapeForSQL(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\'': result += "''"; break;
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\0': result += "\\0"; break;
            default:   result += c; break;
        }
    }
    return result;
}

std::string StringUtils::QuoteForSQL(const std::string& str, char quote) {
    std::string result(1, quote);
    result += EscapeForSQL(str);
    result += quote;
    return result;
}

std::string StringUtils::QuoteIdentifier(const std::string& identifier, char quote) {
    std::string result(1, quote);
    for (char c : identifier) {
        if (c == quote) {
            result += quote;
            result += quote;
        } else {
            result += c;
        }
    }
    result += quote;
    return result;
}

// Escape/Unescape for JSON
std::string StringUtils::EscapeForJSON(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    result += "\\u";
                    std::ostringstream oss;
                    oss << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    result += oss.str();
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

std::string StringUtils::UnescapeFromJSON(const std::string& str) {
    std::string result;
    size_t i = 0;
    while (i < str.length()) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            char next = str[i + 1];
            switch (next) {
                case '"':  result += '"'; i += 2; break;
                case '\\': result += '\\'; i += 2; break;
                case 'n':  result += '\n'; i += 2; break;
                case 'r':  result += '\r'; i += 2; break;
                case 't':  result += '\t'; i += 2; break;
                case 'b':  result += '\b'; i += 2; break;
                case 'f':  result += '\f'; i += 2; break;
                case 'u':
                    if (i + 5 < str.length()) {
                        std::string hex = str.substr(i + 2, 4);
                        try {
                            unsigned int code = std::stoul(hex, nullptr, 16);
                            result += static_cast<char>(code);
                        } catch (...) {
                            result += str[i];
                        }
                        i += 6;
                    } else {
                        result += str[i++];
                    }
                    break;
                default:
                    result += str[i++];
                    break;
            }
        } else {
            result += str[i++];
        }
    }
    return result;
}

// SQL identifier handling
std::string StringUtils::NormalizeTableName(const std::string& name) {
    std::string normalized = Trim(name);
    // Remove surrounding quotes if present
    if (normalized.length() >= 2) {
        char first = normalized[0];
        char last = normalized[normalized.length() - 1];
        if ((first == '\'' || first == '"' || first == '`') && first == last) {
            normalized = normalized.substr(1, normalized.length() - 2);
        }
    }
    return normalized;
}

std::string StringUtils::CombineDatabaseTable(const std::string& database, const std::string& table) {
    return QuoteIdentifier(database) + "." + QuoteIdentifier(table);
}

// Wildcard matching
bool StringUtils::MatchesWildcard(const std::string& str, const std::string& pattern) {
    // Simple wildcard matching with * and ?
    size_t s = 0, p = 0;
    size_t star_pos = std::string::npos;
    size_t match_pos = 0;
    
    while (s < str.length()) {
        if (p < pattern.length() && (pattern[p] == '?' || pattern[p] == str[s])) {
            s++;
            p++;
        } else if (p < pattern.length() && pattern[p] == '*') {
            star_pos = p;
            match_pos = s;
            p++;
        } else if (star_pos != std::string::npos) {
            p = star_pos + 1;
            match_pos++;
            s = match_pos;
        } else {
            return false;
        }
    }
    
    while (p < pattern.length() && pattern[p] == '*') {
        p++;
    }
    
    return p == pattern.length();
}

// Remove comments from SQL
std::string StringUtils::RemoveSQLComments(const std::string& sql) {
    std::string result;
    size_t i = 0;
    bool in_string = false;
    char string_char = 0;
    
    while (i < sql.length()) {
        char c = sql[i];
        
        // Handle string literals
        if (in_string) {
            if (c == string_char && (i + 1 < sql.length() && sql[i + 1] == string_char)) {
                result += c;
                result += sql[i + 1];
                i += 2;
                continue;
            } else if (c == string_char) {
                in_string = false;
                result += c;
                i++;
                continue;
            }
            result += c;
            i++;
            continue;
        }
        
        if (c == '\'' || c == '"') {
            in_string = true;
            string_char = c;
            result += c;
            i++;
            continue;
        }
        
        // Single line comment
        if (c == '-' && i + 1 < sql.length() && sql[i + 1] == '-') {
            while (i < sql.length() && sql[i] != '\n') {
                i++;
            }
            continue;
        }
        
        // Multi-line comment
        if (c == '/' && i + 1 < sql.length() && sql[i + 1] == '*') {
            i += 2;
            while (i < sql.length()) {
                if (sql[i] == '*' && i + 1 < sql.length() && sql[i + 1] == '/') {
                    i += 2;
                    break;
                }
                i++;
            }
            continue;
        }
        
        result += c;
        i++;
    }
    
    return result;
}

} // namespace gh_ost