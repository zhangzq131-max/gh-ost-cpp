# FindMySQL.cmake - Find MySQL Connector/C++
#
# This module finds MySQL Connector/C++ library and headers.
#
# Variables defined:
#   MYSQL_CPP_FOUND        - True if MySQL Connector/C++ was found
#   MYSQL_CPP_INCLUDE_DIRS - Include directories
#   MYSQL_CPP_LIBRARIES    - Libraries to link against
#   MYSQL_CPP_VERSION      - Version string
#
# Usage:
#   find_package(MySQL REQUIRED)
#   target_link_libraries(your_target ${MYSQL_CPP_LIBRARIES})
#

# Try pkg-config first
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_MYSQL_CPP QUIET mysqlcppconn)
endif()

# Find include directory
find_path(MYSQL_CPP_INCLUDE_DIR
    NAMES 
        mysql_connection.h
        cppconn/driver.h
    HINTS 
        ${PC_MYSQL_CPP_INCLUDE_DIRS}
        $ENV{MYSQL_CPP_INCLUDE_DIR}
    PATHS
        /usr/include/mysql/cppconn
        /usr/local/include/mysql/cppconn
        /usr/include/cppconn
        /usr/local/include/cppconn
        C:/Program Files/MySQL/MySQL Connector C++ 8.0/include
        C:/Program Files/MySQL/MySQL Connector C++/include
        "C:/Program Files (x86)/MySQL/MySQL Connector C++ 8.0/include"
        "C:/Program Files (x86)/MySQL/MySQL Connector C++/include"
)

# Find library
find_library(MYSQL_CPP_LIBRARY
    NAMES 
        mysqlcppconn
        mysqlcppconn-static
        mysqlcppconn-x
        mysqlcppconn-8
    HINTS 
        ${PC_MYSQL_CPP_LIBRARY_DIRS}
        $ENV{MYSQL_CPP_LIBRARY_DIR}
    PATHS
        /usr/lib
        /usr/local/lib
        /usr/lib/mysql
        /usr/local/lib/mysql
        C:/Program Files/MySQL/MySQL Connector C++ 8.0/lib
        C:/Program Files/MySQL/MySQL Connector C++/lib
        "C:/Program Files (x86)/MySQL/MySQL Connector C++ 8.0/lib"
        "C:/Program Files (x86)/MySQL/MySQL Connector C++/lib"
)

# Get version
if(PC_MYSQL_CPP_VERSION)
    set(MYSQL_CPP_VERSION ${PC_MYSQL_CPP_VERSION})
elseif(EXISTS "${MYSQL_CPP_INCLUDE_DIR}/mysql_connection.h")
    file(READ "${MYSQL_CPP_INCLUDE_DIR}/mysql_connection.h" _mysql_header)
    string(REGEX MATCH "#define[ \t]+MYSQLCPPCONN_VERSION[ \t]+\"([^\"]+)\"" 
        _version_match "${_mysql_header}")
    if(_version_match)
        set(MYSQL_CPP_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL_CPP
    REQUIRED_VARS 
        MYSQL_CPP_LIBRARY 
        MYSQL_CPP_INCLUDE_DIR
    VERSION_VAR 
        MYSQL_CPP_VERSION
)

# Set output variables
if(MYSQL_CPP_FOUND)
    set(MYSQL_CPP_INCLUDE_DIRS ${MYSQL_CPP_INCLUDE_DIR})
    set(MYSQL_CPP_LIBRARIES ${MYSQL_CPP_LIBRARY})
    
    # Create imported target
    if(NOT TARGET MySQL::Connector)
        add_library(MySQL::Connector UNKNOWN IMPORTED)
        set_target_properties(MySQL::Connector PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_CPP_INCLUDE_DIRS}"
            IMPORTED_LOCATION "${MYSQL_CPP_LIBRARIES}"
        )
    endif()
    
    mark_as_advanced(MYSQL_CPP_INCLUDE_DIR MYSQL_CPP_LIBRARY)
endif()