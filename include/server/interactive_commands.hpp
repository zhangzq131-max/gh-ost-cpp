/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_INTERACTIVE_COMMANDS_HPP
#define GH_OST_INTERACTIVE_COMMANDS_HPP

#include "context/migration_context.hpp"
#include "gh_ost/types.hpp"
#include <string>
#include <functional>
#include <optional>
#include <vector>

namespace gh_ost {

struct CommandResult {
    bool success;
    std::string message;
    std::string data;
    
    CommandResult() : success(false) {}
    CommandResult(bool s, const std::string& msg) : success(s), message(msg) {}
    
    static CommandResult Success(const std::string& msg = "OK") {
        return CommandResult(true, msg);
    }
    
    static CommandResult Failure(const std::string& msg) {
        return CommandResult(false, msg);
    }
    
    static CommandResult WithData(const std::string& data) {
        CommandResult result(true, "OK");
        result.data = data;
        return result;
    }
};

using InteractiveCommandHandler = std::function<CommandResult(const std::string& command,
                                                              const std::vector<std::string>& args)>;

class InteractiveCommands {
public:
    InteractiveCommands(MigrationContext& context);
    ~InteractiveCommands();
    
    CommandResult ProcessCommand(const std::string& command_line);
    
    void RegisterHandler(const std::string& command, InteractiveCommandHandler handler);
    
    CommandResult HandleStatus();
    CommandResult HandleProgress();
    CommandResult HandleThrottle();
    CommandResult HandleUnthrottle();
    CommandResult HandlePause();
    CommandResult HandleResume();
    CommandResult HandlePanicAbort();
    CommandResult HandleCutOver();
    CommandResult HandleForceCutOver();
    CommandResult HandlePostponeCutOver();
    CommandResult HandleChunkSize(const std::vector<std::string>& args);
    CommandResult HandleMaxLag(const std::vector<std::string>& args);
    
    static std::vector<std::string> GetAvailableCommands();
    
private:
    MigrationContext& context_;
    
    std::unordered_map<std::string, InteractiveCommandHandler> handlers_;
    
    std::pair<std::string, std::vector<std::string>> ParseCommand(const std::string& line);
    
    void InitializeHandlers();
};

} // namespace gh_ost

#endif // GH_OST_INTERACTIVE_COMMANDS_HPP