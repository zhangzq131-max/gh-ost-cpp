/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "utils/time_utils.hpp"
#include "utils/string_utils.hpp"
#include <thread>

TEST_CASE("TimeUtils::NowUnix", "[time_utils]") {
    uint64_t now = gh_ost::TimeUtils::NowUnix();
    REQUIRE(now > 0);
    
    // Should be close to current Unix time (2026+)
    REQUIRE(now > 1700000000);  // 2023-ish
}

TEST_CASE("TimeUtils::NowUnixMillis", "[time_utils]") {
    uint64_t now_ms = gh_ost::TimeUtils::NowUnixMillis();
    uint64_t now_s = gh_ost::TimeUtils::NowUnix();
    
    REQUIRE(now_ms > now_s * 1000);
    REQUIRE(now_ms < now_s * 1000 + 2000);  // Within 2 seconds
}

TEST_CASE("TimeUtils::FormatTime", "[time_utils]") {
    auto now = gh_ost::TimeUtils::Now();
    std::string formatted = gh_ost::TimeUtils::FormatTime(now, "%Y-%m-%d");
    
    REQUIRE(formatted.length() == 10);  // YYYY-MM-DD
    REQUIRE(formatted[4] == '-');
    REQUIRE(formatted[7] == '-');
}

TEST_CASE("TimeUtils::FormatDuration", "[time_utils]") {
    gh_ost::Duration d(125.5);  // 125.5 seconds
    
    std::string formatted = gh_ost::TimeUtils::FormatDuration(d);
    REQUIRE(gh_ost::StringUtils::Contains(formatted, "2m"));
    REQUIRE(gh_ost::StringUtils::Contains(formatted, "5s"));
}

TEST_CASE("TimeUtils::ElapsedTimer", "[time_utils]") {
    gh_ost::TimeUtils::ElapsedTimer timer;
    
    REQUIRE(!timer.IsRunning());
    
    timer.Start();
    REQUIRE(timer.IsRunning());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    uint64_t elapsed = timer.ElapsedMillis();
    REQUIRE(elapsed >= 90);   // Should be at least 90ms
    REQUIRE(elapsed < 500);   // Should be less than 500ms
    
    timer.Stop();
    REQUIRE(!timer.IsRunning());
    
    std::string str = timer.ToString();
    REQUIRE(gh_ost::StringUtils::Contains(str, "s"));
}

TEST_CASE("TimeUtils::DeadlineTimer", "[time_utils]") {
    gh_ost::TimeUtils::DeadlineTimer timer(100);  // 100ms timeout
    
    REQUIRE(!timer.IsExpired());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    REQUIRE(timer.IsExpired());
    
    timer.Reset(200);
    REQUIRE(!timer.IsExpired());
    
    REQUIRE(timer.RemainingMillis() > 0);
}

TEST_CASE("TimeUtils::RateLimiter", "[time_utils]") {
    gh_ost::TimeUtils::RateLimiter limiter(100);  // 100ms interval
    
    // First call should proceed immediately
    REQUIRE(limiter.ShouldProceed());
    
    // Second call should not proceed (too soon)
    REQUIRE(!limiter.ShouldProceed());
    
    // Wait for interval
    std::this_thread::sleep_for(std::chrono::milliseconds(110));
    
    // Now should proceed
    REQUIRE(limiter.ShouldProceed());
}