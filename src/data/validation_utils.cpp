#include "regimeflow/data/validation_utils.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <stdexcept>

namespace regimeflow::data {

namespace {

struct RunningStats {
    size_t count = 0;
    double mean = 0.0;
    double m2 = 0.0;

    void push(double value) {
        ++count;
        double delta = value - mean;
        mean += delta / static_cast<double>(count);
        double delta2 = value - mean;
        m2 += delta * delta2;
    }

    double variance() const {
        return count > 1 ? m2 / static_cast<double>(count - 1) : 0.0;
    }

    double stddev() const {
        return std::sqrt(variance());
    }
};

bool within_trading_hours(const Timestamp& ts, int start_seconds, int end_seconds) {
    std::time_t seconds = static_cast<std::time_t>(ts.microseconds() / 1'000'000);
    std::tm tm = {};
#if defined(_WIN32)
    gmtime_s(&tm, &seconds);
#else
    gmtime_r(&seconds, &tm);
#endif
    int secs = tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
    if (start_seconds <= end_seconds) {
        return secs >= start_seconds && secs <= end_seconds;
    }
    return secs >= start_seconds || secs <= end_seconds;
}

bool handle_issue(ValidationSeverity severity,
                  ValidationAction action,
                  const std::string& message,
                  bool collect_report,
                  ValidationReport* report,
                  size_t index) {
    if (collect_report && report) {
        report->add_issue({severity, index, message});
    }

    if (severity == ValidationSeverity::Error) {
        if (action == ValidationAction::Fail) {
            if (!collect_report) {
                throw std::runtime_error("Validation error: " + message);
            }
            return false;
        }
        return false;
    }

    if (action == ValidationAction::Fail) {
        if (!collect_report) {
            throw std::runtime_error("Validation warning treated as error: " + message);
        }
        return false;
    }
    if (action == ValidationAction::Skip) {
        return false;
    }
    return true;
}

ValidationAction normalize_error_action(ValidationAction action) {
    if (action == ValidationAction::Continue || action == ValidationAction::Fill) {
        return ValidationAction::Skip;
    }
    return action;
}

}  // namespace

std::optional<Duration> bar_interval_for(BarType bar_type) {
    switch (bar_type) {
        case BarType::Time_1Min: return Duration::minutes(1);
        case BarType::Time_5Min: return Duration::minutes(5);
        case BarType::Time_15Min: return Duration::minutes(15);
        case BarType::Time_30Min: return Duration::minutes(30);
        case BarType::Time_1Hour: return Duration::hours(1);
        case BarType::Time_4Hour: return Duration::hours(4);
        case BarType::Time_1Day: return Duration::days(1);
        default: return std::nullopt;
    }
}

std::vector<Bar> fill_missing_time_bars(const std::vector<Bar>& bars, Duration interval) {
    if (bars.size() < 2) {
        return bars;
    }
    std::vector<Bar> filled;
    filled.reserve(bars.size());
    Bar prev = bars.front();
    filled.push_back(prev);
    for (size_t i = 1; i < bars.size(); ++i) {
        const Bar& current = bars[i];
        Timestamp expected = prev.timestamp + interval;
        while (expected < current.timestamp) {
            Bar synthetic = prev;
            synthetic.timestamp = expected;
            synthetic.open = prev.close;
            synthetic.high = prev.close;
            synthetic.low = prev.close;
            synthetic.close = prev.close;
            synthetic.volume = 0;
            synthetic.trade_count = 0;
            synthetic.vwap = prev.close;
            filled.push_back(synthetic);
            prev = synthetic;
            expected = prev.timestamp + interval;
        }
        filled.push_back(current);
        prev = current;
    }
    return filled;
}

std::vector<Bar> validate_bars(std::vector<Bar> bars,
                               BarType bar_type,
                               const ValidationConfig& config,
                               bool fill_missing_bars,
                               bool collect_report,
                               ValidationReport* report) {
    if (bars.empty()) {
        return bars;
    }
    std::vector<Bar> out;
    out.reserve(bars.size());
    Timestamp last_ts;
    bool has_last_ts = false;
    double last_close = 0.0;
    bool has_last_close = false;
    Timestamp now;
    if (config.check_future_timestamps) {
        now = Timestamp::now();
    }

    RunningStats price_stats;
    RunningStats volume_stats;
    bool fill_on_gap = false;
    const auto interval = bar_interval_for(bar_type);

    for (size_t i = 0; i < bars.size(); ++i) {
        const auto& bar = bars[i];

        if (config.check_price_bounds) {
            if (!std::isfinite(bar.open) || !std::isfinite(bar.high) ||
                !std::isfinite(bar.low) || !std::isfinite(bar.close) ||
                bar.open <= 0 || bar.high <= 0 || bar.low <= 0 || bar.close <= 0 ||
                (config.max_price > 0.0 &&
                 (bar.open > config.max_price || bar.high > config.max_price ||
                  bar.low > config.max_price || bar.close > config.max_price)) ||
                (bar.high < bar.low || bar.open < bar.low || bar.open > bar.high ||
                 bar.close < bar.low || bar.close > bar.high)) {
                auto action = normalize_error_action(config.on_error);
                if (!handle_issue(ValidationSeverity::Error, action, "OHLC out of range",
                                  collect_report, report, i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_volume_bounds && config.max_volume > 0 && bar.volume > config.max_volume) {
            auto action = normalize_error_action(config.on_error);
            if (!handle_issue(ValidationSeverity::Error, action, "Volume exceeds max_volume",
                              collect_report, report, i + 1)) {
                continue;
            }
        }

        if (config.check_future_timestamps) {
            if (bar.timestamp > now + config.max_future_skew) {
                auto action = normalize_error_action(config.on_error);
                if (!handle_issue(ValidationSeverity::Error, action, "Timestamp is in the future",
                                  collect_report, report, i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_trading_hours &&
            !within_trading_hours(bar.timestamp, config.trading_start_seconds,
                                  config.trading_end_seconds)) {
            auto action = normalize_error_action(config.on_error);
            if (!handle_issue(ValidationSeverity::Error, action, "Timestamp outside trading hours",
                              collect_report, report, i + 1)) {
                continue;
            }
        }

        if (config.require_monotonic_timestamps && has_last_ts && bar.timestamp < last_ts) {
            auto action = normalize_error_action(config.on_error);
            if (!handle_issue(ValidationSeverity::Error, action, "Non-monotonic timestamp",
                              collect_report, report, i + 1)) {
                continue;
            }
        }

        if (config.check_gap && has_last_ts) {
            Duration gap = bar.timestamp - last_ts;
            if (gap.total_microseconds() > config.max_gap.total_microseconds()) {
                auto action = config.on_gap;
                if (action == ValidationAction::Fill && interval.has_value()) {
                    fill_on_gap = true;
                }
                if (!handle_issue(ValidationSeverity::Warning, action,
                                  "Timestamp gap exceeds max_gap", collect_report, report,
                                  i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_price_jump && has_last_close && last_close != 0.0) {
            double jump = std::abs(bar.close - last_close) / std::abs(last_close);
            if (jump > config.max_jump_pct) {
                if (!handle_issue(ValidationSeverity::Warning, config.on_warning,
                                  "Price jump exceeds max_jump_pct", collect_report, report,
                                  i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_outliers && price_stats.count >= config.outlier_warmup) {
            double stddev = price_stats.stddev();
            if (stddev > 0.0) {
                double zscore = std::abs(bar.close - price_stats.mean) / stddev;
                if (zscore > config.outlier_zscore) {
                    if (!handle_issue(ValidationSeverity::Warning, config.on_warning,
                                      "Price outlier detected", collect_report, report,
                                      i + 1)) {
                        continue;
                    }
                }
            }
        }

        if (config.check_outliers && volume_stats.count >= config.outlier_warmup) {
            double stddev = volume_stats.stddev();
            if (stddev > 0.0) {
                double zscore = std::abs(static_cast<double>(bar.volume) - volume_stats.mean) /
                                stddev;
                if (zscore > config.outlier_zscore) {
                    if (!handle_issue(ValidationSeverity::Warning, config.on_warning,
                                      "Volume outlier detected", collect_report, report,
                                      i + 1)) {
                        continue;
                    }
                }
            }
        }

        out.push_back(bar);
        last_ts = bar.timestamp;
        has_last_ts = true;
        last_close = bar.close;
        has_last_close = true;
        if (config.check_outliers) {
            price_stats.push(bar.close);
            volume_stats.push(static_cast<double>(bar.volume));
        }
    }

    if ((fill_missing_bars || fill_on_gap) && interval.has_value()) {
        return fill_missing_time_bars(out, *interval);
    }
    return out;
}

std::vector<Tick> validate_ticks(std::vector<Tick> ticks,
                                 const ValidationConfig& config,
                                 bool collect_report,
                                 ValidationReport* report) {
    if (ticks.empty()) {
        return ticks;
    }
    std::vector<Tick> out;
    out.reserve(ticks.size());
    Timestamp last_ts;
    bool has_last_ts = false;
    double last_price = 0.0;
    bool has_last_price = false;
    Timestamp now;
    if (config.check_future_timestamps) {
        now = Timestamp::now();
    }

    RunningStats price_stats;
    RunningStats volume_stats;

    for (size_t i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];

        if (config.check_price_bounds) {
            if (!std::isfinite(tick.price) || tick.price <= 0 ||
                (config.max_price > 0.0 && tick.price > config.max_price)) {
                auto action = normalize_error_action(config.on_error);
                if (!handle_issue(ValidationSeverity::Error, action, "Invalid price",
                                  collect_report, report, i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_volume_bounds && config.max_volume > 0 &&
            tick.quantity > static_cast<double>(config.max_volume)) {
            auto action = normalize_error_action(config.on_error);
            if (!handle_issue(ValidationSeverity::Error, action, "Quantity exceeds max_volume",
                              collect_report, report, i + 1)) {
                continue;
            }
        }

        if (config.check_future_timestamps) {
            if (tick.timestamp > now + config.max_future_skew) {
                auto action = normalize_error_action(config.on_error);
                if (!handle_issue(ValidationSeverity::Error, action, "Timestamp is in the future",
                                  collect_report, report, i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_trading_hours &&
            !within_trading_hours(tick.timestamp, config.trading_start_seconds,
                                  config.trading_end_seconds)) {
            auto action = normalize_error_action(config.on_error);
            if (!handle_issue(ValidationSeverity::Error, action, "Timestamp outside trading hours",
                              collect_report, report, i + 1)) {
                continue;
            }
        }

        if (config.require_monotonic_timestamps && has_last_ts && tick.timestamp < last_ts) {
            auto action = normalize_error_action(config.on_error);
            if (!handle_issue(ValidationSeverity::Error, action, "Non-monotonic timestamp",
                              collect_report, report, i + 1)) {
                continue;
            }
        }

        if (config.check_gap && has_last_ts) {
            Duration gap = tick.timestamp - last_ts;
            if (gap.total_microseconds() > config.max_gap.total_microseconds()) {
                if (!handle_issue(ValidationSeverity::Warning, config.on_gap,
                                  "Timestamp gap exceeds max_gap", collect_report, report,
                                  i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_price_jump && has_last_price && last_price != 0.0) {
            double jump = std::abs(tick.price - last_price) / std::abs(last_price);
            if (jump > config.max_jump_pct) {
                if (!handle_issue(ValidationSeverity::Warning, config.on_warning,
                                  "Price jump exceeds max_jump_pct", collect_report, report,
                                  i + 1)) {
                    continue;
                }
            }
        }

        if (config.check_outliers && price_stats.count >= config.outlier_warmup) {
            double stddev = price_stats.stddev();
            if (stddev > 0.0) {
                double zscore = std::abs(tick.price - price_stats.mean) / stddev;
                if (zscore > config.outlier_zscore) {
                    if (!handle_issue(ValidationSeverity::Warning, config.on_warning,
                                      "Price outlier detected", collect_report, report,
                                      i + 1)) {
                        continue;
                    }
                }
            }
        }

        if (config.check_outliers && volume_stats.count >= config.outlier_warmup) {
            double stddev = volume_stats.stddev();
            if (stddev > 0.0) {
                double zscore = std::abs(tick.quantity - volume_stats.mean) / stddev;
                if (zscore > config.outlier_zscore) {
                    if (!handle_issue(ValidationSeverity::Warning, config.on_warning,
                                      "Quantity outlier detected", collect_report, report,
                                      i + 1)) {
                        continue;
                    }
                }
            }
        }

        out.push_back(tick);
        last_ts = tick.timestamp;
        has_last_ts = true;
        last_price = tick.price;
        has_last_price = true;
        if (config.check_outliers) {
            price_stats.push(tick.price);
            volume_stats.push(tick.quantity);
        }
    }
    return out;
}

}  // namespace regimeflow::data
