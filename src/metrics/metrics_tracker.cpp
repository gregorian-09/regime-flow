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
    if (regime) {
        regime_attribution_.update(*regime, ret);
        if (last_regime_ && *last_regime_ != *regime) {
            transition_metrics_.update(*last_regime_, *regime, ret);
        }
        last_regime_ = *regime;
    }
}

}  // namespace regimeflow::metrics
