# CompilerOptions.cmake - Compiler-specific options for gh-ost-cpp

# Common compiler flags for all platforms
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -fPIC
    )
    
    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    
    # Release flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    
    # Address sanitizer (optional)
    option(GH_OST_ENABLE_ASAN "Enable Address Sanitizer" OFF)
    if(GH_OST_ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        /W4
        /utf-8
        /permissive-
    )
    
    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /DDEBUG")
    
    # Release flags
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    
    # Disable specific MSVC warnings
    add_compile_options(
        /wd4251  # DLL interface warning
        /wd4275  # DLL base class warning
    )
endif()

# C++17 specific features check
include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
    #include <optional>
    int main() { std::optional<int> o; return 0; }
" HAVE_OPTIONAL)

check_cxx_source_compiles("
    #include <string_view>
    int main() { std::string_view sv; return 0; }
" HAVE_STRING_VIEW)

check_cxx_source_compiles("
    #include <filesystem>
    int main() { std::filesystem::path p; return 0; }
" HAVE_FILESYSTEM)

# Thread support
find_package(Threads REQUIRED)
if(Threads_FOUND)
    set(HAVE_THREADS TRUE)
endif()