/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "logic/checkpoint.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

namespace gh_ost {

Checkpoint::Checkpoint(MigrationContext& context)
    : context_(context) {
}

Checkpoint::~Checkpoint() {}

bool Checkpoint::ReadCheckpoint() {
    // Read latest state from changelog table
    auto conn = context_.MasterConnection();
    if (!conn || !conn->IsConnected()) {
        return false;
    }
    
    auto row = conn->QueryRow(
        "SELECT hint, value FROM " +
        conn->QuoteIdentifier(context_.DatabaseName()) + "." +
        conn->QuoteIdentifier(context_.ChangelogTableName()) +
        " ORDER BY id DESC LIMIT 1");
    
    if (row) {
        current_state_.hint = row->Get("hint").value_or("");
        current_state_.value = row->Get("value").value_or("");
        
        return ParseChangelogEntry(current_state_.hint, current_state_.value);
    }
    
    return false;
}

bool Checkpoint::WriteCheckpoint() {
    return WriteHint(Hint::Heartbeat, 
        StringUtils::ToString(TimeUtils::NowUnixMillis()));
}

bool Checkpoint::ClearCheckpoint() {
    auto conn = context_.MasterConnection();
    if (!conn) return false;
    
    auto result = conn->Execute(
        "DELETE FROM " +
        conn->QuoteIdentifier(context_.DatabaseName()) + "." +
        conn->QuoteIdentifier(context_.ChangelogTableName()));
    
    return result.success;
}

Checkpoint::CheckpointState Checkpoint::GetCurrentState() const {
    return current_state_;
}

void Checkpoint::SetState(const CheckpointState& state) {
    current_state_ = state;
}

bool Checkpoint::CanResume() const {
    return current_state_.IsComplete() || current_state_.IsCancelled();
}

bool Checkpoint::ResumeFromCheckpoint() {
    if (!CanResume()) {
        return false;
    }
    
    LOG_INFO_FMT("Resuming from checkpoint: {}", current_state_.phase);
    
    // Restore state from checkpoint
    context_.SetCurrentBinlogCoordinates(current_state_.coordinates);
    
    return true;
}

bool Checkpoint::WriteHint(Hint hint, const std::string& value) {
    auto conn = context_.MasterConnection();
    if (!conn) return false;
    
    std::string hint_str = HintToString(hint);
    
    auto result = conn->Execute(
        "INSERT INTO " +
        conn->QuoteIdentifier(context_.DatabaseName()) + "." +
        conn->QuoteIdentifier(context_.ChangelogTableName()) +
        " (hint, value) VALUES ('" + StringUtils::EscapeForSQL(hint_str) + "', '" +
        StringUtils::EscapeForSQL(value) + "')");
    
    return result.success;
}

std::optional<std::string> Checkpoint::ReadHint(Hint hint) {
    auto conn = context_.MasterConnection();
    if (!conn) return std::nullopt;
    
    std::string hint_str = HintToString(hint);
    
    auto val = conn->QueryValue(
        "SELECT value FROM " +
        conn->QuoteIdentifier(context_.DatabaseName()) + "." +
        conn->QuoteIdentifier(context_.ChangelogTableName()) +
        " WHERE hint = '" + StringUtils::EscapeForSQL(hint_str) + "' " +
        " ORDER BY id DESC LIMIT 1");
    
    return val;
}

Checkpoint::Hint Checkpoint::ParseHint(const std::string& hint_str) {
    if (hint_str == "heartbeat") return Hint::Heartbeat;
    if (hint_str == "ghost-table-migrated") return Hint::GhostTableMigrated;
    if (hint_str == "row-copy-complete") return Hint::RowCopyComplete;
    if (hint_str == "all-events-up-to-lock-processed") return Hint::AllEventsUpToLockProcessed;
    if (hint_str == "cut-over-phase") return Hint::CutOverPhase;
    if (hint_str == "completed") return Hint::Completed;
    if (hint_str == "cancelled") return Hint::Cancelled;
    if (hint_str == "panic-abort") return Hint::PanicAbort;
    
    return Hint::Heartbeat;
}

std::string Checkpoint::HintToString(Hint hint) {
    switch (hint) {
        case Hint::Heartbeat: return "heartbeat";
        case Hint::GhostTableMigrated: return "ghost-table-migrated";
        case Hint::RowCopyComplete: return "row-copy-complete";
        case Hint::AllEventsUpToLockProcessed: return "all-events-up-to-lock-processed";
        case Hint::CutOverPhase: return "cut-over-phase";
        case Hint::Completed: return "completed";
        case Hint::Cancelled: return "cancelled";
        case Hint::PanicAbort: return "panic-abort";
        default: return "unknown";
    }
}

bool Checkpoint::CheckpointState::IsComplete() const {
    return hint == "completed";
}

bool Checkpoint::CheckpointState::IsCancelled() const {
    return hint == "cancelled" || hint == "panic-abort";
}

bool Checkpoint::ParseChangelogEntry(const std::string& hint, const std::string& value) {
    current_state_.hint = hint;
    current_state_.value = value;
    
    // Parse coordinates if present
    auto coords = BinlogCoordinates::Parse(value);
    if (coords) {
        current_state_.coordinates = *coords;
    }
    
    return true;
}

bool Checkpoint::ValidateState() const {
    return !current_state_.hint.empty();
}

} // namespace gh_ost