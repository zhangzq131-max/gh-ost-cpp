/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_MIGRATOR_HPP
#define GH_OST_MIGRATOR_HPP

#include "context/migration_context.hpp"
#include "logic/inspector.hpp"
#include "logic/applier.hpp"
#include "logic/streamer.hpp"
#include "logic/checkpoint.hpp"
#include "throttle/throttler.hpp"
#include "hooks/hooks_executor.hpp"
#include "server/server.hpp"
#include "utils/thread_pool.hpp"
#include "utils/safe_queue.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

namespace gh_ost {

// Main migration flow manager
class Migrator {
public:
    Migrator();
    ~Migrator();
    
    // Initialize with configuration
    bool Initialize(const MigrationConfig& config);
    
    // Run migration
    bool Run();
    
    // Control
    void Pause();
    void Resume();
    void Cancel();
    
    // Status
    MigrationStatus GetStatus() const;
    MigrationProgress GetProgress() const;
    
    // Interactive commands
    void Throttle();
    void Unthrottle();
    void PanicAbort();
    
    // Cut-over control
    void InitiateCutOver();
    void PostponeCutOver();
    void ForceCutOver();
    
    // Events
    using StatusCallback = std::function<void(const MigrationProgress&)>;
    void SetStatusCallback(StatusCallback callback);
    
    // Context access
    MigrationContext& GetContext() { return context_; }
    
private:
    MigrationContext context_;
    
    std::unique_ptr<Inspector> inspector_;
    std::unique_ptr<Applier> applier_;
    std::unique_ptr<EventsStreamer> streamer_;
    std::unique_ptr<Throttler> throttler_;
    std::unique_ptr<HooksExecutor> hooks_executor_;
    std::unique_ptr<Server> server_;
    
    std::unique_ptr<ThreadPool> thread_pool_;
    
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<bool> cancelled_;
    
    StatusCallback status_callback_;
    
    // Migration phases
    bool PhaseInitialSetup();
    bool PhaseCreateGhostTable();
    bool PhaseAlterGhostTable();
    bool PhaseInspectOriginalTable();
    bool PhaseValidateMigration();
    bool PhasePopulateRowRange();
    bool PhaseReadStreamInitiate();
    bool PhaseRowCopy();
    bool PhaseCutOver();
    bool PhaseCleanup();
    
    // Internal state channels
    std::atomic<bool> first_throttling_collected_;
    std::atomic<bool> ghost_table_migrated_;
    std::atomic<bool> row_copy_complete_;
    std::atomic<bool> all_events_up_to_lock_processed_;
    
    // Work queues
    SafeQueue<std::function<void()>> copy_rows_queue_;
    SafeQueue<std::function<void()>> apply_events_queue_;
    
    // Threads
    std::thread row_copy_thread_;
    std::thread events_apply_thread_;
    std::thread heartbeat_thread_;
    std::thread throttle_thread_;
    
    // Internal methods
    void RunRowCopy();
    void RunEventsApply();
    void RunHeartbeat();
    void RunThrottleCheck();
    
    bool ShouldThrottle() const;
    bool ApplyThrottle();
    
    bool ReadChangelogState();
    void WriteChangelogState(const std::string& hint, const std::string& value);
    
    bool CheckForPanicAbort();
    
    // Status updates
    void UpdateStatus();
    void PrintStatus();
    
    // Error handling
    void HandleError(const std::string& error);
    void CleanupOnError();
};

} // namespace gh_ost

#endif // GH_OST_MIGRATOR_HPP