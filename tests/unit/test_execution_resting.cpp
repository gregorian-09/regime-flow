#include "regimeflow/engine/execution_pipeline.h"
#include "regimeflow/execution/latency_model.h"

#include <gtest/gtest.h>

using regimeflow::SymbolRegistry;
using regimeflow::Timestamp;
using regimeflow::data::Bar;
using regimeflow::data::OrderBook;
using regimeflow::data::Quote;
using regimeflow::data::Tick;
using regimeflow::engine::ExecutionPipeline;
using regimeflow::engine::MarketDataCache;
using regimeflow::engine::Order;
using regimeflow::engine::OrderBookCache;
using regimeflow::engine::OrderSide;
using regimeflow::engine::OrderType;
using regimeflow::events::EventQueue;
using regimeflow::events::OrderEventKind;

TEST(ExecutionPipelineRestingTest, LimitOrderRestsUntilQuoteCrosses) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);

    const auto symbol = SymbolRegistry::instance().intern("REST");
    auto order = Order::limit(symbol, OrderSide::Buy, 10.0, 100.0);
    order.id = 1;
    order.created_at = Timestamp::now();

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Quote away;
    away.symbol = symbol;
    away.timestamp = order.created_at + regimeflow::Duration::milliseconds(1);
    away.bid = 100.5;
    away.ask = 101.0;
    market_data.update(away);
    pipeline.on_market_update(symbol, away.timestamp);
    EXPECT_TRUE(queue.empty());

    Quote cross = away;
    cross.timestamp = away.timestamp + regimeflow::Duration::milliseconds(1);
    cross.bid = 99.8;
    cross.ask = 100.0;
    market_data.update(cross);
    pipeline.on_market_update(symbol, cross.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 1u);
}

TEST(ExecutionPipelineRestingTest, StopOrderTriggersOnLaterTick) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);

    const auto symbol = SymbolRegistry::instance().intern("STOP");
    auto order = Order::stop(symbol, OrderSide::Buy, 5.0, 101.0);
    order.id = 2;
    order.created_at = Timestamp::now();

    Tick before;
    before.symbol = symbol;
    before.timestamp = order.created_at;
    before.price = 100.0;
    market_data.update(before);

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Tick trigger = before;
    trigger.timestamp = before.timestamp + regimeflow::Duration::milliseconds(1);
    trigger.price = 101.2;
    market_data.update(trigger);
    pipeline.on_market_update(symbol, trigger.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 2u);
}

TEST(ExecutionPipelineRestingTest, OpenOnlyBarModeUsesBarOpenForLimitFills) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_bar_simulation_mode(ExecutionPipeline::BarSimulationMode::OpenOnly);

    const auto symbol = SymbolRegistry::instance().intern("BAROPEN");
    auto order = Order::limit(symbol, OrderSide::Buy, 4.0, 100.0);
    order.id = 3;
    order.created_at = Timestamp::now();

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Bar bar;
    bar.symbol = symbol;
    bar.timestamp = order.created_at + regimeflow::Duration::seconds(60);
    bar.open = 99.5;
    bar.high = 103.0;
    bar.low = 99.0;
    bar.close = 102.0;
    market_data.update(bar);
    pipeline.on_bar(bar);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 3u);
}

TEST(ExecutionPipelineRestingTest, IntrabarOhlcModeTriggersStopWithinBarRange) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_bar_simulation_mode(ExecutionPipeline::BarSimulationMode::IntrabarOhlc);

    const auto symbol = SymbolRegistry::instance().intern("BARSTOP");
    auto order = Order::stop(symbol, OrderSide::Buy, 3.0, 105.0);
    order.id = 4;
    order.created_at = Timestamp::now();

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Bar bar;
    bar.symbol = symbol;
    bar.timestamp = order.created_at + regimeflow::Duration::seconds(60);
    bar.open = 100.0;
    bar.high = 106.0;
    bar.low = 99.0;
    bar.close = 101.0;
    market_data.update(bar);
    pipeline.on_bar(bar);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 4u);
}

TEST(ExecutionPipelineRestingTest, StopOrderRejectsOnAdversePriceDrift) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_price_drift_rule(50.0, ExecutionPipeline::PriceDriftAction::Reject);

    const auto symbol = SymbolRegistry::instance().intern("DRIFT_REJECT");
    Tick before;
    before.symbol = symbol;
    before.timestamp = Timestamp::now();
    before.price = 100.0;
    market_data.update(before);

    auto order = Order::stop(symbol, OrderSide::Buy, 5.0, 101.0);
    order.id = 5;
    order.created_at = before.timestamp;

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Tick trigger = before;
    trigger.timestamp = before.timestamp + regimeflow::Duration::milliseconds(1);
    trigger.price = 103.0;
    market_data.update(trigger);
    pipeline.on_market_update(symbol, trigger.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Reject);
    EXPECT_EQ(payload->order_id, 5u);
}

TEST(ExecutionPipelineRestingTest, StopOrderRequotesBeforeFilling) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_price_drift_rule(50.0, ExecutionPipeline::PriceDriftAction::Requote);

    const auto symbol = SymbolRegistry::instance().intern("DRIFT_REQUOTE");
    Tick before;
    before.symbol = symbol;
    before.timestamp = Timestamp::now();
    before.price = 100.0;
    market_data.update(before);

    auto order = Order::stop(symbol, OrderSide::Buy, 5.0, 101.0);
    order.id = 6;
    order.created_at = before.timestamp;

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Tick trigger = before;
    trigger.timestamp = before.timestamp + regimeflow::Duration::milliseconds(1);
    trigger.price = 103.0;
    market_data.update(trigger);
    pipeline.on_market_update(symbol, trigger.timestamp);

    auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Update);
    EXPECT_EQ(payload->order_id, 6u);
    EXPECT_DOUBLE_EQ(payload->price, 103.0);

    trigger.timestamp = trigger.timestamp + regimeflow::Duration::milliseconds(1);
    market_data.update(trigger);
    pipeline.on_market_update(symbol, trigger.timestamp);

    event = queue.pop();
    ASSERT_TRUE(event.has_value());
    payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 6u);
}

TEST(ExecutionPipelineRestingTest, MarketOrderWaitsForLatencyWindow) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_latency_model(
        std::make_unique<regimeflow::execution::FixedLatencyModel>(
            regimeflow::Duration::milliseconds(5)));

    const auto symbol = SymbolRegistry::instance().intern("LATENCY");
    Quote quote;
    quote.symbol = symbol;
    quote.timestamp = Timestamp::now();
    quote.bid = 99.9;
    quote.ask = 100.1;
    market_data.update(quote);

    auto order = Order::market(symbol, OrderSide::Buy, 2.0);
    order.id = 7;
    order.created_at = quote.timestamp;

    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    auto early_quote = quote;
    early_quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    market_data.update(early_quote);
    pipeline.on_market_update(symbol, early_quote.timestamp);
    EXPECT_TRUE(queue.empty());

    auto live_quote = quote;
    live_quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(6);
    market_data.update(live_quote);
    pipeline.on_market_update(symbol, live_quote.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 7u);
}

TEST(ExecutionPipelineRestingTest, RestingLimitAdvancesQueueBeforeMakerFill) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_queue_model(true, 0.5, 4.0);

    const auto symbol = SymbolRegistry::instance().intern("QUEUE_MAKER");
    Quote away;
    away.symbol = symbol;
    away.timestamp = Timestamp::now();
    away.bid = 100.0;
    away.ask = 101.0;
    away.bid_size = 4.0;
    away.ask_size = 4.0;
    market_data.update(away);

    auto order = Order::limit(symbol, OrderSide::Buy, 2.0, 100.0);
    order.id = 8;
    order.created_at = away.timestamp;
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Quote touch = away;
    touch.timestamp = away.timestamp + regimeflow::Duration::milliseconds(1);
    touch.ask = 100.0;
    touch.ask_size = 2.0;
    market_data.update(touch);
    pipeline.on_market_update(symbol, touch.timestamp);
    EXPECT_TRUE(queue.empty());

    touch.timestamp = touch.timestamp + regimeflow::Duration::milliseconds(1);
    market_data.update(touch);
    pipeline.on_market_update(symbol, touch.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 8u);
    EXPECT_TRUE(payload->is_maker);
}

TEST(ExecutionPipelineRestingTest, CrossThroughLimitFillsAsTaker) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_queue_model(true, 0.25, 4.0);

    const auto symbol = SymbolRegistry::instance().intern("QUEUE_TAKER");
    Quote away;
    away.symbol = symbol;
    away.timestamp = Timestamp::now();
    away.bid = 100.0;
    away.ask = 101.0;
    away.bid_size = 8.0;
    away.ask_size = 8.0;
    market_data.update(away);

    auto order = Order::limit(symbol, OrderSide::Buy, 2.0, 100.0);
    order.id = 9;
    order.created_at = away.timestamp;
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    Quote through = away;
    through.timestamp = away.timestamp + regimeflow::Duration::milliseconds(1);
    through.ask = 99.5;
    through.ask_size = 2.0;
    market_data.update(through);
    pipeline.on_market_update(symbol, through.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 9u);
    EXPECT_FALSE(payload->is_maker);
}

TEST(ExecutionPipelineRestingTest, FullDepthQueueModelHandlesNonTopLevelOrders) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_queue_model(true,
                             0.5,
                             1.0,
                             ExecutionPipeline::QueueDepthMode::FullDepth);

    const auto symbol = SymbolRegistry::instance().intern("QUEUE_DEPTH");
    auto order = Order::limit(symbol, OrderSide::Buy, 1.0, 99.0);
    order.id = 10;
    order.created_at = Timestamp::now();
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    OrderBook book;
    book.symbol = symbol;
    book.timestamp = order.created_at + regimeflow::Duration::milliseconds(1);
    book.bids[0].price = 100.0;
    book.bids[0].quantity = 2.0;
    book.bids[1].price = 99.0;
    book.bids[1].quantity = 3.0;
    book.asks[0].price = 99.0;
    book.asks[0].quantity = 1.0;
    order_books.update(book);
    pipeline.on_market_update(symbol, book.timestamp);
    EXPECT_TRUE(queue.empty());

    book.timestamp = book.timestamp + regimeflow::Duration::milliseconds(1);
    order_books.update(book);
    pipeline.on_market_update(symbol, book.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 10u);
    EXPECT_TRUE(payload->is_maker);
}

TEST(ExecutionPipelineRestingTest, SessionGateDefersMarketFillUntilOpen) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);

    ExecutionPipeline::SessionPolicy session;
    session.enabled = true;
    session.start_minute_utc = 9 * 60 + 30;
    session.end_minute_utc = 16 * 60;
    pipeline.set_session_policy(session);

    const auto symbol = SymbolRegistry::instance().intern("SESSION");
    Quote quote;
    quote.symbol = symbol;
    quote.bid = 99.0;
    quote.ask = 100.0;
    quote.timestamp = Timestamp::from_string("2020-01-01 08:00:00", "%Y-%m-%d %H:%M:%S");
    market_data.update(quote);

    auto order = Order::market(symbol, OrderSide::Buy, 1.0);
    order.id = 11;
    order.created_at = quote.timestamp;
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = Timestamp::from_string("2020-01-01 09:29:00", "%Y-%m-%d %H:%M:%S");
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = Timestamp::from_string("2020-01-01 09:30:00", "%Y-%m-%d %H:%M:%S");
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 11u);
}

TEST(ExecutionPipelineRestingTest, QueueAgingReducesQueueAheadAcrossAwayEvents) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_queue_model(true, 0.25, 4.0, ExecutionPipeline::QueueDepthMode::TopOnly, 0.5, 0.0);

    const auto symbol = SymbolRegistry::instance().intern("QUEUE_AGE");
    Quote quote;
    quote.symbol = symbol;
    quote.timestamp = Timestamp::now();
    quote.bid = 100.0;
    quote.ask = 101.0;
    quote.bid_size = 4.0;
    quote.ask_size = 4.0;
    market_data.update(quote);

    auto order = Order::limit(symbol, OrderSide::Buy, 1.0, 100.0);
    order.id = 12;
    order.created_at = quote.timestamp;
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    quote.ask = 100.0;
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    quote.ask = 101.0;
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    quote.ask = 100.0;
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_TRUE(payload->is_maker);
}

TEST(ExecutionPipelineRestingTest, QueueReplenishmentDelaysMakerFillWhenVisibleSizeBuilds) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_queue_model(true, 0.5, 2.0, ExecutionPipeline::QueueDepthMode::TopOnly, 0.0, 1.0);

    const auto symbol = SymbolRegistry::instance().intern("QUEUE_REPLENISH");
    Quote quote;
    quote.symbol = symbol;
    quote.timestamp = Timestamp::now();
    quote.bid = 100.0;
    quote.ask = 101.0;
    quote.bid_size = 2.0;
    quote.ask_size = 2.0;
    market_data.update(quote);

    auto order = Order::limit(symbol, OrderSide::Buy, 1.0, 100.0);
    order.id = 13;
    order.created_at = quote.timestamp;
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    quote.ask = 100.0;
    quote.bid_size = 2.0;
    quote.ask_size = 2.0;
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    quote.bid_size = 4.0;
    quote.ask_size = 4.0;
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(1);
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_TRUE(payload->is_maker);
}

TEST(ExecutionPipelineRestingTest, VenuePriceAdjustmentCanPreventTouchFill) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);

    const auto symbol = SymbolRegistry::instance().intern("VENUE_PRICE");
    Quote quote;
    quote.symbol = symbol;
    quote.timestamp = Timestamp::now();
    quote.bid = 99.5;
    quote.ask = 100.0;
    market_data.update(quote);

    auto adjusted_order = Order::limit(symbol, OrderSide::Buy, 1.0, 100.0);
    adjusted_order.id = 14;
    adjusted_order.created_at = quote.timestamp;
    adjusted_order.metadata["venue_price_adjustment_bps"] = "20";
    pipeline.on_order_submitted(adjusted_order);
    EXPECT_TRUE(queue.empty());

    auto plain_order = Order::limit(symbol, OrderSide::Buy, 1.0, 100.0);
    plain_order.id = 15;
    plain_order.created_at = quote.timestamp;
    pipeline.on_order_submitted(plain_order);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 15u);
    EXPECT_TRUE(queue.empty());
}

TEST(ExecutionPipelineRestingTest, VenueLatencyOverrideDelaysActivationBeyondGlobalLatency) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_latency_model(
        std::make_unique<regimeflow::execution::FixedLatencyModel>(
            regimeflow::Duration::milliseconds(1)));

    const auto symbol = SymbolRegistry::instance().intern("VENUE_LATENCY");
    Quote quote;
    quote.symbol = symbol;
    quote.timestamp = Timestamp::now();
    quote.bid = 99.5;
    quote.ask = 100.0;
    market_data.update(quote);

    auto order = Order::market(symbol, OrderSide::Buy, 1.0);
    order.id = 16;
    order.created_at = quote.timestamp;
    order.metadata["venue_latency_ms"] = "5";
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    auto early_quote = quote;
    early_quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(2);
    market_data.update(early_quote);
    pipeline.on_market_update(symbol, early_quote.timestamp);
    EXPECT_TRUE(queue.empty());

    auto live_quote = quote;
    live_quote.timestamp = quote.timestamp + regimeflow::Duration::milliseconds(6);
    market_data.update(live_quote);
    pipeline.on_market_update(symbol, live_quote.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 16u);
}

TEST(ExecutionPipelineRestingTest, ClosedDateBlocksExecution) {
    MarketDataCache market_data;
    OrderBookCache order_books;
    EventQueue queue;
    ExecutionPipeline pipeline(&market_data, &order_books, &queue);

    ExecutionPipeline::SessionPolicy session;
    session.enabled = true;
    session.allowed_weekdays = {1, 2, 3, 4, 5};
    session.closed_dates.emplace("2020-01-01");
    pipeline.set_session_policy(session);

    const auto symbol = SymbolRegistry::instance().intern("HOLIDAY");
    Quote quote;
    quote.symbol = symbol;
    quote.bid = 99.0;
    quote.ask = 100.0;
    quote.timestamp = Timestamp::from_string("2020-01-01 10:00:00", "%Y-%m-%d %H:%M:%S");
    market_data.update(quote);

    auto order = Order::market(symbol, OrderSide::Buy, 1.0);
    order.id = 17;
    order.created_at = quote.timestamp;
    pipeline.on_order_submitted(order);
    EXPECT_TRUE(queue.empty());

    quote.timestamp = Timestamp::from_string("2020-01-02 10:00:00", "%Y-%m-%d %H:%M:%S");
    market_data.update(quote);
    pipeline.on_market_update(symbol, quote.timestamp);

    const auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->kind, OrderEventKind::Fill);
    EXPECT_EQ(payload->order_id, 17u);
}
