#pragma once

#include "regimeflow/metrics/drawdown.h"
#include "regimeflow/metrics/performance.h"
#include "regimeflow/metrics/attribution.h"
#include "regimeflow/metrics/regime_attribution.h"
#include "regimeflow/metrics/transition_metrics.h"

#include <optional>

namespace regimeflow::metrics {

class MetricsTracker {
public:
    void update(Timestamp timestamp, double equity);
    void update(Timestamp timestamp, const engine::Portfolio& portfolio,
                std::optional<regime::RegimeType> regime = std::nullopt);

    const EquityCurve& equity_curve() const { return equity_curve_; }
    const DrawdownTracker& drawdown() const { return drawdown_; }
    const AttributionTracker& attribution() const { return attribution_; }
    const RegimeAttribution& regime_attribution() const { return regime_attribution_; }
    const TransitionMetrics& transition_metrics() const { return transition_metrics_; }

private:
    EquityCurve equity_curve_;
    DrawdownTracker drawdown_;
    AttributionTracker attribution_;
    RegimeAttribution regime_attribution_;
    TransitionMetrics transition_metrics_;
    double last_equity_ = 0.0;
    std::optional<regime::RegimeType> last_regime_;
};

}  // namespace regimeflow::metrics
