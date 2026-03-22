#include "regimeflow/engine/order.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/order_routing.h"

#include <gtest/gtest.h>

using regimeflow::SymbolRegistry;
using regimeflow::engine::Order;
using regimeflow::engine::OrderSide;
using regimeflow::engine::OrderType;
using regimeflow::engine::OrderId;
using regimeflow::engine::RoutingConfig;
using regimeflow::engine::RoutingContext;
using regimeflow::engine::RoutingMode;
using regimeflow::engine::RoutingVenue;
using regimeflow::engine::SplitMode;
using regimeflow::engine::ParentAggregation;
using regimeflow::engine::SmartOrderRouter;

TEST(SmartOrderRouterTest, ConvertsMarketToLimitOnTightSpread) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;
    cfg.max_spread_bps = 10.0;
    cfg.passive_offset_bps = 0.0;
    cfg.default_venue = "lit_primary";

    SmartOrderRouter router(cfg);
    Order order = Order::market(SymbolRegistry::instance().intern("AAA"),
                                OrderSide::Buy,
                                10.0);

    RoutingContext ctx;
    ctx.bid = 100.0;
    ctx.ask = 100.05;

    auto plan = router.route(order, ctx);
    EXPECT_EQ(plan.routed_order.type, OrderType::Limit);
    EXPECT_NEAR(plan.routed_order.limit_price, 100.0, 1e-6);
    EXPECT_EQ(plan.routed_order.metadata.at("venue"), "lit_primary");
}

TEST(SmartOrderRouterTest, KeepsMarketOnWideSpread) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;
    cfg.max_spread_bps = 5.0;

    SmartOrderRouter router(cfg);
    Order order = Order::market(SymbolRegistry::instance().intern("BBB"),
                                OrderSide::Sell,
                                10.0);

    RoutingContext ctx;
    ctx.bid = 100.0;
    ctx.ask = 100.20;

    auto plan = router.route(order, ctx);
    EXPECT_EQ(plan.routed_order.type, OrderType::Market);
}

TEST(SmartOrderRouterTest, AggressiveRouteUsesAskForBuy) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;

    SmartOrderRouter router(cfg);
    Order order = Order::market(SymbolRegistry::instance().intern("CCC"),
                                OrderSide::Buy,
                                5.0);
    order.metadata["route_style"] = "aggressive";

    RoutingContext ctx;
    ctx.bid = 99.9;
    ctx.ask = 100.1;

    auto plan = router.route(order, ctx);
    EXPECT_EQ(plan.routed_order.type, OrderType::Limit);
    EXPECT_NEAR(plan.routed_order.limit_price, 100.1, 1e-6);
}

TEST(SmartOrderRouterTest, SplitRoutingCreatesChildrenOnActivate) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;
    cfg.split_mode = SplitMode::Parallel;
    cfg.min_child_qty = 1.0;
    RoutingVenue lit_primary;
    lit_primary.name = "lit_primary";
    lit_primary.weight = 0.6;
    RoutingVenue lit_mid;
    lit_mid.name = "lit_mid";
    lit_mid.weight = 0.4;
    cfg.venues = {lit_primary, lit_mid};

    regimeflow::engine::OrderManager manager;
    manager.set_router(std::make_unique<SmartOrderRouter>(cfg),
                       [](const Order&) { return RoutingContext{}; });

    Order order = Order::market(SymbolRegistry::instance().intern("DDD"),
                                OrderSide::Buy,
                                10.0);
    auto parent_res = manager.submit_order(order);
    ASSERT_TRUE(parent_res.is_ok());
    const auto parent_id = parent_res.value();
    const auto parent = manager.get_order(parent_id);
    ASSERT_TRUE(parent.has_value());
    EXPECT_TRUE(parent->is_parent);

    manager.activate_routed_order(parent_id);

    const auto open_orders = manager.get_open_orders();
    size_t child_count = 0;
    for (const auto& open : open_orders) {
        if (open.parent_id == parent_id) {
            ++child_count;
        }
    }
    EXPECT_EQ(child_count, 2u);
}

TEST(SmartOrderRouterTest, SequentialSplitDefersAggregation) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;
    cfg.split_mode = SplitMode::Sequential;
    cfg.parent_aggregation = ParentAggregation::Final;
    cfg.min_child_qty = 1.0;
    RoutingVenue lit_primary;
    lit_primary.name = "lit_primary";
    lit_primary.weight = 0.5;
    RoutingVenue lit_mid;
    lit_mid.name = "lit_mid";
    lit_mid.weight = 0.5;
    cfg.venues = {lit_primary, lit_mid};

    regimeflow::engine::OrderManager manager;
    manager.set_router(std::make_unique<SmartOrderRouter>(cfg),
                       [](const Order&) { return RoutingContext{}; });

    Order order = Order::market(SymbolRegistry::instance().intern("EEE"),
                                OrderSide::Buy,
                                10.0);
    auto parent_res = manager.submit_order(order);
    ASSERT_TRUE(parent_res.is_ok());
    const auto parent_id = parent_res.value();

    manager.activate_routed_order(parent_id);

    auto open_orders = manager.get_open_orders();
    OrderId first_child_id = 0;
    double first_child_qty = 0.0;
    for (const auto& open : open_orders) {
        if (open.parent_id == parent_id) {
            first_child_id = open.id;
            first_child_qty = open.quantity;
            break;
        }
    }
    ASSERT_NE(first_child_id, 0u);

    regimeflow::engine::Fill fill;
    fill.order_id = first_child_id;
    fill.symbol = order.symbol;
    fill.quantity = first_child_qty;
    fill.price = 100.0;
    fill.timestamp = regimeflow::Timestamp::now();
    manager.process_fill(fill);

    const auto parent_after_first = manager.get_order(parent_id);
    ASSERT_TRUE(parent_after_first.has_value());
    EXPECT_EQ(parent_after_first->filled_quantity, 0.0);

    open_orders = manager.get_open_orders();
    OrderId second_child_id = 0;
    double second_child_qty = 0.0;
    for (const auto& open : open_orders) {
        if (open.parent_id == parent_id) {
            second_child_id = open.id;
            second_child_qty = open.quantity;
            break;
        }
    }
    ASSERT_NE(second_child_id, 0u);

    fill.order_id = second_child_id;
    fill.quantity = second_child_qty;
    manager.process_fill(fill);

    const auto parent_final = manager.get_order(parent_id);
    ASSERT_TRUE(parent_final.has_value());
    EXPECT_GT(parent_final->filled_quantity, 0.0);
}

TEST(SmartOrderRouterTest, SequentialSplitAggregatesParent) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;
    cfg.split_mode = SplitMode::Sequential;
    cfg.parent_aggregation = ParentAggregation::Partial;
    cfg.min_child_qty = 1.0;
    RoutingVenue lit_primary;
    lit_primary.name = "lit_primary";
    lit_primary.weight = 0.5;
    RoutingVenue lit_mid;
    lit_mid.name = "lit_mid";
    lit_mid.weight = 0.5;
    cfg.venues = {lit_primary, lit_mid};

    regimeflow::engine::OrderManager manager;
    manager.set_router(std::make_unique<SmartOrderRouter>(cfg),
                       [](const Order&) { return RoutingContext{}; });

    Order order = Order::market(SymbolRegistry::instance().intern("FFF"),
                                OrderSide::Buy,
                                10.0);
    auto parent_res = manager.submit_order(order);
    ASSERT_TRUE(parent_res.is_ok());
    const auto parent_id = parent_res.value();

    manager.activate_routed_order(parent_id);

    auto open_orders = manager.get_open_orders();
    OrderId first_child_id = 0;
    double first_child_qty = 0.0;
    for (const auto& open : open_orders) {
        if (open.parent_id == parent_id) {
            first_child_id = open.id;
            first_child_qty = open.quantity;
            break;
        }
    }
    ASSERT_NE(first_child_id, 0u);

    regimeflow::engine::Fill fill;
    fill.order_id = first_child_id;
    fill.symbol = order.symbol;
    fill.quantity = first_child_qty;
    fill.price = 100.0;
    fill.timestamp = regimeflow::Timestamp::now();
    manager.process_fill(fill);

    const auto parent_after_first = manager.get_order(parent_id);
    ASSERT_TRUE(parent_after_first.has_value());
    EXPECT_GT(parent_after_first->filled_quantity, 0.0);

    open_orders = manager.get_open_orders();
    OrderId second_child_id = 0;
    double second_child_qty = 0.0;
    for (const auto& open : open_orders) {
        if (open.parent_id == parent_id) {
            second_child_id = open.id;
            second_child_qty = open.quantity;
            break;
        }
    }
    ASSERT_NE(second_child_id, 0u);

    fill.order_id = second_child_id;
    fill.quantity = second_child_qty;
    manager.process_fill(fill);

    const auto parent_final = manager.get_order(parent_id);
    ASSERT_TRUE(parent_final.has_value());
    EXPECT_EQ(parent_final->status, regimeflow::engine::OrderStatus::Filled);
}

TEST(SmartOrderRouterTest, SplitChildrenCarryVenueMicrostructureProfile) {
    RoutingConfig cfg;
    cfg.enabled = true;
    cfg.mode = RoutingMode::Smart;
    cfg.split_mode = SplitMode::Parallel;
    cfg.min_child_qty = 1.0;
    RoutingVenue lit_primary;
    lit_primary.name = "lit_primary";
    lit_primary.weight = 0.6;
    lit_primary.maker_rebate_bps = 1.0;
    lit_primary.taker_fee_bps = 2.0;
    lit_primary.price_adjustment_bps = 3.0;
    lit_primary.latency_ms = 7;
    lit_primary.queue_enabled = true;
    lit_primary.queue_progress_fraction = 0.5;
    lit_primary.queue_default_visible_qty = 10.0;
    lit_primary.queue_depth_mode = "full_depth";
    RoutingVenue lit_mid;
    lit_mid.name = "lit_mid";
    lit_mid.weight = 0.4;
    cfg.venues = {lit_primary, lit_mid};

    SmartOrderRouter router(cfg);
    Order order = Order::market(SymbolRegistry::instance().intern("VENUE"),
                                OrderSide::Buy,
                                10.0);

    const auto plan = router.route(order, RoutingContext{});
    ASSERT_EQ(plan.children.size(), 2u);
    EXPECT_EQ(plan.children.front().metadata.at("venue"), "lit_primary");
    EXPECT_EQ(plan.children.front().metadata.at("venue_maker_rebate_bps"), "1.000000");
    EXPECT_EQ(plan.children.front().metadata.at("venue_taker_fee_bps"), "2.000000");
    EXPECT_EQ(plan.children.front().metadata.at("venue_price_adjustment_bps"), "3.000000");
    EXPECT_EQ(plan.children.front().metadata.at("venue_latency_ms"), "7");
    EXPECT_EQ(plan.children.front().metadata.at("venue_queue_enabled"), "true");
    EXPECT_EQ(plan.children.front().metadata.at("venue_queue_depth_mode"), "full_depth");
}
