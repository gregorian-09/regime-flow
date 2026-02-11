#include "regimeflow/common/time.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace regimeflow {

namespace {

time_t timegm_utc(std::tm* tm) {
#if defined(_WIN32)
    return _mkgmtime(tm);
#else
    return timegm(tm);
#endif
}

}  // namespace

Timestamp Timestamp::now() {
    auto now = std::chrono::system_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return Timestamp(us.count());
}

Timestamp Timestamp::from_string(const std::string& str, const std::string& fmt) {
    std::tm tm = {};
    std::istringstream input(str);
    input >> std::get_time(&tm, fmt.c_str());
    if (input.fail()) {
        throw std::invalid_argument("Failed to parse timestamp: " + str);
    }
    time_t seconds = timegm_utc(&tm);
    if (seconds == static_cast<time_t>(-1)) {
        throw std::invalid_argument("Failed to convert timestamp: " + str);
    }
    return Timestamp(static_cast<int64_t>(seconds) * 1'000'000);
}

Timestamp Timestamp::from_date(int year, int month, int day) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    time_t seconds = timegm_utc(&tm);
    if (seconds == static_cast<time_t>(-1)) {
        throw std::invalid_argument("Failed to convert date");
    }
    return Timestamp(static_cast<int64_t>(seconds) * 1'000'000);
}

std::string Timestamp::to_string(const std::string& fmt) const {
    std::time_t seconds = static_cast<std::time_t>(us_ / 1'000'000);
    std::tm tm = {};
#if defined(_WIN32)
    gmtime_s(&tm, &seconds);
#else
    gmtime_r(&seconds, &tm);
#endif
    std::ostringstream output;
    output << std::put_time(&tm, fmt.c_str());
    return output.str();
}

Timestamp Timestamp::operator+(Duration d) const {
    return Timestamp(us_ + d.total_microseconds());
}

Timestamp Timestamp::operator-(Duration d) const {
    return Timestamp(us_ - d.total_microseconds());
}

Duration Timestamp::operator-(const Timestamp& other) const {
    return Duration::microseconds(us_ - other.us_);
}

}  // namespace regimeflow
