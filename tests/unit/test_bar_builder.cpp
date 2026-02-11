#include <gtest/gtest.h>

#include "regimeflow/data/bar_builder.h"
#include "regimeflow/common/types.h"

using namespace regimeflow;
using namespace regimeflow::data;

namespace {

Tick make_tick(SymbolId symbol, const std::string& ts, double price, double qty) {
    Tick tick;
    tick.symbol = symbol;
    tick.timestamp = Timestamp::from_string(ts, "%Y-%m-%d %H:%M:%S");
    tick.price = price;
    tick.quantity = qty;
    return tick;
}  // namespace

}

TEST(BarBuilder, VolumeBarEmitsAtThreshold) {
    BarBuilder::Config cfg;
    cfg.type = BarType::Volume;
    cfg.volume_threshold = 100;
    BarBuilder builder(cfg);

    SymbolId sym = SymbolRegistry::instance().intern("AAPL");
    auto t1 = make_tick(sym, "2024-01-01 00:00:00", 10.0, 40.0);
    auto t2 = make_tick(sym, "2024-01-01 00:00:01", 12.0, 60.0);

    EXPECT_FALSE(builder.process(t1).has_value());
    auto bar = builder.process(t2);
    ASSERT_TRUE(bar.has_value());
    EXPECT_EQ(bar->symbol, sym);
    EXPECT_EQ(bar->volume, 100);
    EXPECT_EQ(bar->open, 10.0);
    EXPECT_EQ(bar->close, 12.0);
    EXPECT_EQ(bar->high, 12.0);
    EXPECT_EQ(bar->low, 10.0);
    EXPECT_EQ(bar->trade_count, 2u);
    EXPECT_NEAR(bar->vwap, (10.0 * 40.0 + 12.0 * 60.0) / 100.0, 1e-9);
}

TEST(BarBuilder, TickBarCountsTicks) {
    BarBuilder::Config cfg;
    cfg.type = BarType::Tick;
    cfg.tick_threshold = 3;
    BarBuilder builder(cfg);

    SymbolId sym = SymbolRegistry::instance().intern("MSFT");
    EXPECT_FALSE(builder.process(make_tick(sym, "2024-01-01 00:00:00", 100.0, 1.0)));
    EXPECT_FALSE(builder.process(make_tick(sym, "2024-01-01 00:00:01", 101.0, 1.0)));
    auto bar = builder.process(make_tick(sym, "2024-01-01 00:00:02", 102.0, 1.0));
    ASSERT_TRUE(bar.has_value());
    EXPECT_EQ(bar->trade_count, 3u);
    EXPECT_EQ(bar->close, 102.0);
}

TEST(BarBuilder, DollarBarEmitsOnDollarThreshold) {
    BarBuilder::Config cfg;
    cfg.type = BarType::Dollar;
    cfg.dollar_threshold = 1000.0;
    BarBuilder builder(cfg);

    SymbolId sym = SymbolRegistry::instance().intern("TSLA");
    EXPECT_FALSE(builder.process(make_tick(sym, "2024-01-01 00:00:00", 50.0, 10.0)));
    auto bar = builder.process(make_tick(sym, "2024-01-01 00:00:01", 25.0, 20.0));
    ASSERT_TRUE(bar.has_value());
    EXPECT_EQ(bar->volume, 30u);
    EXPECT_EQ(bar->close, 25.0);
    EXPECT_NEAR(bar->vwap, 1000.0 / 30.0, 1e-9);
}

TEST(BarBuilder, TimeBarSplitsOnIntervalBoundary) {
    BarBuilder::Config cfg;
    cfg.type = BarType::Time_1Min;
    cfg.time_interval_ms = 60'000;
    BarBuilder builder(cfg);

    SymbolId sym = SymbolRegistry::instance().intern("NVDA");
    EXPECT_FALSE(builder.process(make_tick(sym, "2024-01-01 00:00:00", 10.0, 1.0)));
    EXPECT_FALSE(builder.process(make_tick(sym, "2024-01-01 00:00:30", 11.0, 1.0)));
    auto bar = builder.process(make_tick(sym, "2024-01-01 00:01:01", 12.0, 1.0));
    ASSERT_TRUE(bar.has_value());
    EXPECT_EQ(bar->timestamp.to_string(), "2024-01-01 00:00:00");
    EXPECT_EQ(bar->close, 11.0);
    EXPECT_EQ(bar->trade_count, 2u);
}
