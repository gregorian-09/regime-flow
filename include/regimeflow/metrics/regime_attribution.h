/**
 * @file regime_attribution.h
 * @brief RegimeFlow regimeflow regime attribution declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/regime/types.h"

#include <map>

namespace regimeflow::metrics
{
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
         * @param timestamp Interval end timestamp.
         * @param regime Regime type.
         * @param equity_return Return for the period.
         */
        void update(Timestamp timestamp, regime::RegimeType regime, double equity_return);

        /**
         * @brief Access computed results per regime.
         */
        [[nodiscard]] const std::map<regime::RegimeType, RegimePerformance>& results() const { return results_; }

    private:
        /**
         * @brief Accumulators for per-regime statistics.
         */
        struct RegimeStats {
            double sum = 0.0;
            double sum_sq = 0.0;
            double equity = 1.0;
            double peak = 1.0;
            double max_dd = 0.0;
            int observations = 0;
            int64_t active_duration_us = 0;
            Timestamp first_timestamp;
            Timestamp last_timestamp;
            bool has_timestamp = false;
        };

        void rebuild_results();

        std::map<regime::RegimeType, RegimeStats> stats_;
        std::map<regime::RegimeType, RegimePerformance> results_;
        int total_obs_ = 0;
        Timestamp last_timestamp_;
        bool has_last_timestamp_ = false;
        int64_t total_duration_us_ = 0;
    };
}  // namespace regimeflow::metrics
