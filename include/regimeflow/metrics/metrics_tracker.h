/**
 * @file metrics_tracker.h
 * @brief RegimeFlow regimeflow metrics tracker declarations.
 */

#pragma once

#include "regimeflow/metrics/drawdown.h"
#include "regimeflow/metrics/performance.h"
#include "regimeflow/metrics/attribution.h"
#include "regimeflow/metrics/regime_attribution.h"
#include "regimeflow/metrics/transition_metrics.h"
#include "regimeflow/engine/portfolio.h"

#include <optional>

namespace regimeflow::metrics {

/**
 * @brief Aggregates performance, drawdown, and attribution metrics.
 */
class MetricsTracker {
public:
    /**
     * @brief Update with equity only.
     * @param timestamp Update time.
     * @param equity Current equity.
     */
    void update(Timestamp timestamp, double equity);
    /**
     * @brief Update with full portfolio and optional regime.
     * @param timestamp Update time.
     * @param portfolio Portfolio state.
     * @param regime Optional current regime.
     */
    void update(Timestamp timestamp, const engine::Portfolio& portfolio,
                std::optional<regime::RegimeType> regime = std::nullopt);
    /**
     * @brief Update with full portfolio and regime state.
     * @param timestamp Update time.
     * @param portfolio Portfolio state.
     * @param regime Current regime state.
     */
    void update(Timestamp timestamp, const engine::Portfolio& portfolio,
                const regime::RegimeState& regime);

    /**
     * @brief Access the equity curve.
     */
    const EquityCurve& equity_curve() const { return equity_curve_; }
    /**
     * @brief Access portfolio snapshots captured during updates.
     */
    const std::vector<engine::PortfolioSnapshot>& portfolio_snapshots() const {
        return portfolio_snapshots_;
    }
    /**
     * @brief Access drawdown tracker.
     */
    const DrawdownTracker& drawdown() const { return drawdown_; }
    /**
     * @brief Access attribution tracker.
     */
    const AttributionTracker& attribution() const { return attribution_; }
    /**
     * @brief Access regime attribution tracker.
     */
    const RegimeAttribution& regime_attribution() const { return regime_attribution_; }
    /**
     * @brief Access transition metrics.
     */
    const TransitionMetrics& transition_metrics() const { return transition_metrics_; }
    /**
     * @brief Access recorded regime history.
     */
    const std::vector<regime::RegimeState>& regime_history() const {
        return regime_history_;
    }

private:
    EquityCurve equity_curve_;
    DrawdownTracker drawdown_;
    AttributionTracker attribution_;
    RegimeAttribution regime_attribution_;
    TransitionMetrics transition_metrics_;
    std::vector<engine::PortfolioSnapshot> portfolio_snapshots_;
    std::vector<regime::RegimeState> regime_history_;
    double last_equity_ = 0.0;
    std::optional<regime::RegimeType> last_regime_;
};

}  // namespace regimeflow::metrics
