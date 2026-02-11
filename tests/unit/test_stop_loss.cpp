#include <gtest/gtest.h>

#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/engine/backtest_runner.h"
#include "regimeflow/strategy/strategy.h"

namespace regimeflow::test {

class BuyOnceStrategy final : public strategy::Strategy {
public:
    void initialize(strategy::StrategyContext&) override {}

    void on_bar(const data::Bar& bar) override {
        if (sent_) {
            return;
        }
        engine::Order order = engine::Order::market(bar.symbol, engine::OrderSide::Buy, 10);
        ctx_->submit_order(std::move(order));
        sent_ = true;
    }

private:
    bool sent_ = false;
};

static engine::Portfolio run_with_bars(const std::vector<data::Bar>& bars,
                                       const Config& risk_cfg) {
    engine::BacktestEngine engine(100000.0);
    engine.configure_risk(risk_cfg);

    data::MemoryDataSource source;
    auto symbol = bars.front().symbol;
    source.add_bars(symbol, bars);

    engine::BacktestRunner runner(&engine, &source);
    TimeRange range;
    range.start = bars.front().timestamp;
    range.end = bars.back().timestamp;
    runner.run(std::make_unique<BuyOnceStrategy>(), range, {symbol},
               data::BarType::Time_1Day);
    return engine.portfolio();
}

TEST(StopLoss, FixedStopTriggersExit) {
    auto symbol = SymbolRegistry::instance().intern("STOP");
    std::vector<data::Bar> bars;
    data::Bar b1{};
    b1.symbol = symbol;
    b1.timestamp = Timestamp(1'000'000);
    b1.open = b1.high = b1.low = b1.close = 100.0;
    b1.volume = 100;
    bars.push_back(b1);
    data::Bar b2 = b1;
    b2.timestamp = Timestamp(2'000'000);
    b2.close = 94.0;
    b2.low = 94.0;
    bars.push_back(b2);

    Config risk_cfg;
    risk_cfg.set_path("stop_loss.enable", true);
    risk_cfg.set_path("stop_loss.pct", 0.05);

    auto portfolio = run_with_bars(bars, risk_cfg);
    auto pos = portfolio.get_position(symbol);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->quantity, 0);
}

TEST(StopLoss, TrailingStopTriggersExit) {
    auto symbol = SymbolRegistry::instance().intern("TRAIL");
    std::vector<data::Bar> bars;
    data::Bar b1{};
    b1.symbol = symbol;
    b1.timestamp = Timestamp(1'000'000);
    b1.open = b1.high = b1.low = b1.close = 100.0;
    b1.volume = 100;
    bars.push_back(b1);
    data::Bar b2 = b1;
    b2.timestamp = Timestamp(2'000'000);
    b2.close = 110.0;
    b2.high = 110.0;
    b2.low = 100.0;
    bars.push_back(b2);
    data::Bar b3 = b1;
    b3.timestamp = Timestamp(3'000'000);
    b3.close = 98.0;
    b3.low = 98.0;
    bars.push_back(b3);

    Config risk_cfg;
    risk_cfg.set_path("trailing_stop.enable", true);
    risk_cfg.set_path("trailing_stop.pct", 0.1);

    auto portfolio = run_with_bars(bars, risk_cfg);
    auto pos = portfolio.get_position(symbol);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->quantity, 0);
}

TEST(StopLoss, AtrStopTriggersExit) {
    auto symbol = SymbolRegistry::instance().intern("ATR");
    std::vector<data::Bar> bars;
    data::Bar b1{};
    b1.symbol = symbol;
    b1.timestamp = Timestamp(1'000'000);
    b1.open = 100.0;
    b1.high = 102.0;
    b1.low = 100.0;
    b1.close = 101.0;
    b1.volume = 100;
    bars.push_back(b1);
    data::Bar b2 = b1;
    b2.timestamp = Timestamp(2'000'000);
    b2.high = 103.0;
    b2.low = 101.0;
    b2.close = 102.0;
    bars.push_back(b2);
    data::Bar b3 = b1;
    b3.timestamp = Timestamp(3'000'000);
    b3.high = 99.0;
    b3.low = 97.0;
    b3.close = 97.0;
    bars.push_back(b3);

    Config risk_cfg;
    risk_cfg.set_path("atr_stop.enable", true);
    risk_cfg.set_path("atr_stop.window", static_cast<int64_t>(2));
    risk_cfg.set_path("atr_stop.multiplier", 1.0);

    auto portfolio = run_with_bars(bars, risk_cfg);
    auto pos = portfolio.get_position(symbol);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->quantity, 0);
}

TEST(StopLoss, TimeStopTriggersExit) {
    auto symbol = SymbolRegistry::instance().intern("TIME");
    std::vector<data::Bar> bars;
    auto base = Timestamp::now();
    data::Bar b1{};
    b1.symbol = symbol;
    b1.timestamp = base;
    b1.open = b1.high = b1.low = b1.close = 100.0;
    b1.volume = 100;
    bars.push_back(b1);
    data::Bar b2 = b1;
    b2.timestamp = base + Duration::seconds(120);
    bars.push_back(b2);

    Config risk_cfg;
    risk_cfg.set_path("time_stop.enable", true);
    risk_cfg.set_path("time_stop.max_holding_seconds", static_cast<int64_t>(60));

    auto portfolio = run_with_bars(bars, risk_cfg);
    auto pos = portfolio.get_position(symbol);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->quantity, 0);
}

}  // namespace regimeflow::test
