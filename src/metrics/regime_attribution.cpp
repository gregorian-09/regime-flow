#include "regimeflow/metrics/regime_attribution.h"

#include <cmath>

namespace regimeflow::metrics
{
    void RegimeAttribution::update(const Timestamp timestamp, const regime::RegimeType regime,
                                   const double equity_return) {
        auto& [sum, sum_sq, equity, peak, max_dd, observations, active_duration_us,
               first_timestamp, last_timestamp, has_timestamp] = stats_[regime];
        sum += equity_return;
        sum_sq += equity_return * equity_return;
        equity *= (1.0 + equity_return);
        peak = std::max(peak, equity);
        const double dd = peak > 0.0 ? (peak - equity) / peak : 0.0;
        max_dd = std::max(max_dd, dd);
        observations += 1;
        total_obs_ += 1;
        if (has_last_timestamp_) {
            const int64_t duration_us = std::max<int64_t>(0, (timestamp - last_timestamp_).total_microseconds());
            active_duration_us += duration_us;
            total_duration_us_ += duration_us;
        }
        if (!has_timestamp) {
            first_timestamp = timestamp;
            has_timestamp = true;
        }
        last_timestamp = timestamp;
        last_timestamp_ = timestamp;
        has_last_timestamp_ = true;
        rebuild_results();
    }

    void RegimeAttribution::rebuild_results() {
        results_.clear();
        if (total_obs_ == 0) {
            return;
        }
        for (const auto& [regime, stats] : stats_) {
            if (stats.observations == 0) {
                continue;
            }
            RegimePerformance perf;
            perf.total_return = stats.equity - 1.0;
            perf.observations = stats.observations;
            perf.avg_return = stats.observations > 0
                ? stats.sum / static_cast<double>(stats.observations)
                : 0.0;
            const double variance = stats.observations > 1
                ? (stats.sum_sq - (stats.sum * stats.sum) / stats.observations)
                    / static_cast<double>(stats.observations - 1)
                : 0.0;
            const double stddev = variance > 0.0 ? std::sqrt(variance) : 0.0;
            double periods_per_year = 0.0;
            if (stats.active_duration_us > 0 && stats.observations > 0) {
                const double avg_delta_seconds = static_cast<double>(stats.active_duration_us)
                    / static_cast<double>(stats.observations) / 1'000'000.0;
                if (avg_delta_seconds > 0.0) {
                    periods_per_year = (365.25 * 24.0 * 3600.0) / avg_delta_seconds;
                }
            }
            perf.sharpe = (stddev > 0.0 && periods_per_year > 0.0)
                ? (perf.avg_return / stddev) * std::sqrt(periods_per_year)
                : 0.0;
            perf.max_drawdown = stats.max_dd;
            perf.time_pct = total_duration_us_ > 0
                ? static_cast<double>(stats.active_duration_us) / static_cast<double>(total_duration_us_)
                : 0.0;
            results_[regime] = perf;
        }
    }
}  // namespace regimeflow::metrics
