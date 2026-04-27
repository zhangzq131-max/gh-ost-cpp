/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_TIME_UTILS_HPP
#define GH_OST_TIME_UTILS_HPP

#include <chrono>
#include <string>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <optional>
#include <thread>

namespace gh_ost {

// Time types
using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::duration<double>;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Microseconds = std::chrono::microseconds;
using Nanoseconds = std::chrono::nanoseconds;

class TimeUtils {
public:
    // Current time
    static TimePoint Now();
    static uint64_t NowUnix();
    static uint64_t NowUnixMillis();
    static uint64_t NowUnixMicros();
    static uint64_t NowUnixNanos();
    
    // Time conversion
    static TimePoint FromUnix(uint64_t timestamp);
    static TimePoint FromUnixMillis(uint64_t timestamp_ms);
    static TimePoint FromUnixMicros(uint64_t timestamp_us);
    
    // Duration conversion
    static uint64_t ToMillis(const Duration& duration);
    static uint64_t ToSeconds(const Duration& duration);
    static double ToDoubleSeconds(const Duration& duration);
    
    // Format time
    static std::string FormatTime(const TimePoint& time, 
                                  const std::string& format = "%Y-%m-%d %H:%M:%S");
    static std::string FormatISO8601(const TimePoint& time);
    static std::string FormatISO8601UTC(const TimePoint& time);
    static std::string FormatISO8601WithMillis(const TimePoint& time);
    
    // Format duration
    static std::string FormatDuration(const Duration& duration);
    static std::string FormatDurationHuman(const Duration& duration);
    static std::string FormatDurationPrecise(const Duration& duration);
    
    // Parse time
    static std::optional<TimePoint> ParseTime(const std::string& str, 
                                              const std::string& format = "%Y-%m-%d %H:%M:%S");
    static std::optional<TimePoint> ParseISO8601(const std::string& str);
    static std::optional<TimePoint> ParseMySQLDateTime(const std::string& str);
    
    // Sleep functions
    static void SleepFor(uint64_t milliseconds);
    static void SleepForSeconds(double seconds);
    static void SleepUntil(const TimePoint& time);
    
    // Time comparison
    static bool IsAfter(const TimePoint& a, const TimePoint& b);
    static bool IsBefore(const TimePoint& a, const TimePoint& b);
    static Duration DurationBetween(const TimePoint& start, const TimePoint& end);
    
    // Time arithmetic
    static TimePoint AddSeconds(const TimePoint& time, int64_t seconds);
    static TimePoint AddMinutes(const TimePoint& time, int64_t minutes);
    static TimePoint AddHours(const TimePoint& time, int64_t hours);
    static TimePoint AddDays(const TimePoint& time, int64_t days);
    
    // Time components
    static int Year(const TimePoint& time);
    static int Month(const TimePoint& time);
    static int Day(const TimePoint& time);
    static int Hour(const TimePoint& time);
    static int Minute(const TimePoint& time);
    static int Second(const TimePoint& time);
    static int Millisecond(const TimePoint& time);
    
    // MySQL timestamp helpers
    static std::string FormatMySQLTimestamp(const TimePoint& time);
    static std::string FormatMySQLTimestampWithMicros(const TimePoint& time);
    static uint64_t ToMySQLTimestamp(const TimePoint& time);
    
    // Elapsed time tracker
    class ElapsedTimer {
    public:
        ElapsedTimer();
        
        void Start();
        void Stop();
        void Reset();
        
        Duration Elapsed() const;
        double ElapsedSeconds() const;
        uint64_t ElapsedMillis() const;
        uint64_t ElapsedMicros() const;
        
        std::string ToString() const;
        std::string ToHumanString() const;
        
        bool IsRunning() const { return running_; }
        
    private:
        TimePoint start_time_;
        TimePoint stop_time_;
        bool running_;
    };
    
    // Deadline timer (for timeout operations)
    class DeadlineTimer {
    public:
        explicit DeadlineTimer(Duration timeout);
        explicit DeadlineTimer(uint64_t timeout_millis);
        
        bool IsExpired() const;
        Duration Remaining() const;
        uint64_t RemainingMillis() const;
        
        void Reset();
        void Reset(Duration timeout);
        void Reset(uint64_t timeout_millis);
        
        TimePoint Deadline() const { return deadline_; }
        
    private:
        TimePoint start_;
        TimePoint deadline_;
        Duration timeout_;
    };
    
    // Rate limiter helper
    class RateLimiter {
    public:
        explicit RateLimiter(uint64_t interval_millis);
        
        bool ShouldProceed();
        void WaitForNext();
        
        void SetInterval(uint64_t interval_millis);
        uint64_t Interval() const { return interval_millis_; }
        
    private:
        uint64_t interval_millis_;
        TimePoint last_execution_;
    };
    
private:
    TimeUtils() = delete;
};

} // namespace gh_ost

#endif // GH_OST_TIME_UTILS_HPP