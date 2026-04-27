# gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
# Copyright 2026
# Licensed under MIT License

cmake_minimum_required(VERSION 3.16...4.0)

# Set policy version for compatibility with CMake 4.x
set(CMAKE_POLICY_VERSION_MINIMUM 3.16 CACHE STRING "" FORCE)

project(gh-ost-cpp VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(GH_OST_BUILD_TESTS "Build unit tests" OFF)
option(GH_OST_BUILD_SHARED "Build as shared library" OFF)
option(GH_OST_ENABLE_DEBUG "Enable debug logging" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerOptions)

# Find dependencies
find_package(Threads REQUIRED)

# MySQL Connector/C++ - using pkg-config or manual finding
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MYSQL_CPP mysqlcppconn)
endif()

if(NOT MYSQL_CPP_FOUND)
    # Manual search for MySQL Connector/C++
    find_path(MYSQL_CPP_INCLUDE_DIR 
        NAMES mysql_connection.h
        PATHS 
            /usr/include/mysql/cppconn
            /usr/local/include/mysql/cppconn
            $ENV{MYSQL_CPP_INCLUDE_DIR}
    )
    find_library(MYSQL_CPP_LIBRARY 
        NAMES mysqlcppconn mysqlcppconn-static
        PATHS 
            /usr/lib
            /usr/local/lib
            $ENV{MYSQL_CPP_LIBRARY_DIR}
    )
    
    if(MYSQL_CPP_INCLUDE_DIR AND MYSQL_CPP_LIBRARY)
        set(MYSQL_CPP_FOUND TRUE)
        set(MYSQL_CPP_INCLUDE_DIRS ${MYSQL_CPP_INCLUDE_DIR})
        set(MYSQL_CPP_LIBRARIES ${MYSQL_CPP_LIBRARY})
    endif()
endif()

if(MYSQL_CPP_FOUND)
    message(STATUS "Found MySQL Connector/C++: ${MYSQL_CPP_LIBRARIES}")
else()
    message(WARNING "MySQL Connector/C++ not found. Some features may be unavailable.")
endif()

# spdlog (logging)
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found, will use bundled version or fetch from GitHub")
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# cxxopts (CLI parsing)
include(FetchContent)
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(cxxopts)

# nlohmann_json (JSON handling)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Source files
set(GH_OST_SOURCES
    src/utils/logger.cpp
    src/utils/string_utils.cpp
    src/utils/time_utils.cpp
    src/utils/file_utils.cpp
    src/utils/thread_pool.cpp
    src/context/migration_context.cpp
    src/mysql/connection.cpp
    src/mysql/instance_key.cpp
    src/mysql/utils.cpp
    src/sql/parser.cpp
    src/sql/builder.cpp
    src/sql/types.cpp
    src/sql/encoding.cpp
    src/binlog/binlog_reader.cpp
    src/binlog/binlog_dml_event.cpp
    src/binlog/binlog_entry.cpp
    src/binlog/binlog_coordinates.cpp
    src/logic/migrator.cpp
    src/logic/inspector.cpp
    src/logic/applier.cpp
    src/logic/streamer.cpp
    src/logic/checkpoint.cpp
    src/throttle/throttler.cpp
    src/hooks/hooks_executor.cpp
    src/server/server.cpp
    src/server/interactive_commands.cpp
)

# Header files
set(GH_OST_HEADERS
    include/gh_ost/types.hpp
    include/gh_ost/config.hpp
    include/gh_ost/version.hpp
    include/utils/logger.hpp
    include/utils/string_utils.hpp
    include/utils/time_utils.hpp
    include/utils/file_utils.hpp
    include/utils/thread_pool.hpp
    include/utils/safe_queue.hpp
    include/context/migration_context.hpp
    include/mysql/connection.hpp
    include/mysql/instance_key.hpp
    include/mysql/utils.hpp
    include/mysql/binlog_file.hpp
    include/sql/parser.hpp
    include/sql/builder.hpp
    include/sql/types.hpp
    include/sql/encoding.hpp
    include/binlog/binlog_reader.hpp
    include/binlog/binlog_dml_event.hpp
    include/binlog/binlog_entry.hpp
    include/binlog/binlog_coordinates.hpp
    include/logic/migrator.hpp
    include/logic/inspector.hpp
    include/logic/applier.hpp
    include/logic/streamer.hpp
    include/logic/checkpoint.hpp
    include/throttle/throttler.hpp
    include/hooks/hooks_executor.hpp
    include/server/server.hpp
    include/server/interactive_commands.hpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MYSQL_CPP_INCLUDE_DIRS}
)

# Build executable
if(GH_OST_BUILD_SHARED)
    add_library(gh-ost SHARED ${GH_OST_SOURCES} ${GH_OST_HEADERS})
else()
    add_library(gh-ost STATIC ${GH_OST_SOURCES} ${GH_OST_HEADERS})
endif()

# Link dependencies to gh-ost library
target_link_libraries(gh-ost PUBLIC
    Threads::Threads
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

add_executable(gh-ost-cli src/main.cpp)

# Add include directories from fetched dependencies
target_include_directories(gh-ost-cli PRIVATE
    ${CMAKE_BINARY_DIR}/_deps/cxxopts-src/include
)

target_link_libraries(gh-ost-cli PRIVATE
    gh-ost
)

if(MYSQL_CPP_FOUND)
    target_link_libraries(gh-ost-cli PRIVATE ${MYSQL_CPP_LIBRARIES})
    target_compile_definitions(gh-ost-cli PRIVATE HAVE_MYSQL_CPP=1)
endif()

# Compiler definitions
target_compile_definitions(gh-ost-cli PRIVATE
    GH_OST_VERSION=\"${PROJECT_VERSION}\"
    GH_OST_NAME=\"gh-ost-cpp\"
)

if(GH_OST_ENABLE_DEBUG)
    target_compile_definitions(gh-ost-cli GH_OST_DEBUG=1)
endif()

# Tests
if(GH_OST_BUILD_TESTS)
    enable_testing()
    
    # Catch2 for testing
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(catch2)
    
    # Unit tests
    add_executable(gh-ost-tests
        tests/unit/test_main.cpp
        tests/unit/test_string_utils.cpp
        tests/unit/test_time_utils.cpp
        tests/unit/test_sql_parser.cpp
        tests/unit/test_binlog_entry.cpp
        tests/unit/test_instance_key.cpp
    )
    
    target_link_libraries(gh-ost-tests PRIVATE
        gh-ost
        Catch2::Catch2
    )
    
    add_test(NAME gh-ost-unit-tests COMMAND gh-ost-tests)
endif()

# Install rules
install(TARGETS gh-ost-cli DESTINATION bin)
install(TARGETS gh-ost DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/gh-ost)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gh-ost-cppConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Summary
message(STATUS "")
message(STATUS "gh-ost-cpp Configuration Summary:")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests:    ${GH_OST_BUILD_TESTS}")
message(STATUS "  Debug Mode:     ${GH_OST_ENABLE_DEBUG}")
message(STATUS "  MySQL CPP:      ${MYSQL_CPP_FOUND}")
message(STATUS "")