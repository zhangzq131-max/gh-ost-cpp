/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_VERSION_HPP
#define GH_OST_VERSION_HPP

#include <string>

namespace gh_ost {

// Version constants
constexpr const char* VERSION = "1.0.0";
constexpr const char* NAME = "gh-ost-cpp";
constexpr const char* DESCRIPTION = "GitHub's Online Schema Transmogrifier for MySQL";

// Build information
constexpr int MAJOR_VERSION = 1;
constexpr int MINOR_VERSION = 0;
constexpr int PATCH_VERSION = 0;

// Get version string
inline std::string GetVersion() {
    return std::string(VERSION);
}

// Get full version with build info
inline std::string GetFullVersion() {
    return std::string(NAME) + " " + std::string(VERSION);
}

// Get version components
struct VersionInfo {
    int major;
    int minor;
    int patch;
    std::string name;
    std::string full_string;
    
    VersionInfo() 
        : major(MAJOR_VERSION)
        , minor(MINOR_VERSION)
        , patch(PATCH_VERSION)
        , name(NAME)
        , full_string(GetFullVersion()) {}
};

} // namespace gh_ost

#endif // GH_OST_VERSION_HPP