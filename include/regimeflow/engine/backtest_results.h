/**
 * @file backtest_results.h
 * @brief RegimeFlow regimeflow backtest results declarations.
 */

#pragma once

#include "regimeflow/engine/order.h"
#include "regimeflow/metrics/metrics_tracker.h"
#include "regimeflow/regime/types.h"

#include <vector>

namespace regimeflow::engine {

/**
 * @brief Aggregated results of a backtest run.
 */
struct BacktestResults {
    /**
     * @brief Total return as a fraction (e.g., 0.15 = 15%).
     */
    double total_return = 0.0;
    /**
     * @brief Maximum drawdown as a fraction.
     */
    double max_drawdown = 0.0;
    /**
     * @brief Detailed performance metrics.
     */
    metrics::MetricsTracker metrics;
    /**
     * @brief Execution fills captured during the run.
     */
    std::vector<engine::Fill> fills;
    /**
     * @brief Regime history captured during the run.
     */
    std::vector<regime::RegimeState> regime_history;
};

}  // namespace regimeflow::engine
