#pragma once

#include "regimeflow/metrics/performance.h"

#include <vector>

namespace regimeflow::metrics {

struct PerformanceStats {
    double total_return = 0.0;
    double cagr = 0.0;
    double volatility = 0.0;
    double sharpe = 0.0;
    double sortino = 0.0;
    double calmar = 0.0;
    double max_drawdown = 0.0;
    double var_95 = 0.0;
    double cvar_95 = 0.0;
    double best_return = 0.0;
    double worst_return = 0.0;
};

PerformanceStats compute_stats(const EquityCurve& curve, double periods_per_year);

}  // namespace regimeflow::metrics
