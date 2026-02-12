/**
 * @file regime_attribution.h
 * @brief RegimeFlow regimeflow regime attribution declarations.
 */

#pragma once

#include "regimeflow/regime/types.h"

#include <map>

namespace regimeflow::metrics {

/**
 * @brief Performance metrics for a single regime.
 */
struct RegimePerformance {
    double total_return = 0.0;
    double avg_return = 0.0;
    double sharpe = 0.0;
    double max_drawdown = 0.0;
    double time_pct = 0.0;
    int observations = 0;
};

/**
 * @brief Tracks performance attribution by regime.
 */
class RegimeAttribution {
public:
    /**
     * @brief Update statistics with an equity return in a regime.
     * @param regime Regime type.
     * @param equity_return Return for the period.
     */
    void update(regime::RegimeType regime, double equity_return);

    /**
     * @brief Access computed results per regime.
     */
    const std::map<regime::RegimeType, RegimePerformance>& results() const { return results_; }

private:
    /**
     * @brief Accumulators for per-regime statistics.
     */
    struct RegimeStats {
        double total_return = 0.0;
        double sum = 0.0;
        double sum_sq = 0.0;
        double equity = 1.0;
        double peak = 1.0;
        double max_dd = 0.0;
        int observations = 0;
    };

    void rebuild_results();

    std::map<regime::RegimeType, RegimeStats> stats_;
    std::map<regime::RegimeType, RegimePerformance> results_;
    int total_obs_ = 0;
};

}  // namespace regimeflow::metrics
