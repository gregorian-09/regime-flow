#include "regimeflow/metrics/regime_attribution.h"

#include <cmath>

namespace regimeflow::metrics
{
    void RegimeAttribution::update(const regime::RegimeType regime, const double equity_return) {
        auto& [total_return, sum, sum_sq, equity, peak, max_dd, observations] = stats_[regime];
        total_return += equity_return;
        sum += equity_return;
        sum_sq += equity_return * equity_return;
        equity *= (1.0 + equity_return);
        peak = std::max(peak, equity);
        const double dd = peak > 0.0 ? (peak - equity) / peak : 0.0;
        max_dd = std::max(max_dd, dd);
        observations += 1;
        total_obs_ += 1;
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
            perf.total_return = stats.total_return;
            perf.observations = stats.observations;
            perf.avg_return = stats.observations > 0
                ? stats.sum / static_cast<double>(stats.observations)
                : 0.0;
            const double variance = stats.observations > 1
                ? (stats.sum_sq - (stats.sum * stats.sum) / stats.observations)
                    / static_cast<double>(stats.observations - 1)
                : 0.0;
            const double stddev = variance > 0.0 ? std::sqrt(variance) : 0.0;
            perf.sharpe = stddev > 0.0 ? (perf.avg_return / stddev) : 0.0;
            perf.max_drawdown = stats.max_dd;
            perf.time_pct =
                static_cast<double>(stats.observations) / static_cast<double>(total_obs_);
            results_[regime] = perf;
        }
    }
}  // namespace regimeflow::metrics
