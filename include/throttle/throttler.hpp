/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_THROTTLER_HPP
#define GH_OST_THROTTLER_HPP

#include "context/migration_context.hpp"
#include "mysql/connection.hpp"
#include <memory>
#include <atomic>
#include <string>
#include <cstdint>

namespace gh_ost {

class Throttler {
public:
    Throttler(MigrationContext& context);
    ~Throttler();
    
    // Initialize
    bool Initialize();
    
    // Check if should throttle
    bool ShouldThrottle();
    
    // Apply throttle (pause operations)
    void ApplyThrottle();
    
    // Release throttle
    void ReleaseThrottle();
    
    // Get throttle reason
    ThrottleReason GetReason() const { return reason_; }
    
    // Reason to string
    static std::string ReasonToString(ThrottleReason reason);
    
    // Check specific conditions
    bool CheckReplicationLag();
    bool CheckMaxLoad();
    bool CheckCriticalLoad();
    bool CheckCustomQuery();
    
    // Get current metrics
    uint64_t GetReplicationLagSeconds();
    uint64_t GetThreadsRunning();
    uint64_t GetCurrentLoad();
    
    // Control replicas for throttle check
    void SetControlReplicas(const std::vector<InstanceKey>& replicas);
    
    // HTTP throttle
    void SetHTTPThrottleURL(const std::string& url);
    bool CheckHTTPThrottle();
    
    // Update throttle interval
    void SetInterval(uint64_t millis);
    
    // Statistics
    uint64_t ThrottleCount() const { return throttle_count_; }
    uint64_t TotalThrottleTimeMs() const { return total_throttle_time_ms_; }

private:
    MigrationContext& context_;
    std::vector<std::shared_ptr<Connection>> replica_connections_;
    
    ThrottleReason reason_;
    std::atomic<bool> throttled_;
    
    uint64_t throttle_count_;
    uint64_t total_throttle_time_ms_;
    
    // Last check results
    uint64_t last_replication_lag_;
    uint64_t last_threads_running_;
    
    // HTTP throttle
    std::string http_url_;
    
    // Calculate exponential backoff
    uint64_t CalculateBackoffMs();
    
    // Check all conditions
    bool CheckAllConditions();
};

} // namespace gh_ost

#endif // GH_OST_THROTTLER_HPP