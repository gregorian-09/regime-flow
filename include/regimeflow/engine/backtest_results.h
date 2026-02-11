#pragma once

#include "regimeflow/engine/order.h"
#include "regimeflow/metrics/metrics_tracker.h"

#include <vector>

namespace regimeflow::engine {

struct BacktestResults {
    double total_return = 0.0;
    double max_drawdown = 0.0;
    metrics::MetricsTracker metrics;
    std::vector<engine::Fill> fills;
};

}  // namespace regimeflow::engine
