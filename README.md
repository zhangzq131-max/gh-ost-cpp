# gh-ost-cpp

GitHub's Online Schema Transmogrifier for MySQL - C++ Implementation

## Overview

`gh-ost-cpp` is a C++ implementation of GitHub's [gh-ost](https://github.com/github/gh-ost), a triggerless online schema migration tool for MySQL.

### What is gh-ost?

gh-ost is a tool for performing online schema changes (ALTER TABLE) on MySQL databases without blocking normal operations. Unlike traditional tools that use triggers, gh-ost reads binary logs to capture table changes and applies them asynchronously to a ghost (shadow) table.

## Features

- **Triggerless Design**: No triggers required, avoiding the performance overhead and limitations of trigger-based solutions
- **Asynchronous Processing**: Changes are applied asynchronously from the binlog stream
- **Testable**: Can test migrations on replicas before executing on production
- **Throttleable**: Built-in throttling based on replication lag, load, and custom queries
- **Interactive Control**: Runtime control via TCP socket commands
- **Safe Cut-over**: Atomic table swap with safety mechanisms

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+
- MySQL Connector/C++ (optional, for full functionality)

### Build Steps

```bash
cd D:/codebase/gh-ost-cpp

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest
```

### Build Options

| Option | Description |
|--------|-------------|
| `GH_OST_BUILD_TESTS` | Build unit tests (default: ON) |
| `GH_OST_BUILD_SHARED` | Build as shared library (default: OFF) |
| `GH_OST_ENABLE_DEBUG` | Enable debug logging (default: OFF) |

## Usage

### Basic Migration

```bash
# Test migration (noop mode)
gh-ost-cli --database=mydb --table=mytable --alter="ADD COLUMN new_col INT"

# Execute migration
gh-ost-cli --database=mydb --table=mytable --alter="ADD COLUMN new_col INT" --execute
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `--database` | Database name |
| `--table` | Table to migrate |
| `--alter` | ALTER statement |
| `--execute` | Execute migration |
| `--host` | MySQL host |
| `--port` | MySQL port |
| `--user` | MySQL user |
| `--password` | MySQL password |
| `--max-replication-lag` | Max replication lag threshold |
| `--chunk-size` | Row copy chunk size |
| `--throttle-query` | Custom throttle query |
| `--serve-status-http` | HTTP port for status |

## Project Structure

```
D:/codebase/gh-ost-cpp/
├── CMakeLists.txt          # Main build configuration
├── cmake/                  # CMake modules
├── include/                # Header files
│   ├── gh_ost/             # Core types and config
│   ├── mysql/              # MySQL connection module
│   ├── binlog/             # Binlog reading module
│   ├── sql/                # SQL parsing module
│   ├── logic/              # Migration logic
│   ├── throttle/           # Throttling
│   ├── hooks/              # Hook system
│   ├── server/             # Status server
│   ├── context/            # Migration context
│   └── utils/              # Utility classes
├── src/                    # Source files
├── tests/                  # Unit tests
├── docs/                   # Documentation
└── third_party/            # Third-party dependencies
```

## Architecture

The migration workflow:

```
┌─────────────────┐     ┌─────────────────┐
│   Original      │────▶│    Binlog       │
│    Table        │     │    Stream       │
└─────────────────┘     └─────────────────┘
                                │
                                ▼
                        ┌─────────────────┐
                        │   Events        │
                        │   Streamer      │
                        └─────────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Row Copy      │     │   Events        │     │   Throttler     │
│   Thread        │     │   Applier       │     │                 │
└─────────────────┘     └─────────────────┘     └─────────────────┘
        │                       │
        └───────────────────────┼───────────────────────┘
                                │
                                ▼
                        ┌─────────────────┐
                        │    Ghost        │
                        │    Table        │
                        └─────────────────┘
                                │
                                ▼
                        ┌─────────────────┐
                        │   Cut-over      │
                        │   (Atomic Swap) │
                        └─────────────────┘
```

## Modules

### MySQL Connection (`mysql/`)

- `Connection`: MySQL connection wrapper
- `InstanceKey`: Server identification (host:port)
- `Utils`: MySQL utility functions

### SQL Parser (`sql/`)

- `AlterTableParser`: ALTER TABLE statement parser
- `Builder`: SQL statement builder for migration operations
- `Types`: SQL type definitions

### Binlog Reader (`binlog/`)

- `BinlogReader`: Binlog stream reader interface
- `BinlogDMLEvent`: DML event processing (INSERT/UPDATE/DELETE)
- `BinlogCoordinates`: Position tracking

### Logic (`logic/`)

- `Migrator`: Main migration flow manager
- `Inspector`: Table structure inspection
- `Applier`: Change application to ghost table
- `Streamer`: Event stream processing

### Throttle (`throttle/`)

- `Throttler`: Load and lag-based throttling

### Server (`server/`)

- `Server`: Status and interactive command server
- `InteractiveCommands`: Command processing

## Testing

Unit tests are located in `tests/unit/`. Run with:

```bash
cd build
ctest --output-on-failure
```

Test coverage includes:

- String utilities
- Time utilities
- SQL parsing
- Binlog handling
- Instance key management

## Dependencies

| Library | Purpose |
|---------|---------|
| MySQL Connector/C++ | MySQL connection |
| spdlog | Logging |
| cxxopts | CLI parsing |
| nlohmann/json | JSON handling |
| Catch2 | Testing |

Dependencies are automatically fetched via CMake's FetchContent.

## License

MIT License

## Credits

This is a C++ implementation of GitHub's original [gh-ost](https://github.com/github/gh-ost) tool, developed in Go.