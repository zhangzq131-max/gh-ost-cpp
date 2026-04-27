/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "hooks/hooks_executor.hpp"
#include "utils/logger.hpp"
#include "utils/time_utils.hpp"
#include "utils/file_utils.hpp"
#include "utils/string_utils.hpp"
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace gh_ost {

HooksExecutor::HooksExecutor(MigrationContext& context)
    : context_(context)
    , timeout_seconds_(30) {
}

HooksExecutor::~HooksExecutor() {}

bool HooksExecutor::Initialize() {
    auto config = context_.GetConfig();
    
    if (config.hooks.hook_before_migration) {
        hooks_["before-migration"] = *config.hooks.hook_before_migration;
    }
    
    if (config.hooks.hook_after_migration) {
        hooks_["after-migration"] = *config.hooks.hook_after_migration;
    }
    
    if (config.hooks.hook_before_cut_over) {
        hooks_["before-cut-over"] = *config.hooks.hook_before_cut_over;
    }
    
    if (config.hooks.hook_after_cut_over) {
        hooks_["after-cut-over"] = *config.hooks.hook_after_cut_over;
    }
    
    if (config.hooks.hook_on_status) {
        hooks_["on-status"] = *config.hooks.hook_on_status;
    }
    
    if (config.hooks.hook_on_throttle) {
        hooks_["on-throttle"] = *config.hooks.hook_on_throttle;
    }
    
    if (config.hooks.hook_on_unthrottle) {
        hooks_["on-unthrottle"] = *config.hooks.hook_on_unthrottle;
    }
    
    if (config.hooks.hook_on_failure) {
        hooks_["on-failure"] = *config.hooks.hook_on_failure;
    }
    
    if (config.hooks.hook_on_success) {
        hooks_["on-success"] = *config.hooks.hook_on_success;
    }
    
    timeout_seconds_ = config.hooks.hook_timeout_seconds;
    
    LOG_INFO_FMT("Hooks initialized: {} hooks registered", hooks_.size());
    
    return true;
}

HookResult HooksExecutor::ExecuteHook(const std::string& hook_name) {
    return ExecuteHook(hook_name, {});
}

HookResult HooksExecutor::ExecuteHook(const std::string& hook_name,
                                       const std::vector<std::string>& args) {
    auto path = GetHookPath(hook_name);
    
    if (!path) {
        LOG_DEBUG_FMT("Hook {} not registered", hook_name);
        return HookResult::Success();  // No hook configured
    }
    
    // Check if hook file exists
    if (!FileUtils::IsExecutable(*path)) {
        LOG_WARN_FMT("Hook file {} not executable", *path);
        return HookResult::Failure("Hook file not executable: " + *path);
    }
    
    LOG_INFO_FMT("Executing hook: {} ({})", hook_name, *path);
    
    // Build arguments
    std::vector<std::string> full_args = BuildHookArgs(hook_name);
    for (const auto& arg : args) {
        full_args.push_back(arg);
    }
    
    // Build environment
    std::vector<std::string> env = BuildEnvironment();
    
    // Execute process
    auto result = ExecuteProcess(*path, full_args, env);
    
    if (result.success) {
        LOG_INFO_FMT("Hook {} completed successfully", hook_name);
    } else {
        LOG_ERROR_FMT("Hook {} failed: {}", hook_name, result.error);
        
        auto config = context_.GetConfig();
        if (config.hooks.hook_fail_on_error) {
            // Mark migration as failed
            context_.SetLastError("Hook " + hook_name + " failed: " + result.error);
        }
    }
    
    return result;
}

bool HooksExecutor::ExecuteBeforeMigration() {
    auto result = ExecuteHook("before-migration");
    return result.success;
}

bool HooksExecutor::ExecuteAfterMigration() {
    auto result = ExecuteHook("after-migration");
    return result.success;
}

bool HooksExecutor::ExecuteBeforeCutOver() {
    auto result = ExecuteHook("before-cut-over");
    return result.success;
}

bool HooksExecutor::ExecuteAfterCutOver() {
    auto result = ExecuteHook("after-cut-over");
    return result.success;
}

bool HooksExecutor::ExecuteOnStatus() {
    auto result = ExecuteHook("on-status");
    return result.success;
}

bool HooksExecutor::ExecuteOnThrottle() {
    auto result = ExecuteHook("on-throttle");
    return result.success;
}

bool HooksExecutor::ExecuteOnUnthrottle() {
    auto result = ExecuteHook("on-unthrottle");
    return result.success;
}

bool HooksExecutor::ExecuteOnFailure() {
    auto result = ExecuteHook("on-failure");
    return result.success;
}

bool HooksExecutor::ExecuteOnSuccess() {
    auto result = ExecuteHook("on-success");
    return result.success;
}

void HooksExecutor::SetTimeout(uint64_t timeout_seconds) {
    timeout_seconds_ = timeout_seconds;
}

bool HooksExecutor::HasHook(const std::string& hook_name) const {
    return hooks_.find(hook_name) != hooks_.end();
}

std::optional<std::string> HooksExecutor::GetHookPath(const std::string& hook_name) const {
    auto it = hooks_.find(hook_name);
    if (it != hooks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> HooksExecutor::GetHookEnvironment() const {
    return BuildEnvironment();
}

HookResult HooksExecutor::ExecuteProcess(const std::string& path,
                                         const std::vector<std::string>& args,
                                         const std::vector<std::string>& env) {
    HookResult result;
    
    TimeUtils::ElapsedTimer timer;
    timer.Start();
    
#ifdef _WIN32
    // Windows implementation
    std::string cmd = path;
    for (const auto& arg : args) {
        cmd += " " + arg;
    }
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    // Create process
    if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), 
                        NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        result.error = "Failed to create process";
        return result;
    }
    
    // Wait for process
    uint64_t timeout_ms = timeout_seconds_ * 1000;
    DWORD wait_result = WaitForSingleObject(pi.hProcess, timeout_ms);
    
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        result.error = "Hook timeout";
    } else if (wait_result == WAIT_OBJECT_0) {
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        result.success = exit_code == 0;
        if (!result.success) {
            result.error = "Hook returned non-zero exit code: " + std::to_string(exit_code);
        }
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
#else
    // Unix implementation
    pid_t pid = fork();
    
    if (pid < 0) {
        result.error = "Failed to fork process";
        return result;
    }
    
    if (pid == 0) {
        // Child process
        // Set environment
        for (const auto& e : env) {
            auto parts = StringUtils::Split(e, '=');
            if (parts.size() == 2) {
                setenv(parts[0].c_str(), parts[1].c_str(), 1);
            }
        }
        
        // Build argv
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(path.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execv(path.c_str(), argv.data());
        exit(127);  // exec failed
    }
    
    // Parent process - wait for child
    int status;
    uint64_t start_time = TimeUtils::NowUnixMillis();
    
    while (TimeUtils::NowUnixMillis() - start_time < timeout_seconds_ * 1000) {
        int wait_result = waitpid(pid, &status, WNOHANG);
        
        if (wait_result == pid) {
            result.success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            if (!result.success) {
                result.error = "Hook returned exit code: " + std::to_string(WEXITSTATUS(status));
            }
            break;
        }
        
        TimeUtils::SleepFor(100);
    }
    
    if (TimeUtils::NowUnixMillis() - start_time >= timeout_seconds_ * 1000) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        result.error = "Hook timeout";
    }
#endif
    
    timer.Stop();
    result.duration_ms = timer.ElapsedMillis();
    
    return result;
}

std::vector<std::string> HooksExecutor::BuildHookArgs(const std::string& hook_name) {
    std::vector<std::string> args;
    
    args.push_back(context_.DatabaseName());
    args.push_back(context_.OriginalTableName());
    args.push_back(context_.GhostTableName());
    args.push_back(hook_name);
    
    return args;
}

std::vector<std::string> HooksExecutor::BuildEnvironment() const {
    std::vector<std::string> env;
    
    env.push_back("GH_OST_DATABASE=" + context_.DatabaseName());
    env.push_back("GH_OST_TABLE=" + context_.OriginalTableName());
    env.push_back("GH_OST_GHOST_TABLE=" + context_.GhostTableName());
    env.push_back("GH_OST_CHANGELOG_TABLE=" + context_.ChangelogTableName());
    env.push_back("GH_OST_PHASE=" + context_.PhaseToString(context_.GetCurrentPhase()));
    env.push_back("GH_OST_STATUS=" + std::to_string(static_cast<int>(context_.GetStatus())));
    
    auto progress = context_.GetProgress();
    env.push_back("GH_OST_PROGRESS=" + StringUtils::ToString(progress.percentage_complete));
    env.push_back("GH_OST_TOTAL_ROWS=" + StringUtils::ToString(progress.total_rows));
    env.push_back("GH_OST_COPIED_ROWS=" + StringUtils::ToString(progress.copied_rows));
    
    return env;
}

} // namespace gh_ost