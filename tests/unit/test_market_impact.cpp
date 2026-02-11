#include <gtest/gtest.h>

#include "regimeflow/execution/market_impact.h"
#include "regimeflow/data/order_book.h"

namespace {

TEST(MarketImpactTest, OrderBookImpactScalesWithLiquidity) {
    regimeflow::execution::OrderBookImpactModel impact(50.0);
    regimeflow::engine::Order order;
    order.side = regimeflow::engine::OrderSide::Buy;
    order.quantity = 100;

    regimeflow::data::OrderBook book{};
    book.asks[0].quantity = 1000;
    book.asks[1].quantity = 1000;

    double bps = impact.impact_bps(order, &book);
    EXPECT_GT(bps, 0.0);
    EXPECT_LT(bps, 50.0);
}

}  // namespace
