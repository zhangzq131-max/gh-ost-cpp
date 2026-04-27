/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_LOGGER_HPP
#define GH_OST_LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>
#include <vector>

namespace gh_ost {

// Log level enum matching spdlog
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

// Logger class wrapper for spdlog
class Logger {
public:
    // Get singleton instance
    static Logger& Instance();
    
    // Initialize logger
    void Initialize(const std::string& name = "gh-ost",
                    LogLevel level = LogLevel::Info,
                    const std::string& log_file = "",
                    bool rotate = true,
                    size_t max_size_mb = 10,
                    size_t max_files = 3);
    
    // Shutdown logger
    void Shutdown();
    
    // Set log level
    void SetLevel(LogLevel level);
    
    // Get current log level
    LogLevel GetLevel() const;
    
    // Set pattern
    void SetPattern(const std::string& pattern);
    
    // Enable/disable console output
    void EnableConsole(bool enable);
    
    // Enable/disable file output
    void EnableFile(bool enable, const std::string& filename = "",
                    bool rotate = true, size_t max_size_mb = 10, size_t max_files = 3);
    
    // Log methods
    void Trace(const std::string& msg);
    void Debug(const std::string& msg);
    void Info(const std::string& msg);
    void Warn(const std::string& msg);
    void Error(const std::string& msg);
    void Critical(const std::string& msg);
    
    // Format log methods
    template<typename... Args>
    void TraceFmt(const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->trace(fmt, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void DebugFmt(const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->debug(fmt, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void InfoFmt(const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->info(fmt, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void WarnFmt(const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->warn(fmt, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void ErrorFmt(const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->error(fmt, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void CriticalFmt(const std::string& fmt, Args&&... args) {
        if (logger_) {
            logger_->critical(fmt, std::forward<Args>(args)...);
        }
    }
    
    // Flush logs
    void Flush();
    
    // Flush on specific level
    void FlushOn(LogLevel level);
    
    // Get underlying spdlog logger
    std::shared_ptr<spdlog::logger> GetLogger() const { return logger_; }
    
private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Convert LogLevel to spdlog level
    spdlog::level::level_enum ToSpdlogLevel(LogLevel level) const;
    LogLevel FromSpdlogLevel(spdlog::level::level_enum level) const;
    
    std::shared_ptr<spdlog::logger> logger_;
    std::vector<spdlog::sink_ptr> sinks_;
    bool console_enabled_;
    bool file_enabled_;
    LogLevel current_level_;
};

// Global logging macros for convenience
#define LOG_TRACE(msg)    gh_ost::Logger::Instance().Trace(msg)
#define LOG_DEBUG(msg)    gh_ost::Logger::Instance().Debug(msg)
#define LOG_INFO(msg)     gh_ost::Logger::Instance().Info(msg)
#define LOG_WARN(msg)     gh_ost::Logger::Instance().Warn(msg)
#define LOG_ERROR(msg)    gh_ost::Logger::Instance().Error(msg)
#define LOG_CRITICAL(msg) gh_ost::Logger::Instance().Critical(msg)

// Format macros
#define LOG_TRACE_FMT(fmt, ...)    gh_ost::Logger::Instance().TraceFmt(fmt, __VA_ARGS__)
#define LOG_DEBUG_FMT(fmt, ...)    gh_ost::Logger::Instance().DebugFmt(fmt, __VA_ARGS__)
#define LOG_INFO_FMT(fmt, ...)     gh_ost::Logger::Instance().InfoFmt(fmt, __VA_ARGS__)
#define LOG_WARN_FMT(fmt, ...)     gh_ost::Logger::Instance().WarnFmt(fmt, __VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...)    gh_ost::Logger::Instance().ErrorFmt(fmt, __VA_ARGS__)
#define LOG_CRITICAL_FMT(fmt, ...) gh_ost::Logger::Instance().CriticalFmt(fmt, __VA_ARGS__)

} // namespace gh_ost

#endif // GH_OST_LOGGER_HPP