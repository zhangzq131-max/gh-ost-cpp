/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_HOOKS_EXECUTOR_HPP
#define GH_OST_HOOKS_EXECUTOR_HPP

#include "context/migration_context.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace gh_ost {

// Hook execution result
struct HookResult {
    bool success;
    std::string output;
    std::string error;
    uint64_t duration_ms;
    
    HookResult() : success(false), duration_ms(0) {}
    
    static HookResult Success(const std::string& output = "") {
        HookResult result;
        result.success = true;
        result.output = output;
        return result;
    }
    
    static HookResult Failure(const std::string& error) {
        HookResult result;
        result.error = error;
        return result;
    }
};

// Hooks executor - runs external scripts for migration events
class HooksExecutor {
public:
    HooksExecutor(MigrationContext& context);
    ~HooksExecutor();
    
    // Initialize hooks from configuration
    bool Initialize();
    
    // Execute a specific hook
    HookResult ExecuteHook(const std::string& hook_name);
    
    // Execute hook with arguments
    HookResult ExecuteHook(const std::string& hook_name, 
                           const std::vector<std::string>& args);
    
    // Available hooks
    bool ExecuteBeforeMigration();
    bool ExecuteAfterMigration();
    bool ExecuteBeforeCutOver();
    bool ExecuteAfterCutOver();
    bool ExecuteOnStatus();
    bool ExecuteOnThrottle();
    bool ExecuteOnUnthrottle();
    bool ExecuteOnFailure();
    bool ExecuteOnSuccess();
    
    // Hook timeout
    void SetTimeout(uint64_t timeout_seconds);
    
    // Check if hook exists
    bool HasHook(const std::string& hook_name) const;
    
    // Get hook path
    std::optional<std::string> GetHookPath(const std::string& hook_name) const;
    
    // Environment variables for hooks
    std::vector<std::string> GetHookEnvironment() const;
    
private:
    MigrationContext& context_;
    
    std::unordered_map<std::string, std::string> hooks_;
    
    uint64_t timeout_seconds_;
    
    // Execute external process
    HookResult ExecuteProcess(const std::string& path,
                              const std::vector<std::string>& args,
                              const std::vector<std::string>& env);
    
    // Build hook arguments
    std::vector<std::string> BuildHookArgs(const std::string& hook_name);
    
    // Build environment for hook
    std::vector<std::string> BuildEnvironment() const;
};

} // namespace gh_ost

#endif // GH_OST_HOOKS_EXECUTOR_HPP