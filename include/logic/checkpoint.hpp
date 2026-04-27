/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_CHECKPOINT_HPP
#define GH_OST_CHECKPOINT_HPP

#include "context/migration_context.hpp"
#include "binlog/binlog_coordinates.hpp"
#include <string>
#include <optional>
#include <cstdint>

namespace gh_ost {

// Checkpoint state tracking for migration resumption
class Checkpoint {
public:
    Checkpoint(MigrationContext& context);
    ~Checkpoint();
    
    // Read checkpoint from changelog
    bool ReadCheckpoint();
    
    // Write checkpoint to changelog
    bool WriteCheckpoint();
    
    // Clear checkpoint
    bool ClearCheckpoint();
    
    // Checkpoint state
    struct CheckpointState {
        std::string hint;                   // Hint type
        std::string value;                  // Hint value
        
        // Migration state
        BinlogCoordinates coordinates;
        uint64_t copied_rows;
        std::string phase;
        std::string status;
        
        bool IsComplete() const;
        bool IsCancelled() const;
    };
    
    // Get current checkpoint state
    CheckpointState GetCurrentState() const;
    
    // Set checkpoint state
    void SetState(const CheckpointState& state);
    
    // Check if migration can resume
    bool CanResume() const;
    
    // Resume from checkpoint
    bool ResumeFromCheckpoint();
    
    // Migration hints
    enum class Hint {
        Heartbeat,
        GhostTableMigrated,
        RowCopyComplete,
        AllEventsUpToLockProcessed,
        CutOverPhase,
        Completed,
        Cancelled,
        PanicAbort
    };
    
    // Write specific hint
    bool WriteHint(Hint hint, const std::string& value = "");
    
    // Read specific hint
    std::optional<std::string> ReadHint(Hint hint);
    
    // Parse hint from changelog value
    static Hint ParseHint(const std::string& hint_str);
    static std::string HintToString(Hint hint);
    
private:
    MigrationContext& context_;
    CheckpointState current_state_;
    
    // Parse changelog entry
    bool ParseChangelogEntry(const std::string& hint, const std::string& value);
    
    // Validate checkpoint state
    bool ValidateState() const;
};

} // namespace gh_ost

#endif // GH_OST_CHECKPOINT_HPP