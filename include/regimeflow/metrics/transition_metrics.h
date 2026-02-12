/**
 * @file transition_metrics.h
 * @brief RegimeFlow regimeflow transition metrics declarations.
 */

#pragma once

#include "regimeflow/regime/types.h"

#include <map>
#include <utility>
#include <vector>

namespace regimeflow::metrics {

/**
 * @brief Aggregated transition statistics between regimes.
 */
struct TransitionStats {
    double avg_return = 0.0;
    double volatility = 0.0;
    int observations = 0;
};

/**
 * @brief Tracks metrics for regime transitions.
 */
class TransitionMetrics {
public:
    /**
     * @brief Update transition metrics with a return.
     * @param from Previous regime.
     * @param to Next regime.
     * @param equity_return Return during the transition window.
     */
    void update(regime::RegimeType from, regime::RegimeType to, double equity_return);
    /**
     * @brief Access computed transition statistics.
     */
    const std::map<std::pair<regime::RegimeType, regime::RegimeType>, TransitionStats>& results() const {
        return results_;
    }

private:
    /**
     * @brief Accumulates returns for a transition.
     */
    struct Accumulator {
        std::vector<double> returns;
    };

    void rebuild_results();

    std::map<std::pair<regime::RegimeType, regime::RegimeType>, Accumulator> acc_;
    std::map<std::pair<regime::RegimeType, regime::RegimeType>, TransitionStats> results_;
};

}  // namespace regimeflow::metrics
