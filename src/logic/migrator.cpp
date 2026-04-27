/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "logic/migrator.hpp"
#include "utils/logger.hpp"
#include "utils/time_utils.hpp"
#include "utils/string_utils.hpp"
#include <sstream>

namespace gh_ost {

Migrator::Migrator()
    : running_(false)
    , paused_(false)
    , cancelled_(false)
    , first_throttling_collected_(false)
    , ghost_table_migrated_(false)
    , row_copy_complete_(false)
    , all_events_up_to_lock_processed_(false)
    , copy_rows_queue_(1000)
    , apply_events_queue_(10000) {
}

Migrator::~Migrator() {
    Cancel();
}

bool Migrator::Initialize(const MigrationConfig& config) {
    context_.SetConfig(config);
    context_.SetStartTime();
    context_.SetStatus(MigrationStatus::Running);
    context_.SetCurrentPhase(MigrationContext::Phase::InitialSetup);
    
    // Create components
    inspector_ = std::make_unique<Inspector>(context_);
    applier_ = std::make_unique<Applier>(context_);
    streamer_ = std::make_unique<EventsStreamer>(context_);
    throttler_ = std::make_unique<Throttler>(context_);
    hooks_executor_ = std::make_unique<HooksExecutor>(context_);
    server_ = std::make_unique<Server>(context_);
    
    // Thread pool
    thread_pool_ = std::make_unique<ThreadPool>(4);
    
    LOG_INFO("Migrator initialized");
    return true;
}

bool Migrator::Run() {
    running_ = true;
    
    LOG_INFO("Starting migration");
    
    // Execute pre-migration hook
    if (hooks_executor_) {
        hooks_executor_->ExecuteHook("before-migration");
    }
    
    // Phase 1: Initial setup
    if (!PhaseInitialSetup()) {
        HandleError("Initial setup failed");
        return false;
    }
    
    // Phase 2: Create ghost table
    if (!PhaseCreateGhostTable()) {
        HandleError("Create ghost table failed");
        return false;
    }
    
    // Phase 3: Alter ghost table
    if (!PhaseAlterGhostTable()) {
        HandleError("Alter ghost table failed");
        return false;
    }
    
    // Phase 4: Inspect original table
    if (!PhaseInspectOriginalTable()) {
        HandleError("Inspect original table failed");
        return false;
    }
    
    // Phase 5: Validate migration
    if (!PhaseValidateMigration()) {
        HandleError("Validation failed");
        return false;
    }
    
    // Phase 6: Populate row range
    if (!PhasePopulateRowRange()) {
        HandleError("Populate row range failed");
        return false;
    }
    
    // Phase 7: Initiate stream
    if (!PhaseReadStreamInitiate()) {
        HandleError("Stream initiation failed");
        return false;
    }
    
    // Start status server
    if (server_) {
        server_->Start();
    }
    
    // Phase 8: Row copy (main loop)
    context_.SetCurrentPhase(MigrationContext::Phase::RowCopy);
    
    if (!PhaseRowCopy()) {
        HandleError("Row copy failed");
        return false;
    }
    
    // Phase 9: Cut-over
    context_.SetCurrentPhase(MigrationContext::Phase::CutOver);
    
    if (!PhaseCutOver()) {
        HandleError("Cut-over failed");
        return false;
    }
    
    // Phase 10: Cleanup
    context_.SetCurrentPhase(MigrationContext::Phase::Cleanup);
    
    if (!PhaseCleanup()) {
        HandleError("Cleanup failed");
        return false;
    }
    
    // Execute post-migration hook
    if (hooks_executor_) {
        hooks_executor_->ExecuteHook("after-migration");
    }
    
    context_.SetStatus(MigrationStatus::Completed);
    context_.SetCurrentPhase(MigrationContext::Phase::Completed);
    
    LOG_INFO("Migration completed successfully");
    return true;
}

bool Migrator::PhaseInitialSetup() {
    LOG_INFO("Phase: Initial Setup");
    
    // Initialize inspector
    if (!inspector_->Initialize()) {
        context_.SetLastError("Inspector initialization failed");
        return false;
    }
    
    // Detect master and replica
    if (!inspector_->DetectMaster()) {
        context_.SetLastError("Failed to detect master");
        return false;
    }
    
    // Check privileges
    if (!inspector_->CheckPrivileges()) {
        context_.SetLastError("Insufficient privileges");
        return false;
    }
    
    return true;
}

bool Migrator::PhaseCreateGhostTable() {
    LOG_INFO("Phase: Create Ghost Table");
    
    // Check if ghost table already exists
    if (inspector_->GhostTableExists()) {
        LOG_WARN_FMT("Ghost table {} already exists", context_.GhostTableName());
        
        // Drop existing ghost table if needed
        if (!applier_->DropGhostTable()) {
            return false;
        }
    }
    
    // Create ghost table
    if (!applier_->CreateGhostTable()) {
        return false;
    }
    
    // Create changelog table
    if (inspector_->ChangelogTableExists()) {
        applier_->DropChangelogTable();
    }
    
    if (!applier_->CreateChangelogTable()) {
        return false;
    }
    
    ghost_table_migrated_ = true;
    
    return true;
}

bool Migrator::PhaseAlterGhostTable() {
    LOG_INFO("Phase: Alter Ghost Table");
    
    if (!applier_->AlterGhostTable()) {
        return false;
    }
    
    // Inspect ghost table after alter
    if (!inspector_->InspectGhostTable()) {
        return false;
    }
    
    return true;
}

bool Migrator::PhaseInspectOriginalTable() {
    LOG_INFO("Phase: Inspect Original Table");
    
    if (!inspector_->InspectOriginalTable()) {
        return false;
    }
    
    // Determine shared unique key
    if (!inspector_->DetermineSharedUniqueKey()) {
        context_.SetLastError("Cannot find suitable unique key");
        return false;
    }
    
    // Determine column pairs
    if (!inspector_->DetermineColumnPairs()) {
        return false;
    }
    
    return true;
}

bool Migrator::PhaseValidateMigration() {
    LOG_INFO("Phase: Validate Migration");
    
    auto result = inspector_->ValidateMigration();
    
    if (!result.valid) {
        for (const auto& error : result.errors) {
            LOG_ERROR_FMT("Validation error: {}", error);
        }
        return false;
    }
    
    for (const auto& warning : result.warnings) {
        LOG_WARN_FMT("Validation warning: {}", warning);
    }
    
    return true;
}

bool Migrator::PhasePopulateRowRange() {
    LOG_INFO("Phase: Populate Row Range");
    
    if (!inspector_->ReadRowRange()) {
        return false;
    }
    
    // Get row count
    uint64_t row_count = inspector_->GetEstimatedRowCount();
    if (context_.GetConfig().exact_rowcount) {
        row_count = inspector_->GetExactRowCount();
    }
    
    context_.SetTotalRowCount(row_count);
    
    LOG_INFO_FMT("Row range: {} to {}, total: {} rows", 
                 context_.MinKey(), context_.MaxKey(), row_count);
    
    return true;
}

bool Migrator::PhaseReadStreamInitiate() {
    LOG_INFO("Phase: Initiate Stream");
    
    if (!streamer_->Initialize()) {
        return false;
    }
    
    // Store initial binlog coordinates
    context_.SetInitialBinlogCoordinates(streamer_->GetCurrentCoordinates());
    
    // Write "good to go" hint to changelog
    applier_->WriteChangelog("good-to-go", "");
    
    // Start streamer
    if (!streamer_->Start()) {
        return false;
    }
    
    // Start heartbeat thread
    heartbeat_thread_ = std::thread(&Migrator::RunHeartbeat, this);
    
    // Start throttle check thread
    throttle_thread_ = std::thread(&Migrator::RunThrottleCheck, this);
    
    return true;
}

bool Migrator::PhaseRowCopy() {
    LOG_INFO("Phase: Row Copy");
    
    // Start row copy thread
    row_copy_thread_ = std::thread(&Migrator::RunRowCopy, this);
    
    // Start events apply thread
    events_apply_thread_ = std::thread(&Migrator::RunEventsApply, this);
    
    // Monitor progress
    while (running_ && !row_copy_complete_ && !cancelled_) {
        // Check for throttle
        if (ShouldThrottle()) {
            ApplyThrottle();
        }
        
        // Check for panic abort
        if (CheckForPanicAbort()) {
            PanicAbort();
            return false;
        }
        
        // Update status
        UpdateStatus();
        
        // Sleep
        TimeUtils::SleepFor(context_.GetConfig().status_update_interval_millis);
    }
    
    // Wait for threads to complete
    if (row_copy_thread_.joinable()) row_copy_thread_.join();
    if (events_apply_thread_.joinable()) events_apply_thread_.join();
    
    return !cancelled_;
}

bool Migrator::PhaseCutOver() {
    LOG_INFO("Phase: Cut-Over");
    
    // Wait for all events to be processed
    while (running_ && !all_events_up_to_lock_processed_ && !cancelled_) {
        TimeUtils::SleepFor(100);
    }
    
    if (cancelled_) return false;
    
    // Check for postponed cut-over
    if (context_.HasPostponedCutOver()) {
        LOG_INFO("Cut-over is postponed. Waiting for flag...");
        // Wait until postponed flag is removed or force flag is set
        while (running_ && !cancelled_) {
            // Check flags
            // TODO: Implement flag file checking
            
            TimeUtils::SleepFor(1000);
        }
        
        if (cancelled_) return false;
    }
    
    // Execute cut-over
    context_.SetCutOverPhase(true);
    
    // Execute pre-cut-over hook
    if (hooks_executor_) {
        hooks_executor_->ExecuteHook("before-cut-over");
    }
    
    // Lock original table
    if (!applier_->LockOriginalTable()) {
        return false;
    }
    
    // Wait for remaining events
    // Process any remaining binlog events up to lock point
    
    // Rename tables
    if (!applier_->RenameTables()) {
        applier_->UnlockTables();
        return false;
    }
    
    // Unlock tables
    applier_->UnlockTables();
    
    // Execute post-cut-over hook
    if (hooks_executor_) {
        hooks_executor_->ExecuteHook("after-cut-over");
    }
    
    context_.SetCutOverPhase(false);
    
    return true;
}

bool Migrator::PhaseCleanup() {
    LOG_INFO("Phase: Cleanup");
    
    // Drop old table if configured
    if (context_.GetConfig().drop_original_table) {
        applier_->DropOldTable();
    }
    
    // Drop changelog table
    applier_->DropChangelogTable();
    
    // Stop streamer
    streamer_->Stop();
    
    // Stop heartbeat thread
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
    
    // Stop throttle thread
    if (throttle_thread_.joinable()) throttle_thread_.join();
    
    // Stop status server
    if (server_) {
        server_->Stop();
    }
    
    return true;
}

void Migrator::RunRowCopy() {
    LOG_INFO("Row copy thread started");
    
    std::string last_key = context_.MinKey();
    uint64_t chunk_size = context_.GetConfig().throttle.chunk_size;
    
    while (running_ && !cancelled_ && !paused_) {
        // Check if row copy is complete
        if (context_.CopiedRowCount() >= context_.TotalRowCount()) {
            row_copy_complete_ = true;
            LOG_INFO("Row copy complete");
            break;
        }
        
        // Throttle check
        while (ShouldThrottle()) {
            TimeUtils::SleepFor(100);
        }
        
        // Get next chunk
        std::string max_key = last_key;
        // Calculate chunk range
        
        // Copy rows
        if (!applier_->CopyRows(last_key, max_key, chunk_size)) {
            LOG_ERROR("Row copy failed");
            break;
        }
        
        // Update last key
        last_key = max_key;
        
        // Update progress
        UpdateStatus();
    }
    
    LOG_INFO("Row copy thread stopped");
}

void Migrator::RunEventsApply() {
    LOG_INFO("Events apply thread started");
    
    while (running_ && !cancelled_) {
        // Get event from queue
        auto entry = streamer_->GetEventQueue().PopFor(100);
        
        if (!entry) {
            if (row_copy_complete_) {
                // No more events and row copy complete
                all_events_up_to_lock_processed_ = true;
                break;
            }
            continue;
        }
        
        // Process event
        auto& entry_ptr = *entry;
        if (entry_ptr->IsDML() && entry_ptr->IsForTable(
                context_.DatabaseName(), context_.OriginalTableName())) {
            
            // Apply DML to ghost table
            auto dml_event = entry_ptr->DmlEvent();
            if (dml_event) {
                applier_->ApplyDMLEvent(dml_event);
            }
        }
        
        // Update current coordinates
        context_.SetCurrentBinlogCoordinates(entry_ptr->Coordinates());
        
        // Update statistics
        context_.AppliedEventCount()++;
    }
    
    LOG_INFO("Events apply thread stopped");
}

void Migrator::RunHeartbeat() {
    LOG_INFO("Heartbeat thread started");
    
    while (running_ && !cancelled_) {
        applier_->WriteHeartbeat();
        TimeUtils::SleepFor(context_.HeartbeatInterval());
    }
    
    LOG_INFO("Heartbeat thread stopped");
}

void Migrator::RunThrottleCheck() {
    LOG_INFO("Throttle check thread started");
    
    while (running_ && !cancelled_) {
        if (throttler_->ShouldThrottle()) {
            context_.SetThrottled(true);
            context_.SetThrottleReason(throttler_->GetReason());
            throttler_->ApplyThrottle();
        } else {
            context_.SetThrottled(false);
            context_.SetThrottleReason(ThrottleReason::None);
        }
        
        TimeUtils::SleepFor(context_.GetConfig().throttle.throttle_interval_millis);
    }
    
    LOG_INFO("Throttle check thread stopped");
}

bool Migrator::ShouldThrottle() const {
    return context_.IsThrottled();
}

bool Migrator::ApplyThrottle() {
    if (status_callback_) {
        status_callback_(context_.GetProgress());
    }
    
    LOG_INFO_FMT("Throttled: {}", 
                 Throttler::ReasonToString(context_.GetThrottleReason()));
    
    return true;
}

bool Migrator::CheckForPanicAbort() {
    // Check for panic abort flag file
    // TODO: Implement flag file checking
    return false;
}

void Migrator::UpdateStatus() {
    PrintStatus();
    
    if (status_callback_) {
        status_callback_(context_.GetProgress());
    }
}

void Migrator::PrintStatus() {
    auto progress = context_.GetProgress();
    
    std::ostringstream oss;
    oss << "Copy: " << progress.copied_rows << "/" << progress.total_rows;
    oss << " (" << std::fixed << std::setprecision(1) << progress.percentage_complete << "%)";
    oss << ", Applied: " << progress.events_applied;
    oss << ", ETA: " << TimeUtils::FormatDurationHuman(progress.estimated_remaining);
    
    LOG_INFO(oss.str());
}

void Migrator::HandleError(const std::string& error) {
    LOG_ERROR_FMT("Migration error: {}", error);
    context_.SetLastError(error);
    context_.SetStatus(MigrationStatus::Failed);
    context_.SetCurrentPhase(MigrationContext::Phase::Failed);
    
    // Execute failure hook
    if (hooks_executor_) {
        hooks_executor_->ExecuteHook("on-failure");
    }
    
    CleanupOnError();
}

void Migrator::CleanupOnError() {
    running_ = false;
    cancelled_ = true;
    
    // Stop all threads
    if (streamer_) streamer_->Stop();
    
    // Drop ghost table if needed
    if (ghost_table_migrated_ && context_.GetConfig().drop_ghost_table) {
        applier_->DropGhostTable();
    }
    
    // Drop changelog table
    applier_->DropChangelogTable();
}

void Migrator::Pause() {
    paused_ = true;
    context_.SetStatus(MigrationStatus::Paused);
    LOG_INFO("Migration paused");
}

void Migrator::Resume() {
    paused_ = false;
    context_.SetStatus(MigrationStatus::Running);
    LOG_INFO("Migration resumed");
}

void Migrator::Cancel() {
    cancelled_ = true;
    running_ = false;
    context_.SetStatus(MigrationStatus::Cancelled);
    
    // Join all threads
    if (row_copy_thread_.joinable()) row_copy_thread_.join();
    if (events_apply_thread_.joinable()) events_apply_thread_.join();
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
    if (throttle_thread_.joinable()) throttle_thread_.join();
    
    CleanupOnError();
    LOG_INFO("Migration cancelled");
}

void Migrator::Throttle() {
    context_.SetThrottled(true);
    context_.SetThrottleReason(ThrottleReason::UserCommand);
    LOG_INFO("User initiated throttle");
}

void Migrator::Unthrottle() {
    context_.SetThrottled(false);
    context_.SetThrottleReason(ThrottleReason::None);
    LOG_INFO("User initiated unthrottle");
}

void Migrator::PanicAbort() {
    LOG_CRITICAL("Panic abort initiated");
    context_.SetStatus(MigrationStatus::Cancelled);
    
    // Execute failure hook
    if (hooks_executor_) {
        hooks_executor_->ExecuteHook("on-failure");
    }
    
    Cancel();
}

void Migrator::InitiateCutOver() {
    // Remove postpone flag if exists
    LOG_INFO("Initiating cut-over");
}

void Migrator::PostponeCutOver() {
    context_.GetRuntimeConfig().paused = true;
    LOG_INFO("Cut-over postponed");
}

void Migrator::ForceCutOver() {
    LOG_INFO("Force cut-over initiated");
    // Remove postpone flag and initiate cut-over immediately
}

MigrationStatus Migrator::GetStatus() const {
    return context_.GetStatus();
}

MigrationProgress Migrator::GetProgress() const {
    return context_.GetProgress();
}

void Migrator::SetStatusCallback(StatusCallback callback) {
    status_callback_ = callback;
}

} // namespace gh_ost