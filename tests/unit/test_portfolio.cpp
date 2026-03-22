#include "regimeflow/engine/portfolio.h"

#include <gtest/gtest.h>

using regimeflow::SymbolRegistry;
using regimeflow::Timestamp;
using regimeflow::engine::Fill;
using regimeflow::engine::MarginProfile;
using regimeflow::engine::Portfolio;

TEST(PortfolioTest, MarginSnapshotComputesDerivedAccountState) {
    Portfolio portfolio(100000.0);

    MarginProfile profile;
    profile.initial_margin_ratio = 0.5;
    profile.maintenance_margin_ratio = 0.25;
    profile.stop_out_margin_level = 0.5;
    portfolio.configure_margin(profile);

    Fill fill;
    fill.symbol = SymbolRegistry::instance().intern("MARGIN");
    fill.quantity = 1000.0;
    fill.price = 60.0;
    fill.timestamp = Timestamp::now();
    portfolio.update_position(fill);

    const auto margin = portfolio.margin_snapshot();
    EXPECT_DOUBLE_EQ(margin.initial_margin, 30000.0);
    EXPECT_DOUBLE_EQ(margin.maintenance_margin, 15000.0);
    EXPECT_DOUBLE_EQ(margin.available_funds, 70000.0);
    EXPECT_DOUBLE_EQ(margin.margin_excess, 85000.0);
    EXPECT_DOUBLE_EQ(margin.buying_power, 140000.0);
    EXPECT_FALSE(margin.margin_call);
    EXPECT_FALSE(margin.stop_out);
}

TEST(PortfolioTest, SnapshotCarriesMarginFieldsAndStopOutState) {
    Portfolio portfolio(100000.0);

    MarginProfile profile;
    profile.initial_margin_ratio = 0.5;
    profile.maintenance_margin_ratio = 0.25;
    profile.stop_out_margin_level = 0.5;
    portfolio.configure_margin(profile);

    Fill fill;
    fill.symbol = SymbolRegistry::instance().intern("STOP");
    fill.quantity = 2000.0;
    fill.price = 100.0;
    fill.timestamp = Timestamp::now();
    portfolio.update_position(fill);
    portfolio.mark_to_market(fill.symbol, 20.0, fill.timestamp);

    const auto snapshot = portfolio.snapshot(fill.timestamp);
    EXPECT_DOUBLE_EQ(snapshot.initial_margin, 20000.0);
    EXPECT_DOUBLE_EQ(snapshot.maintenance_margin, 10000.0);
    EXPECT_DOUBLE_EQ(snapshot.available_funds, -80000.0);
    EXPECT_DOUBLE_EQ(snapshot.margin_excess, -70000.0);
    EXPECT_DOUBLE_EQ(snapshot.buying_power, 0.0);
    EXPECT_TRUE(snapshot.margin_call);
    EXPECT_TRUE(snapshot.stop_out);
}
