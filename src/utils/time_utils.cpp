/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "utils/time_utils.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace gh_ost {

// Current time
TimePoint TimeUtils::Now() {
    return std::chrono::system_clock::now();
}

uint64_t TimeUtils::NowUnix() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

uint64_t TimeUtils::NowUnixMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

uint64_t TimeUtils::NowUnixMicros() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

uint64_t TimeUtils::NowUnixNanos() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// Time conversion
TimePoint TimeUtils::FromUnix(uint64_t timestamp) {
    return std::chrono::system_clock::from_time_t(static_cast<time_t>(timestamp));
}

TimePoint TimeUtils::FromUnixMillis(uint64_t timestamp_ms) {
    return std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(
        std::chrono::milliseconds(timestamp_ms)
    );
}

TimePoint TimeUtils::FromUnixMicros(uint64_t timestamp_us) {
    return std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>(
        std::chrono::microseconds(timestamp_us)
    );
}

// Duration conversion
uint64_t TimeUtils::ToMillis(const Duration& duration) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
    );
}

uint64_t TimeUtils::ToSeconds(const Duration& duration) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(duration).count()
    );
}

double TimeUtils::ToDoubleSeconds(const Duration& duration) {
    return duration.count();
}

// Format time
std::string TimeUtils::FormatTime(const TimePoint& time, const std::string& format) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, format.c_str());
    return oss.str();
}

std::string TimeUtils::FormatISO8601(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    
    // Add timezone offset
    char tz_buf[6];
    strftime(tz_buf, sizeof(tz_buf), "%z", tm);
    oss << tz_buf;
    
    return oss.str();
}

std::string TimeUtils::FormatISO8601UTC(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::gmtime(&t);
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string TimeUtils::FormatISO8601WithMillis(const TimePoint& time) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()
    );
    auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ms -= s;
    
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    char tz_buf[6];
    strftime(tz_buf, sizeof(tz_buf), "%z", tm);
    oss << tz_buf;
    
    return oss.str();
}

// Format duration
std::string TimeUtils::FormatDuration(const Duration& duration) {
    double total_seconds = duration.count();
    
    if (total_seconds < 0) return "0s";
    
    uint64_t total_sec_int = static_cast<uint64_t>(total_seconds);
    uint64_t hours = total_sec_int / 3600;
    uint64_t minutes = (total_sec_int % 3600) / 60;
    uint64_t seconds = total_sec_int % 60;
    
    std::ostringstream oss;
    if (hours > 0) oss << hours << "h ";
    if (minutes > 0 || hours > 0) oss << minutes << "m ";
    oss << seconds << "s";
    
    return oss.str();
}

std::string TimeUtils::FormatDurationHuman(const Duration& duration) {
    double total_seconds = duration.count();
    
    if (total_seconds < 0) return "0s";
    
    if (total_seconds < 1) {
        return std::to_string(static_cast<uint64_t>(total_seconds * 1000)) + "ms";
    }
    
    uint64_t total_sec_int = static_cast<uint64_t>(total_seconds);
    uint64_t days = total_sec_int / 86400;
    uint64_t hours = (total_sec_int % 86400) / 3600;
    uint64_t minutes = (total_sec_int % 3600) / 60;
    uint64_t seconds = total_sec_int % 60;
    
    std::ostringstream oss;
    if (days > 0) oss << days << "d ";
    if (hours > 0 || days > 0) oss << hours << "h ";
    if (minutes > 0 || hours > 0 || days > 0) oss << minutes << "m ";
    oss << seconds << "s";
    
    return oss.str();
}

std::string TimeUtils::FormatDurationPrecise(const Duration& duration) {
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(millis);
    millis -= seconds;
    
    std::ostringstream oss;
    oss << seconds.count() << "." << std::setfill('0') << std::setw(3) << millis.count() << "s";
    return oss.str();
}

// Parse time
std::optional<TimePoint> TimeUtils::ParseTime(const std::string& str, const std::string& format) {
    std::tm tm = {};
    std::istringstream iss(str);
    iss >> std::get_time(&tm, format.c_str());
    if (iss.fail()) return std::nullopt;
    
    auto time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return time;
}

std::optional<TimePoint> TimeUtils::ParseISO8601(const std::string& str) {
    // Try various ISO 8601 formats
    std::tm tm = {};
    std::istringstream iss(str);
    
    // Try with timezone
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S%z");
    if (!iss.fail()) {
        auto time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        return time;
    }
    
    // Try UTC
    iss.clear();
    iss.str(str);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (!iss.fail()) {
        auto time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        return time;
    }
    
    // Try without timezone
    iss.clear();
    iss.str(str);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (!iss.fail()) {
        auto time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        return time;
    }
    
    return std::nullopt;
}

std::optional<TimePoint> TimeUtils::ParseMySQLDateTime(const std::string& str) {
    // MySQL datetime formats: YYYY-MM-DD HH:MM:SS or YYYY-MM-DD HH:MM:SS.mmmmmm
    std::tm tm = {};
    std::istringstream iss(str);
    
    // Try with microseconds
    std::string base_str = str;
    int micros = 0;
    size_t dot_pos = str.find('.');
    if (dot_pos != std::string::npos) {
        base_str = str.substr(0, dot_pos);
        std::string micros_str = str.substr(dot_pos + 1);
        micros = std::stoi(micros_str);
    }
    
    iss.str(base_str);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (!iss.fail()) {
        auto time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        // Add microseconds if present
        if (micros > 0) {
            time += std::chrono::microseconds(micros);
        }
        return time;
    }
    
    return std::nullopt;
}

// Sleep functions
void TimeUtils::SleepFor(uint64_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void TimeUtils::SleepForSeconds(double seconds) {
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
}

void TimeUtils::SleepUntil(const TimePoint& time) {
    std::this_thread::sleep_until(time);
}

// Time comparison
bool TimeUtils::IsAfter(const TimePoint& a, const TimePoint& b) {
    return a > b;
}

bool TimeUtils::IsBefore(const TimePoint& a, const TimePoint& b) {
    return a < b;
}

Duration TimeUtils::DurationBetween(const TimePoint& start, const TimePoint& end) {
    return end - start;
}

// Time arithmetic
TimePoint TimeUtils::AddSeconds(const TimePoint& time, int64_t seconds) {
    return time + std::chrono::seconds(seconds);
}

TimePoint TimeUtils::AddMinutes(const TimePoint& time, int64_t minutes) {
    return time + std::chrono::minutes(minutes);
}

TimePoint TimeUtils::AddHours(const TimePoint& time, int64_t hours) {
    return time + std::chrono::hours(hours);
}

TimePoint TimeUtils::AddDays(const TimePoint& time, int64_t days) {
    return time + std::chrono::hours(days * 24);
}

// Time components
int TimeUtils::Year(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    return tm ? tm->tm_year + 1900 : 0;
}

int TimeUtils::Month(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    return tm ? tm->tm_mon + 1 : 0;
}

int TimeUtils::Day(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    return tm ? tm->tm_mday : 0;
}

int TimeUtils::Hour(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    return tm ? tm->tm_hour : 0;
}

int TimeUtils::Minute(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    return tm ? tm->tm_min : 0;
}

int TimeUtils::Second(const TimePoint& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    return tm ? tm->tm_sec : 0;
}

int TimeUtils::Millisecond(const TimePoint& time) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()
    );
    auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    return static_cast<int>((ms - s).count());
}

// MySQL timestamp helpers
std::string TimeUtils::FormatMySQLTimestamp(const TimePoint& time) {
    return FormatTime(time, "%Y-%m-%d %H:%M:%S");
}

std::string TimeUtils::FormatMySQLTimestampWithMicros(const TimePoint& time) {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        time.time_since_epoch()
    );
    auto s = std::chrono::duration_cast<std::chrono::seconds>(us);
    us -= s;
    
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&t);
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(6) << us.count();
    return oss.str();
}

uint64_t TimeUtils::ToMySQLTimestamp(const TimePoint& time) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            time.time_since_epoch()
        ).count()
    );
}

// ElapsedTimer implementation
TimeUtils::ElapsedTimer::ElapsedTimer() 
    : start_time_(TimePoint{})
    , stop_time_(TimePoint{})
    , running_(false) {}

void TimeUtils::ElapsedTimer::Start() {
    start_time_ = Now();
    running_ = true;
}

void TimeUtils::ElapsedTimer::Stop() {
    stop_time_ = Now();
    running_ = false;
}

void TimeUtils::ElapsedTimer::Reset() {
    start_time_ = TimePoint{};
    stop_time_ = TimePoint{};
    running_ = false;
}

Duration TimeUtils::ElapsedTimer::Elapsed() const {
    if (running_) {
        return DurationBetween(start_time_, Now());
    }
    return DurationBetween(start_time_, stop_time_);
}

double TimeUtils::ElapsedTimer::ElapsedSeconds() const {
    return ToDoubleSeconds(Elapsed());
}

uint64_t TimeUtils::ElapsedTimer::ElapsedMillis() const {
    return ToMillis(Elapsed());
}

uint64_t TimeUtils::ElapsedTimer::ElapsedMicros() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(Elapsed()).count();
}

std::string TimeUtils::ElapsedTimer::ToString() const {
    return FormatDurationPrecise(Elapsed());
}

std::string TimeUtils::ElapsedTimer::ToHumanString() const {
    return FormatDurationHuman(Elapsed());
}

// DeadlineTimer implementation
TimeUtils::DeadlineTimer::DeadlineTimer(Duration timeout) 
    : timeout_(timeout) {
    start_ = Now();
    deadline_ = start_ + std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
}

TimeUtils::DeadlineTimer::DeadlineTimer(uint64_t timeout_millis) 
    : timeout_(std::chrono::milliseconds(timeout_millis)) {
    start_ = Now();
    deadline_ = start_ + std::chrono::milliseconds(timeout_millis);
}

bool TimeUtils::DeadlineTimer::IsExpired() const {
    return Now() >= deadline_;
}

Duration TimeUtils::DeadlineTimer::Remaining() const {
    TimePoint now = Now();
    if (now >= deadline_) return Duration(0);
    return deadline_ - now;
}

uint64_t TimeUtils::DeadlineTimer::RemainingMillis() const {
    return ToMillis(Remaining());
}

void TimeUtils::DeadlineTimer::Reset() {
    start_ = Now();
    deadline_ = start_ + std::chrono::duration_cast<std::chrono::milliseconds>(timeout_);
}

void TimeUtils::DeadlineTimer::Reset(Duration timeout) {
    timeout_ = timeout;
    Reset();
}

void TimeUtils::DeadlineTimer::Reset(uint64_t timeout_millis) {
    timeout_ = std::chrono::milliseconds(timeout_millis);
    Reset();
}

// RateLimiter implementation
TimeUtils::RateLimiter::RateLimiter(uint64_t interval_millis) 
    : interval_millis_(interval_millis) {
    last_execution_ = Now();
}

bool TimeUtils::RateLimiter::ShouldProceed() {
    TimePoint now = Now();
    Duration elapsed = DurationBetween(last_execution_, now);
    
    if (ToMillis(elapsed) >= interval_millis_) {
        last_execution_ = now;
        return true;
    }
    return false;
}

void TimeUtils::RateLimiter::WaitForNext() {
    TimePoint now = Now();
    Duration elapsed = DurationBetween(last_execution_, now);
    uint64_t elapsed_ms = ToMillis(elapsed);
    
    if (elapsed_ms < interval_millis_) {
        SleepFor(interval_millis_ - elapsed_ms);
    }
    last_execution_ = Now();
}

void TimeUtils::RateLimiter::SetInterval(uint64_t interval_millis) {
    interval_millis_ = interval_millis;
}

} // namespace gh_ost