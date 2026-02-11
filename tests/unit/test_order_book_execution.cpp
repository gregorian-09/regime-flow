#include <gtest/gtest.h>

#include "regimeflow/execution/order_book_execution_model.h"

namespace {

TEST(OrderBookExecutionModelTest, PartialFillWhenLiquidityInsufficient) {
    auto book = std::make_shared<regimeflow::data::OrderBook>();
    book->asks[0].price = 100.0;
    book->asks[0].quantity = 50.0;
    book->asks[1].price = 101.0;
    book->asks[1].quantity = 25.0;

    regimeflow::execution::OrderBookExecutionModel model(book);
    regimeflow::engine::Order order;
    order.id = 1;
    order.symbol = 1;
    order.side = regimeflow::engine::OrderSide::Buy;
    order.quantity = 200.0;

    auto fills = model.execute(order, 100.0, regimeflow::Timestamp::now());
    double filled = 0.0;
    for (const auto& f : fills) {
        filled += std::abs(f.quantity);
    }
    EXPECT_EQ(filled, 75.0);
}

}  // namespace
