/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "throttle/throttler.hpp"
#include "utils/logger.hpp"
#include "utils/time_utils.hpp"
#include "mysql/utils.hpp"
#include <sstream>

namespace gh_ost {

Throttler::Throttler(MigrationContext& context)
    : context_(context)
    , reason_(ThrottleReason::None)
    , throttled_(false)
    , throttle_count_(0)
    , total_throttle_time_ms_(0)
    , last_replication_lag_(0)
    , last_threads_running_(0) {
}

Throttler::~Throttler() {}

bool Throttler::Initialize() {
    LOG_INFO("Throttler initialized");
    return true;
}

bool Throttler::ShouldThrottle() {
    return CheckAllConditions();
}

void Throttler::ApplyThrottle() {
    throttled_ = true;
    throttle_count_++;
    
    LOG_INFO_FMT("Throttling applied: {}", ReasonToString(reason_));
}

void Throttler::ReleaseThrottle() {
    throttled_ = false;
    reason_ = ThrottleReason::None;
    
    LOG_INFO("Throttle released");
}

std::string Throttler::ReasonToString(ThrottleReason reason) {
    switch (reason) {
        case ThrottleReason::None: return "None";
        case ThrottleReason::ReplicationLag: return "Replication Lag";
        case ThrottleReason::MaxLoad: return "Max Load";
        case ThrottleReason::CriticalLoad: return "Critical Load";
        case ThrottleReason::UserCommand: return "User Command";
        case ThrottleReason::Heartbeat: return "Heartbeat";
        case ThrottleReason::PostponedCutOver: return "Postponed Cut-Over";
        default: return "Unknown";
    }
}

bool Throttler::CheckReplicationLag() {
    auto config = context_.GetConfig();
    
    if (config.throttle.max_replication_lag <= 0) {
        return false;
    }
    
    uint64_t lag = GetReplicationLagSeconds();
    last_replication_lag_ = lag;
    
    if (lag > config.throttle.max_replication_lag) {
        reason_ = ThrottleReason::ReplicationLag;
        return true;
    }
    
    return false;
}

bool Throttler::CheckMaxLoad() {
    auto config = context_.GetConfig();
    
    if (!config.throttle.max_load_threads_running) {
        return false;
    }
    
    uint64_t threads = GetThreadsRunning();
    last_threads_running_ = threads;
    
    if (threads > config.throttle.max_load_threads_running) {
        reason_ = ThrottleReason::MaxLoad;
        return true;
    }
    
    return false;
}

bool Throttler::CheckCriticalLoad() {
    auto config = context_.GetConfig();
    
    if (!config.throttle.critical_load_threads_running) {
        return false;
    }
    
    uint64_t threads = GetThreadsRunning();
    
    if (threads > config.throttle.critical_load_threads_running) {
        reason_ = ThrottleReason::CriticalLoad;
        LOG_WARN_FMT("CRITICAL: Threads running {}", threads);
        return true;
    }
    
    return false;
}

bool Throttler::CheckCustomQuery() {
    auto config = context_.GetConfig();
    
    if (!config.throttle.throttle_query) {
        return false;
    }
    
    auto conn = context_.MasterConnection();
    if (!conn) return false;
    
    auto result = conn->QueryValue(*config.throttle.throttle_query);
    
    if (result) {
        auto val = StringUtils::ParseUInt64(*result);
        if (val && *val > 0) {
            return true;
        }
    }
    
    return false;
}

uint64_t Throttler::GetReplicationLagSeconds() {
    uint64_t max_lag = 0;
    
    auto conn = context_.ReplicaConnection();
    if (conn && conn->IsConnected()) {
        max_lag = MySQLUtils::GetReplicationLagSeconds(*conn);
    }
    
    for (auto& replica_conn : replica_connections_) {
        if (replica_conn && replica_conn->IsConnected()) {
            uint64_t lag = MySQLUtils::GetReplicationLagSeconds(*replica_conn);
            max_lag = std::max(max_lag, lag);
        }
    }
    
    return max_lag;
}

uint64_t Throttler::GetThreadsRunning() {
    auto conn = context_.MasterConnection();
    if (!conn) return 0;
    
    return conn->GetThreadsRunning();
}

uint64_t Throttler::GetCurrentLoad() {
    return GetThreadsRunning();
}

void Throttler::SetControlReplicas(const std::vector<InstanceKey>& replicas) {
    replica_connections_.clear();
    
    for (const auto& key : replicas) {
        auto conn = std::make_shared<Connection>();
        if (conn->Connect(context_.GetConfig().replica_connection)) {
            replica_connections_.push_back(conn);
        }
    }
    
    LOG_INFO_FMT("Control replicas set: {} connections", replica_connections_.size());
}

void Throttler::SetHTTPThrottleURL(const std::string& url) {
    http_url_ = url;
}

bool Throttler::CheckHTTPThrottle() {
    if (http_url_.empty()) return false;
    return false;
}

void Throttler::SetInterval(uint64_t millis) {
    context_.GetConfig().throttle.throttle_interval_millis = millis;
}

uint64_t Throttler::CalculateBackoffMs() {
    uint64_t base_ms = context_.GetConfig().throttle.throttle_interval_millis;
    uint64_t max_ms = context_.GetConfig().cut_over.cut_over_exponential_backoff_max_wait_seconds * 1000;
    
    uint64_t backoff = base_ms;
    for (uint64_t i = 0; i < throttle_count_; ++i) {
        backoff *= 2;
        if (backoff >= max_ms) {
            backoff = max_ms;
            break;
        }
    }
    
    return backoff;
}

bool Throttler::CheckAllConditions() {
    if (CheckCriticalLoad()) return true;
    if (CheckMaxLoad()) return true;
    if (CheckReplicationLag()) return true;
    if (CheckCustomQuery()) return true;
    if (CheckHTTPThrottle()) return true;
    
    if (context_.IsThrottled() && 
        context_.GetThrottleReason() == ThrottleReason::UserCommand) {
        reason_ = ThrottleReason::UserCommand;
        return true;
    }
    
    return false;
}

} // namespace gh_ost