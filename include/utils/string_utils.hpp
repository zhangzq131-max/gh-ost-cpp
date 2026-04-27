/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_STRING_UTILS_HPP
#define GH_OST_STRING_UTILS_HPP

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <sstream>
#include <optional>
#include <cstdint>
#include <cctype>
#include <iomanip>
#include <regex>

namespace gh_ost {

class StringUtils {
public:
    // Trim functions
    static std::string Trim(const std::string& str);
    static std::string TrimLeft(const std::string& str);
    static std::string TrimRight(const std::string& str);
    
    // Trim whitespace and specific characters
    static std::string Trim(const std::string& str, char ch);
    static std::string Trim(const std::string& str, const std::string& chars);
    
    // Case conversion
    static std::string ToUpper(const std::string& str);
    static std::string ToLower(const std::string& str);
    static std::string Capitalize(const std::string& str);
    
    // Split functions
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);
    static std::vector<std::string> SplitLines(const std::string& str);
    
    // Join function
    static std::string Join(const std::vector<std::string>& parts, const std::string& delimiter);
    static std::string Join(const std::vector<std::string_view>& parts, const std::string& delimiter);
    
    // Contains and find functions
    static bool Contains(const std::string& str, const std::string& substring);
    static bool Contains(const std::string& str, char ch);
    static bool ContainsIgnoreCase(const std::string& str, const std::string& substring);
    
    static size_t Find(const std::string& str, const std::string& substring);
    static size_t FindIgnoreCase(const std::string& str, const std::string& substring);
    
    // Starts with / Ends with
    static bool StartsWith(const std::string& str, const std::string& prefix);
    static bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix);
    static bool EndsWith(const std::string& str, const std::string& suffix);
    static bool EndsWithIgnoreCase(const std::string& str, const std::string& suffix);
    
    // Replace functions
    static std::string Replace(const std::string& str, const std::string& from, const std::string& to);
    static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);
    static std::string ReplaceFirst(const std::string& str, const std::string& from, const std::string& to);
    static std::string ReplaceLast(const std::string& str, const std::string& from, const std::string& to);
    
    // String manipulation
    static std::string PadLeft(const std::string& str, size_t length, char pad = ' ');
    static std::string PadRight(const std::string& str, size_t length, char pad = ' ');
    static std::string Repeat(const std::string& str, size_t count);
    
    // Substring
    static std::string Substring(const std::string& str, size_t start, size_t length = std::string::npos);
    static std::string Take(const std::string& str, size_t n);
    static std::string Drop(const std::string& str, size_t n);
    
    // Is empty / blank
    static bool IsEmpty(const std::string& str);
    static bool IsBlank(const std::string& str);
    
    // Number conversion
    static std::optional<int32_t> ParseInt32(const std::string& str);
    static std::optional<int64_t> ParseInt64(const std::string& str);
    static std::optional<uint32_t> ParseUInt32(const std::string& str);
    static std::optional<uint64_t> ParseUInt64(const std::string& str);
    static std::optional<double> ParseDouble(const std::string& str);
    static std::optional<float> ParseFloat(const std::string& str);
    
    static std::string ToString(int32_t value);
    static std::string ToString(int64_t value);
    static std::string ToString(uint32_t value);
    static std::string ToString(uint64_t value);
    static std::string ToString(double value, int precision = 6);
    static std::string ToString(float value, int precision = 6);
    static std::string ToString(bool value);
    
    // Hex conversion
    static std::string ToHex(const std::string& str);
    static std::string ToHex(const std::vector<uint8_t>& data);
    static std::string FromHex(const std::string& hex);
    static std::vector<uint8_t> HexToBytes(const std::string& hex);
    
    // Escape/Unescape for SQL
    static std::string EscapeForSQL(const std::string& str);
    static std::string QuoteForSQL(const std::string& str, char quote = '\'');
    static std::string QuoteIdentifier(const std::string& identifier, char quote = '`');
    
    // Escape/Unescape for JSON
    static std::string EscapeForJSON(const std::string& str);
    static std::string UnescapeFromJSON(const std::string& str);
    
    // Format functions
    template<typename... Args>
    static std::string Format(const std::string& fmt, Args&&... args) {
        // Simple placeholder replacement - for more complex formatting use fmt library
        std::string result = fmt;
        size_t pos = 0;
        std::string args_str[] = { ToString(std::forward<Args>(args))... };
        int index = 0;
        for (const auto& arg : args_str) {
            std::string placeholder = "{" + std::to_string(index) + "}";
            pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), arg);
                pos += arg.length();
            }
            index++;
        }
        return result;
    }
    
    // SQL identifier handling
    static std::string NormalizeTableName(const std::string& name);
    static std::string CombineDatabaseTable(const std::string& database, const std::string& table);
    
    // Wildcard matching
    static bool MatchesWildcard(const std::string& str, const std::string& pattern);
    
    // Remove comments from SQL
    static std::string RemoveSQLComments(const std::string& sql);
    
private:
    StringUtils() = delete;
};

} // namespace gh_ost

#endif // GH_OST_STRING_UTILS_HPP