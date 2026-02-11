#include <gtest/gtest.h>

#include "regimeflow/metrics/performance_metrics.h"

namespace regimeflow::test {

TEST(PerformanceMetrics, ComputesMaxDrawdownAndCalmar) {
    regimeflow::metrics::EquityCurve curve;
    curve.add_point(regimeflow::Timestamp(0), 100.0);
    curve.add_point(regimeflow::Timestamp(regimeflow::Duration::days(100).total_microseconds()), 120.0);
    curve.add_point(regimeflow::Timestamp(regimeflow::Duration::days(200).total_microseconds()), 80.0);
    curve.add_point(regimeflow::Timestamp(regimeflow::Duration::days(365).total_microseconds()), 130.0);

    auto stats = regimeflow::metrics::compute_stats(curve, 252.0);
    EXPECT_NEAR(stats.max_drawdown, 0.3333333, 1e-4);
    EXPECT_NEAR(stats.cagr, 0.3, 1e-3);
    EXPECT_NEAR(stats.calmar, 0.9, 1e-3);
    EXPECT_GT(stats.var_95, 0.0);
    EXPECT_GE(stats.cvar_95, stats.var_95);
}

}  // namespace regimeflow::test
