#include <gtest/gtest.h>

#include "regimeflow/metrics/performance_calculator.h"

namespace regimeflow::test {

using regimeflow::Duration;
using regimeflow::Timestamp;

engine::PortfolioSnapshot make_snapshot(Timestamp ts, double equity) {
    engine::PortfolioSnapshot snap;
    snap.timestamp = ts;
    snap.equity = equity;
    return snap;
}

TEST(PerformanceCalculator, ComputesSummaryWithBenchmarkAndTrades) {
    metrics::PerformanceCalculator calculator;

    std::vector<engine::PortfolioSnapshot> curve;
    curve.push_back(make_snapshot(Timestamp(0), 100.0));
    curve.push_back(make_snapshot(Timestamp(Duration::days(1).total_microseconds()), 110.0));
    curve.push_back(make_snapshot(Timestamp(Duration::days(2).total_microseconds()), 99.0));
    curve.push_back(make_snapshot(Timestamp(Duration::days(3).total_microseconds()), 108.9));

    std::vector<engine::Fill> fills;
    engine::Fill buy1;
    buy1.symbol = 1;
    buy1.quantity = 10.0;
    buy1.price = 10.0;
    buy1.commission = 1.0;
    buy1.timestamp = curve[1].timestamp;
    fills.push_back(buy1);

    engine::Fill sell1;
    sell1.symbol = 1;
    sell1.quantity = -10.0;
    sell1.price = 12.0;
    sell1.commission = 1.0;
    sell1.timestamp = curve[2].timestamp;
    fills.push_back(sell1);

    engine::Fill buy2;
    buy2.symbol = 1;
    buy2.quantity = 5.0;
    buy2.price = 20.0;
    buy2.commission = 0.5;
    buy2.timestamp = curve[2].timestamp;
    fills.push_back(buy2);

    engine::Fill sell2;
    sell2.symbol = 1;
    sell2.quantity = -5.0;
    sell2.price = 19.0;
    sell2.commission = 0.5;
    sell2.timestamp = curve[3].timestamp;
    fills.push_back(sell2);

    std::vector<double> benchmark = {0.05, -0.02, 0.03};

    auto summary = calculator.calculate(curve, fills, 0.0, &benchmark);
    EXPECT_NEAR(summary.total_return, 0.089, 1e-3);
    EXPECT_NEAR(summary.best_day, 0.1, 1e-6);
    EXPECT_NEAR(summary.worst_day, -0.1, 1e-6);
    EXPECT_NEAR(summary.avg_monthly_return, 0.089, 1e-3);
    EXPECT_GT(summary.downside_deviation, 0.0);
    EXPECT_GT(summary.var_95, 0.0);
    EXPECT_GT(summary.cvar_95, 0.0);
    EXPECT_GT(summary.information_ratio, 0.0);
    EXPECT_GT(summary.treynor_ratio, 0.0);

    EXPECT_EQ(summary.total_trades, 2);
    EXPECT_EQ(summary.winning_trades, 1);
    EXPECT_EQ(summary.losing_trades, 1);
    EXPECT_NEAR(summary.win_rate, 0.5, 1e-6);
    EXPECT_NEAR(summary.avg_win, 18.0, 1e-6);
    EXPECT_NEAR(summary.avg_loss, -6.0, 1e-6);
    EXPECT_NEAR(summary.win_loss_ratio, 3.0, 1e-6);
    EXPECT_NEAR(summary.profit_factor, 3.0, 1e-6);
    EXPECT_NEAR(summary.expectancy, 6.0, 1e-6);
    EXPECT_GT(summary.annual_turnover, 0.0);
}

TEST(PerformanceCalculator, AggregatesRegimeAndTransitionMetrics) {
    metrics::PerformanceCalculator calculator;

    std::vector<engine::PortfolioSnapshot> curve;
    curve.push_back(make_snapshot(Timestamp(0), 100.0));
    curve.push_back(make_snapshot(Timestamp(Duration::days(1).total_microseconds()), 105.0));
    curve.push_back(make_snapshot(Timestamp(Duration::days(2).total_microseconds()), 95.0));
    curve.push_back(make_snapshot(Timestamp(Duration::days(3).total_microseconds()), 98.0));

    std::vector<engine::Fill> fills;
    engine::Fill fill;
    fill.symbol = 1;
    fill.quantity = 1.0;
    fill.price = 100.0;
    fill.timestamp = curve[1].timestamp;
    fills.push_back(fill);

    std::vector<regime::RegimeState> regimes;
    regimes.push_back({regime::RegimeType::Bull, 0.9, {}, {}, 0, curve[0].timestamp});
    regimes.push_back({regime::RegimeType::Bear, 0.8, {}, {}, 0, curve[2].timestamp});

    auto by_regime = calculator.calculate_by_regime(curve, fills, regimes);
    EXPECT_NEAR(by_regime[regime::RegimeType::Bull].time_percentage, 0.5, 1e-6);
    EXPECT_NEAR(by_regime[regime::RegimeType::Bear].time_percentage, 0.5, 1e-6);

    std::vector<regime::RegimeTransition> transitions;
    transitions.push_back({regime::RegimeType::Bull, regime::RegimeType::Bear, curve[1].timestamp});
    transitions.push_back({regime::RegimeType::Bear, regime::RegimeType::Bull, curve[2].timestamp});
    transitions.push_back({regime::RegimeType::Bull, regime::RegimeType::Bear, curve[3].timestamp});

    auto metrics = calculator.calculate_transitions(curve, transitions);
    bool found_bull_bear = false;
    for (const auto& entry : metrics) {
        if (entry.from == regime::RegimeType::Bull && entry.to == regime::RegimeType::Bear) {
            EXPECT_EQ(entry.occurrences, 2);
            found_bull_bear = true;
        }
    }
    EXPECT_TRUE(found_bull_bear);
}

}  // namespace regimeflow::test
