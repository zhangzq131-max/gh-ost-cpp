/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "context/migration_context.hpp"
#include "utils/logger.hpp"
#include "utils/time_utils.hpp"
#include "utils/string_utils.hpp"

namespace gh_ost {

MigrationContext::MigrationContext()
    : sql_builder_("", "", "", "")
    , total_row_count_(0)
    , copied_row_count_(0)
    , applied_event_count_(0)
    , queued_event_count_(0)
    , status_(MigrationStatus::NotStarted)
    , current_phase_(Phase::NotStarted)
    , throttled_(false)
    , throttle_reason_(ThrottleReason::None)
    , cut_over_phase_(false) {
}

MigrationContext::~MigrationContext() {}

void MigrationContext::SetConfig(const MigrationConfig& config) {
    config_ = config;
    
    // Generate ghost and changelog table names
    GenerateGhostTableName();
    GenerateChangelogTableName();
    
    // Initialize SQL builder
    sql_builder_ = SQLBuilder(config_.database_name, config_.table_name, 
                              ghost_table_name_, changelog_table_name_);
}

void MigrationContext::SetMasterConnection(std::shared_ptr<Connection> conn) {
    master_connection_ = conn;
}

void MigrationContext::SetReplicaConnection(std::shared_ptr<Connection> conn) {
    replica_connection_ = conn;
}

void MigrationContext::SetMasterKey(const InstanceKey& key) {
    master_key_ = key;
}

void MigrationContext::SetReplicaKey(const InstanceKey& key) {
    replica_key_ = key;
}

void MigrationContext::SetAlterStatement(const AlterStatement& alter) {
    alter_statement_ = alter;
}

void MigrationContext::SetOriginalTableStructure(const TableStructure& structure) {
    original_structure_ = structure;
}

void MigrationContext::SetGhostTableStructure(const TableStructure& structure) {
    ghost_structure_ = structure;
}

void MigrationContext::SetUniqueKey(const KeyInfo& key) {
    unique_key_ = key;
}

void MigrationContext::SetColumnPairs(const ColumnPairs& pairs) {
    column_pairs_ = pairs;
}

void MigrationContext::SetMinKey(const std::string& key) {
    min_key_ = key;
}

void MigrationContext::SetMaxKey(const std::string& key) {
    max_key_ = key;
}

void MigrationContext::SetTotalRowCount(uint64_t count) {
    total_row_count_ = count;
}

MigrationProgress MigrationContext::GetProgress() const {
    MigrationProgress progress;
    progress.total_rows = total_row_count_;
    progress.copied_rows = copied_row_count_;
    progress.remaining_rows = total_row_count_ - copied_row_count_;
    progress.events_applied = applied_event_count_;
    progress.events_queued = queued_event_count_;
    progress.percentage_complete = GetProgressPercentage();
    progress.estimated_remaining = EstimatedRemaining();
    return progress;
}

double MigrationContext::GetProgressPercentage() const {
    if (total_row_count_ == 0) return 0.0;
    return static_cast<double>(copied_row_count_) / static_cast<double>(total_row_count_) * 100.0;
}

void MigrationContext::SetStatus(MigrationStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = status;
}

void MigrationContext::SetThrottled(bool throttled) {
    throttled_ = throttled;
}

void MigrationContext::SetThrottleReason(ThrottleReason reason) {
    throttle_reason_ = reason;
}

void MigrationContext::SetCutOverPhase(bool phase) {
    cut_over_phase_ = phase;
}

void MigrationContext::SetInitialBinlogCoordinates(const BinlogCoordinates& coords) {
    initial_coordinates_ = coords;
}

void MigrationContext::SetCurrentBinlogCoordinates(const BinlogCoordinates& coords) {
    current_coordinates_ = coords;
}

void MigrationContext::SetStartTime() {
    start_time_ = TimeUtils::Now();
}

Duration MigrationContext::ElapsedDuration() const {
    if (start_time_ == TimePoint{}) return Duration(0);
    return TimeUtils::DurationBetween(start_time_, TimeUtils::Now());
}

Duration MigrationContext::EstimatedRemaining() const {
    double elapsed_seconds = TimeUtils::ToDoubleSeconds(ElapsedDuration());
    
    if (elapsed_seconds <= 0 || copied_row_count_ == 0) return Duration(0);
    
    // Calculate rate (rows per second)
    double rate = static_cast<double>(copied_row_count_) / elapsed_seconds;
    
    // Estimate remaining rows
    uint64_t remaining = total_row_count_ - copied_row_count_;
    
    // Estimate time
    double estimated_seconds = static_cast<double>(remaining) / rate;
    
    return Duration(estimated_seconds);
}

void MigrationContext::SetCurrentPhase(Phase phase) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_phase_ = phase;
}

std::string MigrationContext::PhaseToString(Phase phase) const {
    switch (phase) {
        case Phase::NotStarted: return "Not Started";
        case Phase::InitialSetup: return "Initial Setup";
        case Phase::RowCopy: return "Row Copy";
        case Phase::AllEventsUpToLockProcessed: return "Processing Events";
        case Phase::CutOver: return "Cut-Over";
        case Phase::Cleanup: return "Cleanup";
        case Phase::Completed: return "Completed";
        case Phase::Failed: return "Failed";
        default: return "Unknown";
    }
}

void MigrationContext::SetLastError(const std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_ = error;
}

void MigrationContext::GenerateGhostTableName() {
    if (!config_.ghost_table_name.empty()) {
        ghost_table_name_ = config_.ghost_table_name;
    } else {
        ghost_table_name_ = "_gh_ost_" + config_.table_name;
    }
}

void MigrationContext::GenerateChangelogTableName() {
    if (!config_.changelog_table_name.empty()) {
        changelog_table_name_ = config_.changelog_table_name;
    } else {
        changelog_table_name_ = "_gh_ost_" + config_.table_name + "_changelog";
    }
}

} // namespace gh_ost