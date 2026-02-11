#include <gtest/gtest.h>

#include "regimeflow/execution/slippage.h"

namespace regimeflow::test {

TEST(SlippageModels, RegimeBpsUsesMetadata) {
    std::unordered_map<regimeflow::regime::RegimeType, double> map;
    map[regimeflow::regime::RegimeType::Bear] = 20.0;

    regimeflow::execution::RegimeBpsSlippageModel model(5.0, map);

    regimeflow::engine::Order order = regimeflow::engine::Order::market(
        regimeflow::SymbolRegistry::instance().intern("AAA"),
        regimeflow::engine::OrderSide::Buy, 10);
    order.metadata["regime"] = "bear";

    double price = model.execution_price(order, 100.0);
    EXPECT_NEAR(price, 100.2, 1e-6); // 20 bps
}

TEST(SlippageModels, RegimeBpsFallsBackToDefault) {
    std::unordered_map<regimeflow::regime::RegimeType, double> map;
    regimeflow::execution::RegimeBpsSlippageModel model(10.0, map);

    regimeflow::engine::Order order = regimeflow::engine::Order::market(
        regimeflow::SymbolRegistry::instance().intern("AAA"),
        regimeflow::engine::OrderSide::Sell, 10);

    double price = model.execution_price(order, 100.0);
    EXPECT_NEAR(price, 99.9, 1e-6); // 10 bps sell
}

}  // namespace regimeflow::test
