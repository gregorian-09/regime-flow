#pragma once

#include "regimeflow/metrics/drawdown.h"
#include "regimeflow/metrics/metrics_tracker.h"
#include "regimeflow/metrics/performance_calculator.h"
#include "regimeflow/metrics/performance_metrics.h"
#include "regimeflow/metrics/regime_attribution.h"
#include "regimeflow/metrics/transition_metrics.h"

#include <map>

namespace regimeflow::metrics {

struct Report {
    PerformanceStats performance;
    PerformanceSummary performance_summary;
    double max_drawdown = 0.0;
    Timestamp max_drawdown_start;
    Timestamp max_drawdown_end;
    std::map<regime::RegimeType, RegimePerformance> regime_performance;
    std::map<std::pair<regime::RegimeType, regime::RegimeType>, TransitionStats> transitions;
};

Report build_report(const MetricsTracker& tracker, double periods_per_year);
Report build_report(const MetricsTracker& tracker,
                    const std::vector<engine::Fill>& fills,
                    double risk_free_rate = 0.0,
                    const std::vector<double>* benchmark_returns = nullptr);

}  // namespace regimeflow::metrics
