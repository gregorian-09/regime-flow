#include "regimeflow/metrics/transition_metrics.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::metrics {

void TransitionMetrics::update(regime::RegimeType from, regime::RegimeType to,
                               double equity_return) {
    acc_[{from, to}].returns.push_back(equity_return);
    rebuild_results();
}

void TransitionMetrics::rebuild_results() {
    results_.clear();
    for (const auto& [key, acc] : acc_) {
        TransitionStats stats;
        if (!acc.returns.empty()) {
            double sum = 0.0;
            for (double r : acc.returns) sum += r;
            stats.avg_return = sum / static_cast<double>(acc.returns.size());
            double var = 0.0;
            for (double r : acc.returns) {
                double diff = r - stats.avg_return;
                var += diff * diff;
            }
            if (acc.returns.size() > 1) {
                var /= static_cast<double>(acc.returns.size() - 1);
            }
            stats.volatility = std::sqrt(var);
            stats.observations = static_cast<int>(acc.returns.size());
        }
        results_[key] = stats;
    }
}

}  // namespace regimeflow::metrics
