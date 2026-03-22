#include <gtest/gtest.h>

#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/engine/backtest_results.h"

namespace regimeflow::test
{
    TEST(BacktestResultsTest, VenueAnalyticsAggregatesCostsAndSplitAttribution) {
        engine::BacktestResults results;

        engine::Fill venue_a_one;
        venue_a_one.order_id = 1;
        venue_a_one.parent_order_id = 100;
        venue_a_one.quantity = 5.0;
        venue_a_one.price = 100.0;
        venue_a_one.commission = 1.0;
        venue_a_one.transaction_cost = 0.5;
        venue_a_one.slippage = 0.1;
        venue_a_one.is_maker = true;
        venue_a_one.venue = "lit_a";

        engine::Fill venue_a_two = venue_a_one;
        venue_a_two.order_id = 2;
        venue_a_two.parent_order_id = 100;
        venue_a_two.quantity = -3.0;
        venue_a_two.price = 101.0;
        venue_a_two.commission = 0.5;
        venue_a_two.transaction_cost = 0.25;
        venue_a_two.slippage = -0.05;
        venue_a_two.is_maker = false;

        engine::Fill venue_b;
        venue_b.order_id = 3;
        venue_b.quantity = 2.0;
        venue_b.price = 99.0;
        venue_b.commission = 0.2;
        venue_b.transaction_cost = 0.1;
        venue_b.slippage = 0.0;
        venue_b.is_maker = false;
        venue_b.venue = "lit_b";

        results.fills = {venue_a_one, venue_a_two, venue_b};

        const auto analytics = results.venue_analytics();
        ASSERT_EQ(analytics.size(), 2u);

        const auto& lit_a = analytics[0];
        EXPECT_EQ(lit_a.venue, "lit_a");
        EXPECT_EQ(lit_a.fill_count, 2u);
        EXPECT_EQ(lit_a.order_count, 2u);
        EXPECT_EQ(lit_a.parent_order_count, 1u);
        EXPECT_EQ(lit_a.split_fill_count, 2u);
        EXPECT_DOUBLE_EQ(lit_a.total_cost, 2.25);
        EXPECT_NEAR(lit_a.maker_fill_ratio, 0.5, 1e-9);
        EXPECT_GT(lit_a.avg_slippage_bps, 0.0);

        const auto& lit_b = analytics[1];
        EXPECT_EQ(lit_b.venue, "lit_b");
        EXPECT_EQ(lit_b.fill_count, 1u);
        EXPECT_EQ(lit_b.order_count, 1u);
        EXPECT_EQ(lit_b.parent_order_count, 0u);
        EXPECT_EQ(lit_b.split_fill_count, 0u);
        EXPECT_DOUBLE_EQ(lit_b.total_cost, 0.3);
    }

    TEST(BacktestResultsTest, DashboardSnapshotSummarizesHeadlineAndExecutionData) {
        engine::BacktestResults results;
        const auto symbol = SymbolRegistry::instance().intern("TEST");
        results.setup.strategy_name = "demo";
        results.setup.symbols = {"TEST"};
        results.setup.start_date = "2020-01-01";
        results.setup.end_date = "2020-01-02";
        results.setup.bar_type = "1d";
        results.setup.execution_model = "basic";
        results.setup.tick_mode = "synthetic_ticks";

        engine::Fill fill;
        fill.order_id = 7;
        fill.parent_order_id = 70;
        fill.symbol = symbol;
        fill.quantity = 2.0;
        fill.price = 100.0;
        fill.commission = 1.25;
        fill.transaction_cost = 0.75;
        fill.slippage = 0.1;
        fill.is_maker = true;
        fill.venue = "lit_a";
        results.fills = {fill};

        engine::Portfolio portfolio(10000.0);
        portfolio.set_cash(9900.0, Timestamp::from_date(2020, 1, 2));
        portfolio.set_position(symbol, 2.0, 100.0, 75.0, Timestamp::from_date(2020, 1, 2));
        portfolio.configure_margin({0.1, 0.06, 0.4});
        results.metrics.update(Timestamp::from_date(2020, 1, 2), portfolio);

        const auto dashboard = results.dashboard_snapshot();
        EXPECT_DOUBLE_EQ(dashboard.equity, 10050.0);
        EXPECT_DOUBLE_EQ(dashboard.cash, 9900.0);
        EXPECT_DOUBLE_EQ(dashboard.buying_power, 100350.0);
        EXPECT_DOUBLE_EQ(dashboard.initial_margin, 15.0);
        EXPECT_DOUBLE_EQ(dashboard.maintenance_margin, 9.0);
        EXPECT_DOUBLE_EQ(dashboard.available_funds, 10035.0);
        EXPECT_DOUBLE_EQ(dashboard.margin_excess, 10041.0);
        EXPECT_DOUBLE_EQ(dashboard.total_commission, 1.25);
        EXPECT_DOUBLE_EQ(dashboard.total_transaction_cost, 0.75);
        EXPECT_DOUBLE_EQ(dashboard.total_cost, 2.0);
        EXPECT_NEAR(dashboard.maker_fill_ratio, 1.0, 1e-9);
        EXPECT_EQ(dashboard.setup.symbols.size(), 1u);
        EXPECT_EQ(dashboard.setup.symbols.front(), "TEST");
        ASSERT_EQ(dashboard.venue_summary.size(), 1u);
        EXPECT_EQ(dashboard.venue_summary.front().venue, "lit_a");
        EXPECT_EQ(dashboard.position_count, 1u);

        const auto dashboard_json = results.dashboard_snapshot_json();
        EXPECT_NE(dashboard_json.find("\"setup\""), std::string::npos);
        EXPECT_NE(dashboard_json.find("\"headline\""), std::string::npos);
        EXPECT_NE(dashboard_json.find("\"venue_summary\""), std::string::npos);

        const auto terminal = results.dashboard_terminal({false, 3});
        EXPECT_NE(terminal.find("REGIMEFLOW STRATEGY TESTER"), std::string::npos);
        EXPECT_NE(terminal.find("SETUP"), std::string::npos);
        EXPECT_NE(terminal.find("SUMMARY"), std::string::npos);
        EXPECT_NE(terminal.find("lit_a"), std::string::npos);

        const auto venues_only = results.dashboard_terminal(
            engine::DashboardRenderOptions{false, 3, engine::DashboardTab::Venues});
        EXPECT_NE(venues_only.find("VENUES"), std::string::npos);
        EXPECT_EQ(venues_only.find("SETUP"), std::string::npos);
    }

    TEST(BacktestResultsTest, BacktestEngineDashboardReflectsCurrentSetup) {
        engine::BacktestEngine engine(100000.0, "USD");
        engine::DashboardSetup setup;
        setup.strategy_name = "demo";
        setup.symbols = {"TEST"};
        setup.start_date = "2020-01-01";
        setup.end_date = "2020-01-03";
        setup.bar_type = "1d";
        setup.execution_model = "basic";
        engine.set_dashboard_setup(setup);

        const auto snapshot = engine.dashboard_snapshot();
        EXPECT_EQ(snapshot.setup.strategy_name, "demo");
        ASSERT_EQ(snapshot.setup.symbols.size(), 1u);
        EXPECT_EQ(snapshot.setup.symbols.front(), "TEST");

        const auto json = engine.dashboard_snapshot_json();
        EXPECT_NE(json.find("\"setup\""), std::string::npos);

        const auto terminal = engine.dashboard_terminal({false, 3});
        EXPECT_NE(terminal.find("SETUP"), std::string::npos);
        EXPECT_NE(terminal.find("demo"), std::string::npos);

        const auto journal_only = engine.dashboard_terminal(
            engine::DashboardRenderOptions{false, 3, engine::DashboardTab::Journal});
        EXPECT_NE(journal_only.find("JOURNAL"), std::string::npos);
        EXPECT_EQ(journal_only.find("SETUP"), std::string::npos);
    }
}  // namespace regimeflow::test
