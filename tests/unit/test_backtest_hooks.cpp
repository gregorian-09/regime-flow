#include <gtest/gtest.h>

#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/events/event.h"
#include "regimeflow/strategy/strategy.h"

#include <filesystem>
#include <vector>

namespace regimeflow::test {

class CountingStrategy final : public strategy::Strategy {
public:
    void initialize(strategy::StrategyContext&) override {}

    void on_bar(const data::Bar&) override { ++bar_count_; }
    void on_tick(const data::Tick&) override { ++tick_count_; }
    void on_timer(const std::string&) override { ++timer_count_; }

    int bar_count() const { return bar_count_; }
    int tick_count() const { return tick_count_; }
    int timer_count() const { return timer_count_; }

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
    auto source = data::DataSourceFactory::create(data_cfg);
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
    auto source = data::DataSourceFactory::create(data_cfg);
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
    auto* strategy_ptr = strategy.get();
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
    auto source = data::DataSourceFactory::create(data_cfg);
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
                         [&](plugins::HookContext& ctx) {
                             timer_hook = (ctx.timer_id() == "t1");
                             return plugins::HookResult::Continue;
                         });

    auto strategy = std::make_unique<CountingStrategy>();
    auto* strategy_ptr = strategy.get();
    engine.set_strategy(std::move(strategy));

    SymbolId symbol = SymbolRegistry::instance().intern("TEST");
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

}  // namespace regimeflow::test
