#include "regimeflow/metrics/metrics_tracker.h"

namespace regimeflow::metrics {

void MetricsTracker::update(Timestamp timestamp, double equity) {
    last_equity_ = equity;
    equity_curve_.add_point(timestamp, equity);
    drawdown_.update(timestamp, equity);
}

void MetricsTracker::update(Timestamp timestamp, const engine::Portfolio& portfolio,
                            std::optional<regime::RegimeType> regime) {
    double equity = portfolio.equity();
    double ret = 0.0;
    if (last_equity_ > 0.0) {
        ret = (equity - last_equity_) / last_equity_;
    }
    last_equity_ = equity;

    equity_curve_.add_point(timestamp, equity);
    drawdown_.update(timestamp, equity);
    attribution_.update(timestamp, portfolio);
    portfolio_snapshots_.push_back(portfolio.snapshot(timestamp));
    if (regime) {
        regime::RegimeState state;
        state.regime = *regime;
        state.timestamp = timestamp;
        regime_history_.push_back(state);
        regime_attribution_.update(*regime, ret);
        if (last_regime_ && *last_regime_ != *regime) {
            transition_metrics_.update(*last_regime_, *regime, ret);
        }
        last_regime_ = *regime;
    }
}

void MetricsTracker::update(Timestamp timestamp, const engine::Portfolio& portfolio,
                            const regime::RegimeState& regime) {
    double equity = portfolio.equity();
    double ret = 0.0;
    if (last_equity_ > 0.0) {
        ret = (equity - last_equity_) / last_equity_;
    }
    last_equity_ = equity;

    equity_curve_.add_point(timestamp, equity);
    drawdown_.update(timestamp, equity);
    attribution_.update(timestamp, portfolio);
    portfolio_snapshots_.push_back(portfolio.snapshot(timestamp));
    regime_history_.push_back(regime);

    regime_attribution_.update(regime.regime, ret);
    if (last_regime_ && *last_regime_ != regime.regime) {
        transition_metrics_.update(*last_regime_, regime.regime, ret);
    }
    last_regime_ = regime.regime;
}

}  // namespace regimeflow::metrics
