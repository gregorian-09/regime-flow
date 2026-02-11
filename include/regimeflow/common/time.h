#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <string>

namespace regimeflow {

class Duration {
public:
    static Duration microseconds(int64_t us) { return Duration(us); }
    static Duration milliseconds(int64_t ms) { return Duration(ms * 1000); }
    static Duration seconds(int64_t s) { return Duration(s * 1'000'000); }
    static Duration minutes(int64_t m) { return seconds(m * 60); }
    static Duration hours(int64_t h) { return minutes(h * 60); }
    static Duration days(int64_t d) { return hours(d * 24); }
    static Duration months(int m) { return days(static_cast<int64_t>(m) * 30); }

    int64_t total_microseconds() const { return us_; }
    int64_t total_milliseconds() const { return us_ / 1000; }
    int64_t total_seconds() const { return us_ / 1'000'000; }

private:
    explicit Duration(int64_t us) : us_(us) {}
    int64_t us_ = 0;
};

class Timestamp {
public:
    Timestamp() = default;
    explicit Timestamp(int64_t microseconds) : us_(microseconds) {}

    static Timestamp now();
    static Timestamp from_string(const std::string& str, const std::string& fmt);
    static Timestamp from_date(int year, int month, int day);

    int64_t microseconds() const { return us_; }
    int64_t milliseconds() const { return us_ / 1000; }
    int64_t seconds() const { return us_ / 1'000'000; }

    std::string to_string(const std::string& fmt = "%Y-%m-%d %H:%M:%S") const;

    auto operator<=>(const Timestamp& other) const = default;

    Timestamp operator+(Duration d) const;
    Timestamp operator-(Duration d) const;
    Duration operator-(const Timestamp& other) const;

private:
    int64_t us_ = 0;
};

}  // namespace regimeflow
