#include <gtest/gtest.h>

#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/events/event.h"
#include "regimeflow/strategy/strategy.h"

#include <filesystem>
#include <vector>

namespace regimeflow::test
{
    class CountingStrategy final : public strategy::Strategy {
    public:
        void initialize(strategy::StrategyContext&) override {}

        void on_bar(const data::Bar&) override { ++bar_count_; }
        void on_tick(const data::Tick&) override { ++tick_count_; }
        void on_timer(const std::string&) override { ++timer_count_; }

        [[nodiscard]] int bar_count() const { return bar_count_; }
        [[nodiscard]] int tick_count() const { return tick_count_; }
        [[nodiscard]] int timer_count() const { return timer_count_; }

    private:
        int bar_count_ = 0;
        int tick_count_ = 0;
        int timer_count_ = 0;
    };

    TEST(BacktestHooks, BarHookPriorityOrder) {
        engine::BacktestEngine engine(100000.0);

        std::vector<int> order;
        engine.register_hook(plugins::HookType::Bar,
                             [&](plugins::HookContext&) {
                                 order.push_back(2);
                                 return plugins::HookResult::Continue;
                             },
                             200);
        engine.register_hook(plugins::HookType::Bar,
                             [&](plugins::HookContext&) {
                                 order.push_back(1);
                                 return plugins::HookResult::Continue;
                             },
                             100);

        Config data_cfg;
        data_cfg.set("type", "csv");
        data_cfg.set("file_pattern", "{symbol}.csv");
        data_cfg.set("has_header", true);
        data_cfg.set("data_directory",
                     (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string());
        const auto source = data::DataSourceFactory::create(data_cfg);
        ASSERT_TRUE(source);

        std::vector<SymbolId> symbols = {SymbolRegistry::instance().intern("TEST")};
        TimeRange range;
        range.start = Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
        range.end = Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");
        auto bar_it = source->create_iterator(symbols, range, data::BarType::Time_1Day);
        auto tick_it = source->create_tick_iterator(symbols, range);
        auto book_it = source->create_book_iterator(symbols, range);
        engine.load_data(std::move(bar_it), std::move(tick_it), std::move(book_it));

        auto strategy = std::make_unique<CountingStrategy>();
        engine.set_strategy(std::move(strategy));
        engine.run();

        ASSERT_GE(order.size(), 1u);
        EXPECT_EQ(order[0], 1);
    }

    TEST(BacktestHooks, CancelSkipsBarProcessing) {
        engine::BacktestEngine engine(100000.0);

        engine.register_hook(plugins::HookType::Bar,
                             [&](plugins::HookContext&) {
                                 return plugins::HookResult::Cancel;
                             },
                             50);

        Config data_cfg;
        data_cfg.set("type", "csv");
        data_cfg.set("file_pattern", "{symbol}.csv");
        data_cfg.set("has_header", true);
        data_cfg.set("data_directory",
                     (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string());
        const auto source = data::DataSourceFactory::create(data_cfg);
        ASSERT_TRUE(source);

        const std::vector<SymbolId> symbols = {SymbolRegistry::instance().intern("TEST")};
        TimeRange range;
        range.start = Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
        range.end = Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");
        auto bar_it = source->create_iterator(symbols, range, data::BarType::Time_1Day);
        auto tick_it = source->create_tick_iterator(symbols, range);
        auto book_it = source->create_book_iterator(symbols, range);
        engine.load_data(std::move(bar_it), std::move(tick_it), std::move(book_it));

        auto strategy = std::make_unique<CountingStrategy>();
        const auto* strategy_ptr = strategy.get();
        engine.set_strategy(std::move(strategy));
        engine.run();

        EXPECT_EQ(strategy_ptr->bar_count(), 0);
    }

    TEST(BacktestHooks, ProgressCallbackReportsCompletion) {
        engine::BacktestEngine engine(100000.0);

        std::vector<std::string> messages;
        engine.on_progress([&](double, const std::string& msg) {
            messages.push_back(msg);
        });

        Config data_cfg;
        data_cfg.set("type", "csv");
        data_cfg.set("file_pattern", "{symbol}.csv");
        data_cfg.set("has_header", true);
        data_cfg.set("data_directory",
                     (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string());
        const auto source = data::DataSourceFactory::create(data_cfg);
        ASSERT_TRUE(source);

        const std::vector<SymbolId> symbols = {SymbolRegistry::instance().intern("TEST")};
        TimeRange range;
        range.start = Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
        range.end = Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");
        auto bar_it = source->create_iterator(symbols, range, data::BarType::Time_1Day);
        auto tick_it = source->create_tick_iterator(symbols, range);
        auto book_it = source->create_book_iterator(symbols, range);
        engine.load_data(std::move(bar_it), std::move(tick_it), std::move(book_it));

        auto strategy = std::make_unique<CountingStrategy>();
        engine.set_strategy(std::move(strategy));
        engine.run();

        ASSERT_FALSE(messages.empty());
        EXPECT_EQ(messages.back(), "complete");
    }

    TEST(BacktestHooks, TickAndTimerHooksInvoke) {
        engine::BacktestEngine engine(100000.0);

        bool tick_hook = false;
        bool timer_hook = false;
        engine.register_hook(plugins::HookType::Tick,
                             [&](plugins::HookContext&) {
                                 tick_hook = true;
                                 return plugins::HookResult::Continue;
                             });
        engine.register_hook(plugins::HookType::Timer,
                             [&](const plugins::HookContext& ctx) {
                                 timer_hook = (ctx.timer_id() == "t1");
                                 return plugins::HookResult::Continue;
                             });

        auto strategy = std::make_unique<CountingStrategy>();
        const auto* strategy_ptr = strategy.get();
        engine.set_strategy(std::move(strategy));

        const SymbolId symbol = SymbolRegistry::instance().intern("TEST");
        data::Tick tick;
        tick.symbol = symbol;
        tick.price = 100.0;
        tick.quantity = 1.0;
        tick.timestamp = Timestamp(0);
        engine.enqueue(events::make_market_event(tick));

        engine.enqueue(events::make_system_event(events::SystemEventKind::Timer, Timestamp(1), 0, "t1"));

        engine.run();

        EXPECT_TRUE(tick_hook);
        EXPECT_TRUE(timer_hook);
        EXPECT_EQ(strategy_ptr->tick_count(), 1);
        EXPECT_EQ(strategy_ptr->timer_count(), 1);
    }

    TEST(BacktestHooks, RealTickModeSkipsSyntheticBarExecutionAfterTickSeen) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("simulation.tick_mode", "real_ticks");
        exec_cfg.set_path("simulation.bar_mode", "intrabar_ohlc");
        engine.configure_execution(exec_cfg);

        const SymbolId symbol = SymbolRegistry::instance().intern("REALTICK");

        data::Tick first_tick;
        first_tick.symbol = symbol;
        first_tick.price = 100.0;
        first_tick.quantity = 1.0;
        first_tick.timestamp = Timestamp(0);
        engine.enqueue(events::make_market_event(first_tick));
        ASSERT_TRUE(engine.step());

        auto order = engine::Order::limit(symbol, engine::OrderSide::Buy, 1.0, 99.0);
        order.created_at = Timestamp(1);
        const auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());
        const auto order_id = result.value();

        data::Bar bar;
        bar.symbol = symbol;
        bar.open = 100.0;
        bar.high = 101.0;
        bar.low = 98.0;
        bar.close = 100.0;
        bar.volume = 100;
        bar.timestamp = Timestamp(2);
        engine.enqueue(events::make_market_event(bar));
        ASSERT_TRUE(engine.step());

        const auto after_bar = engine.order_manager().get_order(order_id);
        ASSERT_TRUE(after_bar.has_value());
        EXPECT_EQ(after_bar->status, engine::OrderStatus::Pending);

        data::Tick fill_tick = first_tick;
        fill_tick.price = 98.5;
        fill_tick.timestamp = Timestamp(3);
        engine.enqueue(events::make_market_event(fill_tick));
        ASSERT_TRUE(engine.step());
        ASSERT_TRUE(engine.step());

        const auto after_tick = engine.order_manager().get_order(order_id);
        ASSERT_TRUE(after_tick.has_value());
        EXPECT_EQ(after_tick->status, engine::OrderStatus::Filled);
    }

    TEST(BacktestHooks, SyntheticOhlcTickProfileFillsFromBarRange) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("simulation.tick_mode", "synthetic_ticks");
        exec_cfg.set_path("simulation.synthetic_tick_profile", "ohlc_4tick");
        engine.configure_execution(exec_cfg);

        const SymbolId symbol = SymbolRegistry::instance().intern("SYNTH");
        auto order = engine::Order::limit(symbol, engine::OrderSide::Buy, 1.0, 99.0);
        order.created_at = Timestamp(1);
        const auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());
        const auto order_id = result.value();

        data::Bar bar;
        bar.symbol = symbol;
        bar.open = 100.0;
        bar.high = 101.0;
        bar.low = 98.0;
        bar.close = 100.0;
        bar.volume = 100;
        bar.timestamp = Timestamp(2);
        engine.enqueue(events::make_market_event(bar));
        ASSERT_TRUE(engine.step());
        ASSERT_TRUE(engine.step());

        const auto filled_order = engine.order_manager().get_order(order_id);
        ASSERT_TRUE(filled_order.has_value());
        EXPECT_EQ(filled_order->status, engine::OrderStatus::Filled);
    }

    TEST(BacktestHooks, TradingHaltAndResumeSystemEventsGateExecution) {
        engine::BacktestEngine engine(100000.0);

        const SymbolId symbol = SymbolRegistry::instance().intern("HALT_EVT");
        data::Quote quote;
        quote.symbol = symbol;
        quote.bid = 99.0;
        quote.ask = 100.0;
        quote.timestamp = Timestamp(1);
        engine.enqueue(events::make_market_event(quote));
        ASSERT_TRUE(engine.step());

        auto order = engine::Order::limit(symbol, engine::OrderSide::Buy, 1.0, 99.0);
        order.created_at = Timestamp(2);
        const auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());
        const auto order_id = result.value();

        engine.enqueue(events::make_system_event(events::SystemEventKind::TradingHalt,
                                                 Timestamp(3),
                                                 0,
                                                 "HALT_EVT"));
        ASSERT_TRUE(engine.step());

        auto live_quote = quote;
        live_quote.timestamp = Timestamp(4);
        live_quote.ask = 99.0;
        engine.enqueue(events::make_market_event(live_quote));
        ASSERT_TRUE(engine.step());
        const auto halted_order = engine.order_manager().get_order(order_id);
        ASSERT_TRUE(halted_order.has_value());
        EXPECT_EQ(halted_order->status, engine::OrderStatus::Pending);

        engine.enqueue(events::make_system_event(events::SystemEventKind::TradingResume,
                                                 Timestamp(5),
                                                 0,
                                                 "HALT_EVT"));
        ASSERT_TRUE(engine.step());

        live_quote.timestamp = Timestamp(6);
        engine.enqueue(events::make_market_event(live_quote));
        ASSERT_TRUE(engine.step());
        ASSERT_TRUE(engine.step());

        const auto resumed_order = engine.order_manager().get_order(order_id);
        ASSERT_TRUE(resumed_order.has_value());
        EXPECT_EQ(resumed_order->status, engine::OrderStatus::Filled);
    }

    TEST(BacktestHooks, SessionCalendarConfigBlocksClosedDates) {
        engine::BacktestEngine engine(100000.0);

        Config exec_cfg;
        exec_cfg.set_path("session.enabled", true);
        exec_cfg.set_path("session.weekdays",
                          ConfigValue::Array{ConfigValue("mon"),
                                             ConfigValue("tue"),
                                             ConfigValue("wed"),
                                             ConfigValue("thu"),
                                             ConfigValue("fri")});
        exec_cfg.set_path("session.closed_dates",
                          ConfigValue::Array{ConfigValue("2020-01-01")});
        engine.configure_execution(exec_cfg);

        const SymbolId symbol = SymbolRegistry::instance().intern("HOLIDAY_CFG");
        data::Quote holiday_quote;
        holiday_quote.symbol = symbol;
        holiday_quote.bid = 99.0;
        holiday_quote.ask = 100.0;
        holiday_quote.timestamp = Timestamp::from_string("2020-01-01 10:00:00", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(holiday_quote));
        ASSERT_TRUE(engine.step());

        auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 1.0);
        order.created_at = Timestamp::from_string("2020-01-01 10:00:01", "%Y-%m-%d %H:%M:%S");
        order.tif = engine::TimeInForce::GTC;
        const auto result = engine.order_manager().submit_order(order);
        ASSERT_TRUE(result.is_ok());
        const auto order_id = result.value();

        auto trading_quote = holiday_quote;
        trading_quote.timestamp = Timestamp::from_string("2020-01-02 10:00:00", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(trading_quote));
        ASSERT_TRUE(engine.step());
        ASSERT_TRUE(engine.step());

        const auto filled_order = engine.order_manager().get_order(order_id);
        ASSERT_TRUE(filled_order.has_value());
        EXPECT_EQ(filled_order->status, engine::OrderStatus::Filled);
    }
}  // namespace regimeflow::test
