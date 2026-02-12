/**
 * @file time.h
 * @brief RegimeFlow regimeflow time declarations.
 */

#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <string>

namespace regimeflow {

/**
 * @brief Convenience duration wrapper stored in microseconds.
 *
 * @details Provides explicit construction helpers (microseconds, milliseconds,
 * seconds, minutes, hours, days, months) and basic accessors for total time.
 * Months are modeled as fixed 30-day durations for deterministic backtests
 * and schedule math.
 */
class Duration {
public:
    /**
     * @brief Construct a duration from microseconds.
     * @param us Microseconds.
     * @return Duration representing the supplied microseconds.
     */
    static Duration microseconds(int64_t us) { return Duration(us); }
    /**
     * @brief Construct a duration from milliseconds.
     * @param ms Milliseconds.
     * @return Duration representing the supplied milliseconds.
     */
    static Duration milliseconds(int64_t ms) { return Duration(ms * 1000); }
    /**
     * @brief Construct a duration from seconds.
     * @param s Seconds.
     * @return Duration representing the supplied seconds.
     */
    static Duration seconds(int64_t s) { return Duration(s * 1'000'000); }
    /**
     * @brief Construct a duration from minutes.
     * @param m Minutes.
     * @return Duration representing the supplied minutes.
     */
    static Duration minutes(int64_t m) { return seconds(m * 60); }
    /**
     * @brief Construct a duration from hours.
     * @param h Hours.
     * @return Duration representing the supplied hours.
     */
    static Duration hours(int64_t h) { return minutes(h * 60); }
    /**
     * @brief Construct a duration from days.
     * @param d Days.
     * @return Duration representing the supplied days.
     */
    static Duration days(int64_t d) { return hours(d * 24); }
    /**
     * @brief Construct a duration from months.
     * @details Uses a fixed 30-day month for deterministic scheduling.
     * @param m Months.
     * @return Duration representing the supplied months.
     */
    static Duration months(int m) { return days(static_cast<int64_t>(m) * 30); }

    /**
     * @brief Total duration in microseconds.
     * @return Microseconds.
     */
    int64_t total_microseconds() const { return us_; }
    /**
     * @brief Total duration in milliseconds.
     * @return Milliseconds (truncated).
     */
    int64_t total_milliseconds() const { return us_ / 1000; }
    /**
     * @brief Total duration in seconds.
     * @return Seconds (truncated).
     */
    int64_t total_seconds() const { return us_ / 1'000'000; }

private:
    explicit Duration(int64_t us) : us_(us) {}
    int64_t us_ = 0;
};

/**
 * @brief Timestamp stored as microseconds since epoch.
 */
class Timestamp {
public:
    /**
     * @brief Construct a zero/epoch timestamp.
     */
    Timestamp() = default;
    /**
     * @brief Construct from microseconds since epoch.
     * @param microseconds Microseconds since epoch.
     */
    explicit Timestamp(int64_t microseconds) : us_(microseconds) {}

    /**
     * @brief Current wall-clock timestamp.
     * @return Timestamp representing now.
     */
    static Timestamp now();
    /**
     * @brief Parse a timestamp from a string and format.
     * @param str Input timestamp string.
     * @param fmt strftime/strptime-compatible format.
     * @return Parsed timestamp.
     */
    static Timestamp from_string(const std::string& str, const std::string& fmt);
    /**
     * @brief Construct from a calendar date at midnight.
     * @param year Year (e.g., 2026).
     * @param month Month (1-12).
     * @param day Day of month (1-31).
     * @return Timestamp at 00:00:00 local time for the date.
     */
    static Timestamp from_date(int year, int month, int day);

    /**
     * @brief Microseconds since epoch.
     * @return Microseconds.
     */
    int64_t microseconds() const { return us_; }
    /**
     * @brief Milliseconds since epoch.
     * @return Milliseconds (truncated).
     */
    int64_t milliseconds() const { return us_ / 1000; }
    /**
     * @brief Seconds since epoch.
     * @return Seconds (truncated).
     */
    int64_t seconds() const { return us_ / 1'000'000; }

    /**
     * @brief Format timestamp as a string.
     * @param fmt strftime-compatible format string.
     * @return Formatted timestamp string.
     */
    std::string to_string(const std::string& fmt = "%Y-%m-%d %H:%M:%S") const;

    /**
     * @brief Three-way comparison by epoch microseconds.
     */
    auto operator<=>(const Timestamp& other) const = default;

    /**
     * @brief Add a duration to this timestamp.
     * @param d Duration to add.
     * @return New timestamp offset by the duration.
     */
    Timestamp operator+(Duration d) const;
    /**
     * @brief Subtract a duration from this timestamp.
     * @param d Duration to subtract.
     * @return New timestamp offset by the duration.
     */
    Timestamp operator-(Duration d) const;
    /**
     * @brief Difference between two timestamps.
     * @param other Timestamp to subtract from this instance.
     * @return Duration between timestamps.
     */
    Duration operator-(const Timestamp& other) const;

private:
    int64_t us_ = 0;
};

}  // namespace regimeflow
