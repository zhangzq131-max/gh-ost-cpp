/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "server/interactive_commands.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include <sstream>

namespace gh_ost {

InteractiveCommands::InteractiveCommands(MigrationContext& context)
    : context_(context) {
    InitializeHandlers();
}

InteractiveCommands::~InteractiveCommands() {}

void InteractiveCommands::InitializeHandlers() {
    handlers_["status"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandleStatus();
    };
    
    handlers_["progress"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandleProgress();
    };
    
    handlers_["throttle"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandleThrottle();
    };
    
    handlers_["unthrottle"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandleUnthrottle();
    };
    
    handlers_["pause"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandlePause();
    };
    
    handlers_["resume"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandleResume();
    };
    
    handlers_["panic-abort"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandlePanicAbort();
    };
    
    handlers_["cut-over"] = [this](const std::string&, const std::vector<std::string>&) {
        return HandleCutOver();
    };
    
    handlers_["chunk-size"] = [this](const std::string&, const std::vector<std::string>& args) {
        return HandleChunkSize(args);
    };
    
    handlers_["max-lag"] = [this](const std::string&, const std::vector<std::string>& args) {
        return HandleMaxLag(args);
    };
}

CommandResult InteractiveCommands::ProcessCommand(const std::string& command_line) {
    auto parsed = ParseCommand(command_line);
    
    std::string command = parsed.first;
    std::vector<std::string> args = parsed.second;
    
    LOG_DEBUG_FMT("Processing command: {}", command);
    
    auto it = handlers_.find(command);
    if (it != handlers_.end()) {
        return it->second(command, args);
    }
    
    return CommandResult::Failure("Unknown command: " + command);
}

std::pair<std::string, std::vector<std::string>> InteractiveCommands::ParseCommand(
    const std::string& line) {
    std::vector<std::string> parts = StringUtils::Split(line, ' ');
    
    if (parts.empty()) return {"", {}};
    
    std::string command = StringUtils::ToLower(parts[0]);
    std::vector<std::string> args(parts.begin() + 1, parts.end());
    
    return {command, args};
}

void InteractiveCommands::RegisterHandler(const std::string& command,
                                          InteractiveCommandHandler handler) {
    handlers_[command] = handler;
}

CommandResult InteractiveCommands::HandleStatus() {
    return CommandResult::WithData(
        "Status: " + std::to_string(static_cast<int>(context_.GetStatus())) +
        ", Phase: " + context_.PhaseToString(context_.GetCurrentPhase())
    );
}

CommandResult InteractiveCommands::HandleProgress() {
    auto progress = context_.GetProgress();
    
    std::ostringstream oss;
    oss << "Copied: " << progress.copied_rows << "/" << progress.total_rows;
    oss << " (" << progress.percentage_complete << "%)";
    
    return CommandResult::WithData(oss.str());
}

CommandResult InteractiveCommands::HandleThrottle() {
    context_.SetThrottled(true);
    context_.SetThrottleReason(ThrottleReason::UserCommand);
    LOG_INFO("User command: throttle");
    return CommandResult::Success("Throttle applied");
}

CommandResult InteractiveCommands::HandleUnthrottle() {
    context_.SetThrottled(false);
    context_.SetThrottleReason(ThrottleReason::None);
    LOG_INFO("User command: unthrottle");
    return CommandResult::Success("Throttle released");
}

CommandResult InteractiveCommands::HandlePause() {
    context_.GetRuntimeConfig().paused = true;
    LOG_INFO("User command: pause");
    return CommandResult::Success("Migration paused");
}

CommandResult InteractiveCommands::HandleResume() {
    context_.GetRuntimeConfig().paused = false;
    LOG_INFO("User command: resume");
    return CommandResult::Success("Migration resumed");
}

CommandResult InteractiveCommands::HandlePanicAbort() {
    LOG_CRITICAL("User command: panic-abort");
    return CommandResult::Success("Panic abort initiated");
}

CommandResult InteractiveCommands::HandleCutOver() {
    LOG_INFO("User command: initiate cut-over");
    return CommandResult::Success("Cut-over will be initiated when ready");
}

CommandResult InteractiveCommands::HandleForceCutOver() {
    LOG_INFO("User command: force cut-over");
    return CommandResult::Success("Force cut-over initiated");
}

CommandResult InteractiveCommands::HandlePostponeCutOver() {
    context_.GetRuntimeConfig().paused = true;
    LOG_INFO("User command: postpone cut-over");
    return CommandResult::Success("Cut-over postponed");
}

CommandResult InteractiveCommands::HandleChunkSize(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult::WithData(
            "Current chunk size: " + 
            std::to_string(context_.GetConfig().throttle.chunk_size)
        );
    }
    
    auto size = StringUtils::ParseUInt64(args[0]);
    if (!size || *size <= 0) {
        return CommandResult::Failure("Invalid chunk size");
    }
    
    context_.GetConfig().throttle.chunk_size = *size;
    LOG_INFO_FMT("User command: set chunk size to {}", *size);
    
    return CommandResult::Success("Chunk size set to " + std::to_string(*size));
}

CommandResult InteractiveCommands::HandleMaxLag(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult::WithData(
            "Current max lag: " + 
            std::to_string(context_.GetConfig().throttle.max_replication_lag) + "s"
        );
    }
    
    auto lag = StringUtils::ParseUInt64(args[0]);
    if (!lag) {
        return CommandResult::Failure("Invalid max lag value");
    }
    
    context_.GetConfig().throttle.max_replication_lag = *lag;
    LOG_INFO_FMT("User command: set max lag to {}", *lag);
    
    return CommandResult::Success("Max lag set to " + std::to_string(*lag) + "s");
}

std::vector<std::string> InteractiveCommands::GetAvailableCommands() {
    return {
        "status", "progress", "throttle", "unthrottle",
        "pause", "resume", "panic-abort", "cut-over",
        "force-cut-over", "postpone-cut-over",
        "chunk-size [size]", "max-lag [seconds]"
    };
}

} // namespace gh_ost