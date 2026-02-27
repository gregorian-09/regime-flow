#include "regimeflow/engine/order_manager.h"

#include <gtest/gtest.h>

using regimeflow::Timestamp;
using regimeflow::engine::Order;
using regimeflow::engine::OrderManager;
using regimeflow::engine::OrderSide;
using regimeflow::engine::OrderType;
using regimeflow::engine::TimeInForce;

TEST(OrderTifTest, GtdRequiresExpireAt) {
    OrderManager manager;
    Order order = Order::market(regimeflow::SymbolRegistry::instance().intern("AAA"),
                                OrderSide::Buy,
                                10.0);
    order.tif = TimeInForce::GTD;
    auto result = manager.submit_order(order);
    EXPECT_TRUE(result.is_err());
}

TEST(OrderTifTest, GtdAllowsExpireAt) {
    OrderManager manager;
    Order order = Order::market(regimeflow::SymbolRegistry::instance().intern("AAA"),
                                OrderSide::Buy,
                                10.0);
    order.tif = TimeInForce::GTD;
    order.expire_at = Timestamp::now();
    auto result = manager.submit_order(order);
    EXPECT_TRUE(result.is_ok());
}
