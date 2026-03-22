#include <gtest/gtest.h>

#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/execution/commission.h"
#include "regimeflow/execution/transaction_cost.h"
#include "regimeflow/common/config.h"

namespace regimeflow::test
{
    TEST(TransactionCosts, AppliesToPortfolioCash) {
        engine::BacktestEngine engine(100000.0);
        engine.set_commission_model(std::make_unique<execution::ZeroCommissionModel>());
        engine.set_transaction_cost_model(
            std::make_unique<execution::FixedBpsTransactionCostModel>(10.0));

        auto symbol = SymbolRegistry::instance().intern("TST");
        data::Bar bar;
        bar.timestamp = Timestamp(1000);
        bar.symbol = symbol;
        bar.open = 100.0;
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.volume = 1;
        engine.market_data().update(bar);

        auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 10);
        auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());

        engine.event_loop().run();

        double expected = 100000.0 - (100.0 * 10.0) - 1.0;  // 10 bps of $1000 = $1
        EXPECT_NEAR(engine.portfolio().cash(), expected, 1e-6);
    }

    TEST(TransactionCosts, PerShareConfigApplied) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("transaction_cost.type", "per_share");
        exec_cfg.set_path("transaction_cost.per_share", 0.01);
        engine.configure_execution(exec_cfg);

        auto symbol = SymbolRegistry::instance().intern("TST");
        data::Bar bar;
        bar.timestamp = Timestamp(1000);
        bar.symbol = symbol;
        bar.open = 100.0;
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.volume = 1;
        engine.market_data().update(bar);

        auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 10);
        auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());

        engine.event_loop().run();

        double expected = 100000.0 - (100.0 * 10.0) - 0.10;
        EXPECT_NEAR(engine.portfolio().cash(), expected, 1e-6);
    }

    TEST(TransactionCosts, PerOrderConfigApplied) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("transaction_cost.type", "per_order");
        exec_cfg.set_path("transaction_cost.per_order", 2.5);
        engine.configure_execution(exec_cfg);

        auto symbol = SymbolRegistry::instance().intern("TST");
        data::Bar bar;
        bar.timestamp = Timestamp(1000);
        bar.symbol = symbol;
        bar.open = 100.0;
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.volume = 1;
        engine.market_data().update(bar);

        auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 1);
        auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());

        engine.event_loop().run();

        double expected = 100000.0 - 100.0 - 2.5;
        EXPECT_NEAR(engine.portfolio().cash(), expected, 1e-6);
    }

    TEST(TransactionCosts, TieredConfigApplied) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("transaction_cost.type", "tiered");
        ConfigValue::Array tiers;
        ConfigValue::Object tier1;
        tier1["max_notional"] = ConfigValue(500.0);
        tier1["bps"] = ConfigValue(10.0);
        ConfigValue::Object tier2;
        tier2["max_notional"] = ConfigValue(0.0);  // catch-all
        tier2["bps"] = ConfigValue(5.0);
        tiers.emplace_back(tier1);
        tiers.emplace_back(tier2);
        exec_cfg.set_path("transaction_cost.tiers", tiers);
        engine.configure_execution(exec_cfg);

        auto symbol = SymbolRegistry::instance().intern("TST");
        data::Bar bar;
        bar.timestamp = Timestamp(1000);
        bar.symbol = symbol;
        bar.open = 100.0;
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.volume = 1;
        engine.market_data().update(bar);

        auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 10);
        auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());

        engine.event_loop().run();

        double expected = 100000.0 - 100.0 * 10.0 - 0.5;  // 5 bps of $1000
        EXPECT_NEAR(engine.portfolio().cash(), expected, 1e-6);
    }

    TEST(TransactionCosts, MakerTakerRebateAppliedForMakerFill) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("transaction_cost.type", "maker_taker");
        exec_cfg.set_path("transaction_cost.maker_rebate_bps", 10.0);
        exec_cfg.set_path("transaction_cost.taker_fee_bps", 20.0);
        exec_cfg.set_path("queue.enabled", true);
        exec_cfg.set_path("queue.progress_fraction", 1.0);
        exec_cfg.set_path("queue.default_visible_qty", 1.0);
        engine.configure_execution(exec_cfg);

        const auto symbol = SymbolRegistry::instance().intern("MKR");
        data::Quote away;
        away.timestamp = Timestamp(1000);
        away.symbol = symbol;
        away.bid = 100.0;
        away.ask = 101.0;
        away.bid_size = 1.0;
        away.ask_size = 1.0;
        engine.market_data().update(away);

        auto order = engine::Order::limit(symbol, engine::OrderSide::Buy, 1.0, 100.0);
        order.created_at = away.timestamp;
        auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());

        auto touch = away;
        touch.timestamp = Timestamp(1001);
        touch.ask = 100.0;
        engine.enqueue(events::make_market_event(touch));
        ASSERT_TRUE(engine.step());
        ASSERT_TRUE(engine.step());

        const double expected = 100000.0 - 100.0 + 0.1;  // 10 bps rebate on $100 notional
        EXPECT_NEAR(engine.portfolio().cash(), expected, 1e-6);
    }

    TEST(TransactionCosts, MakerTakerFeeAppliedForTakerFill) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("transaction_cost.type", "maker_taker");
        exec_cfg.set_path("transaction_cost.maker_rebate_bps", 10.0);
        exec_cfg.set_path("transaction_cost.taker_fee_bps", 20.0);
        exec_cfg.set_path("queue.enabled", true);
        exec_cfg.set_path("queue.progress_fraction", 0.1);
        exec_cfg.set_path("queue.default_visible_qty", 10.0);
        engine.configure_execution(exec_cfg);

        const auto symbol = SymbolRegistry::instance().intern("TKR");
        data::Quote away;
        away.timestamp = Timestamp(2000);
        away.symbol = symbol;
        away.bid = 100.0;
        away.ask = 101.0;
        away.bid_size = 10.0;
        away.ask_size = 10.0;
        engine.market_data().update(away);

        auto order = engine::Order::limit(symbol, engine::OrderSide::Buy, 1.0, 100.0);
        order.created_at = away.timestamp;
        auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());

        auto through = away;
        through.timestamp = Timestamp(2001);
        through.ask = 99.5;
        engine.enqueue(events::make_market_event(through));
        ASSERT_TRUE(engine.step());
        ASSERT_TRUE(engine.step());

        const double expected_fee = 99.5 * 0.002;  // 20 bps taker fee
        const double expected = 100000.0 - 99.5 - expected_fee;
        EXPECT_NEAR(engine.portfolio().cash(), expected, 1e-6);
    }
}  // namespace regimeflow::test
