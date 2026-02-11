#include "regimeflow/metrics/regime_attribution.h"

#include <cmath>

namespace regimeflow::metrics {

void RegimeAttribution::update(regime::RegimeType regime, double equity_return) {
    auto& stats = stats_[regime];
    stats.total_return += equity_return;
    stats.sum += equity_return;
    stats.sum_sq += equity_return * equity_return;
    stats.equity *= (1.0 + equity_return);
    stats.peak = std::max(stats.peak, stats.equity);
    double dd = stats.peak > 0.0 ? (stats.peak - stats.equity) / stats.peak : 0.0;
    stats.max_dd = std::max(stats.max_dd, dd);
    stats.observations += 1;
    total_obs_ += 1;
    rebuild_results();
}

void RegimeAttribution::rebuild_results() {
    results_.clear();
    for (const auto& [regime, stats] : stats_) {
        RegimePerformance perf;
        perf.total_return = stats.total_return;
        perf.observations = stats.observations;
        perf.avg_return = stats.observations > 0
            ? stats.sum / static_cast<double>(stats.observations)
            : 0.0;
        double variance = stats.observations > 1
            ? (stats.sum_sq - (stats.sum * stats.sum) / stats.observations)
                / static_cast<double>(stats.observations - 1)
            : 0.0;
        double stddev = variance > 0.0 ? std::sqrt(variance) : 0.0;
        perf.sharpe = stddev > 0.0 ? (perf.avg_return / stddev) : 0.0;
        perf.max_drawdown = stats.max_dd;
        perf.time_pct = total_obs_ > 0
            ? static_cast<double>(stats.observations) / static_cast<double>(total_obs_)
            : 0.0;
        results_[regime] = perf;
    }
}

}  // namespace regimeflow::metrics
