/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "utils/logger.hpp"

namespace gh_ost {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : console_enabled_(false)
    , file_enabled_(false)
    , current_level_(LogLevel::Info) {
    // Create default console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    sinks_.push_back(console_sink);
    
    // Create default logger
    logger_ = std::make_shared<spdlog::logger>("gh-ost", sinks_.begin(), sinks_.end());
    logger_->set_level(spdlog::level::info);
    spdlog::register_logger(logger_);
    console_enabled_ = true;
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Initialize(const std::string& name, LogLevel level,
                        const std::string& log_file, bool rotate,
                        size_t max_size_mb, size_t max_files) {
    Shutdown();
    
    sinks_.clear();
    
    // Console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    sinks_.push_back(console_sink);
    console_enabled_ = true;
    
    // File sink
    if (!log_file.empty()) {
        if (rotate) {
            size_t max_size = max_size_mb * 1024 * 1024;
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file, max_size, max_files);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks_.push_back(file_sink);
        } else {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks_.push_back(file_sink);
        }
        file_enabled_ = true;
    }
    
    // Create logger
    logger_ = std::make_shared<spdlog::logger>(name, sinks_.begin(), sinks_.end());
    logger_->set_level(ToSpdlogLevel(level));
    current_level_ = level;
    
    spdlog::register_logger(logger_);
    spdlog::set_default_logger(logger_);
}

void Logger::Shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::drop(logger_->name());
    }
    spdlog::shutdown();
}

void Logger::SetLevel(LogLevel level) {
    if (logger_) {
        logger_->set_level(ToSpdlogLevel(level));
        current_level_ = level;
    }
}

LogLevel Logger::GetLevel() const {
    return current_level_;
}

void Logger::SetPattern(const std::string& pattern) {
    if (logger_) {
        logger_->set_pattern(pattern);
    }
}

void Logger::EnableConsole(bool enable) {
    console_enabled_ = enable;
    // Note: Would need to recreate logger to actually enable/disable
}

void Logger::EnableFile(bool enable, const std::string& filename,
                        bool rotate, size_t max_size_mb, size_t max_files) {
    file_enabled_ = enable;
    // Note: Would need to recreate logger to actually enable/disable
}

void Logger::Trace(const std::string& msg) {
    if (logger_) logger_->trace(msg);
}

void Logger::Debug(const std::string& msg) {
    if (logger_) logger_->debug(msg);
}

void Logger::Info(const std::string& msg) {
    if (logger_) logger_->info(msg);
}

void Logger::Warn(const std::string& msg) {
    if (logger_) logger_->warn(msg);
}

void Logger::Error(const std::string& msg) {
    if (logger_) logger_->error(msg);
}

void Logger::Critical(const std::string& msg) {
    if (logger_) logger_->critical(msg);
}

void Logger::Flush() {
    if (logger_) logger_->flush();
}

void Logger::FlushOn(LogLevel level) {
    if (logger_) {
        logger_->flush_on(ToSpdlogLevel(level));
    }
}

spdlog::level::level_enum Logger::ToSpdlogLevel(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:    return spdlog::level::trace;
        case LogLevel::Debug:    return spdlog::level::debug;
        case LogLevel::Info:     return spdlog::level::info;
        case LogLevel::Warn:     return spdlog::level::warn;
        case LogLevel::Error:    return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        case LogLevel::Off:      return spdlog::level::off;
        default:                 return spdlog::level::info;
    }
}

LogLevel Logger::FromSpdlogLevel(spdlog::level::level_enum level) const {
    switch (level) {
        case spdlog::level::trace:    return LogLevel::Trace;
        case spdlog::level::debug:    return LogLevel::Debug;
        case spdlog::level::info:     return LogLevel::Info;
        case spdlog::level::warn:     return LogLevel::Warn;
        case spdlog::level::err:      return LogLevel::Error;
        case spdlog::level::critical: return LogLevel::Critical;
        case spdlog::level::off:      return LogLevel::Off;
        default:                      return LogLevel::Info;
    }
}

} // namespace gh_ost