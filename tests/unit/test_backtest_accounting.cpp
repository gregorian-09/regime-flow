#include <gtest/gtest.h>

#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/events/event.h"

namespace regimeflow::test
{
    TEST(BacktestAccounting, StopOutLiquidatesWorstPositionAndHaltsTrading) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("account.margin.initial_margin_ratio", 0.5);
        exec_cfg.set_path("account.margin.maintenance_margin_ratio", 0.25);
        exec_cfg.set_path("account.margin.stop_out_margin_level", 0.75);
        exec_cfg.set_path("account.enforcement.enabled", true);
        exec_cfg.set_path("account.enforcement.margin_call_action", "cancel_open_orders");
        exec_cfg.set_path("account.enforcement.stop_out_action", "liquidate_worst_first");
        exec_cfg.set_path("account.enforcement.halt_after_stop_out", true);
        exec_cfg.set_path("commission.type", "fixed");
        exec_cfg.set_path("commission.amount", 5.0);
        exec_cfg.set_path("transaction_cost.type", "per_order");
        exec_cfg.set_path("transaction_cost.per_order", 2.5);
        engine.configure_execution(exec_cfg);

        const SymbolId weak = SymbolRegistry::instance().intern("WEAK");
        const SymbolId strong = SymbolRegistry::instance().intern("STRONG");
        const Timestamp start(1);
        engine.portfolio().set_cash(-50000.0, start);
        engine.portfolio().set_position(weak, 1000.0, 100.0, 100.0, start);
        engine.portfolio().set_position(strong, 500.0, 100.0, 100.0, start);

        auto resting = engine::Order::limit(strong, engine::OrderSide::Buy, 1.0, 99.0);
        resting.created_at = start;
        const auto submit_result = engine.order_manager().submit_order(resting);
        ASSERT_TRUE(submit_result.is_ok());
        const auto resting_id = submit_result.value();

        data::Bar weak_bar;
        weak_bar.symbol = weak;
        weak_bar.open = 100.0;
        weak_bar.high = 100.0;
        weak_bar.low = 10.0;
        weak_bar.close = 10.0;
        weak_bar.volume = 1;
        weak_bar.timestamp = Timestamp(2);
        engine.enqueue(events::make_market_event(weak_bar));
        ASSERT_TRUE(engine.step());

        const auto weak_position = engine.portfolio().get_position(weak);
        ASSERT_TRUE(weak_position.has_value());
        EXPECT_DOUBLE_EQ(weak_position->quantity, 0.0);
        EXPECT_DOUBLE_EQ(engine.portfolio().cash(), -40007.5);

        const auto strong_position = engine.portfolio().get_position(strong);
        ASSERT_TRUE(strong_position.has_value());
        EXPECT_DOUBLE_EQ(strong_position->quantity, 500.0);

        const auto snapshot = engine.dashboard_snapshot();
        EXPECT_DOUBLE_EQ(snapshot.total_commission, 5.0);
        EXPECT_DOUBLE_EQ(snapshot.total_transaction_cost, 2.5);
        EXPECT_DOUBLE_EQ(snapshot.total_cost, 7.5);

        const auto cancelled = engine.order_manager().get_order(resting_id);
        ASSERT_TRUE(cancelled.has_value());
        EXPECT_EQ(cancelled->status, engine::OrderStatus::Cancelled);

        auto rejected = engine::Order::market(strong, engine::OrderSide::Buy, 1.0);
        rejected.created_at = Timestamp(3);
        EXPECT_TRUE(engine.order_manager().submit_order(rejected).is_err());
    }

    TEST(BacktestAccounting, DayTransitionAppliesFinancingCharges) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("account.financing.enabled", true);
        exec_cfg.set_path("account.financing.long_rate_bps_per_day", 10.0);
        exec_cfg.set_path("account.financing.short_borrow_bps_per_day", 20.0);
        engine.configure_execution(exec_cfg);

        const SymbolId symbol = SymbolRegistry::instance().intern("CARRY");
        const Timestamp start = Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
        engine.portfolio().set_cash(0.0, start);
        engine.portfolio().set_position(symbol, 1000.0, 100.0, 100.0, start);

        data::Bar first_bar;
        first_bar.symbol = symbol;
        first_bar.open = 100.0;
        first_bar.high = 100.0;
        first_bar.low = 100.0;
        first_bar.close = 100.0;
        first_bar.volume = 1;
        first_bar.timestamp = start;
        engine.enqueue(events::make_market_event(first_bar));
        ASSERT_TRUE(engine.step());

        data::Bar second_bar = first_bar;
        second_bar.timestamp = Timestamp::from_string("2020-01-02 00:00:00", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(second_bar));
        ASSERT_TRUE(engine.step());

        EXPECT_DOUBLE_EQ(engine.portfolio().cash(), -100.0);
        EXPECT_NEAR(engine.portfolio().equity(), 99900.0, 1e-9);
    }
}  // namespace regimeflow::test
