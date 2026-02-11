#include <gtest/gtest.h>

#include "regimeflow/events/event_queue.h"
#include "regimeflow/events/event.h"
#include "regimeflow/data/bar.h"

namespace regimeflow::test {

TEST(EventQueueOrdering, OrdersByTimestampThenPriority) {
    events::EventQueue queue;
    Timestamp ts(1000);

    auto system_evt = events::make_system_event(events::SystemEventKind::Timer, ts);

    data::Bar bar;
    bar.timestamp = ts;
    bar.symbol = SymbolRegistry::instance().intern("AAA");
    bar.open = 1.0;
    bar.high = 1.0;
    bar.low = 1.0;
    bar.close = 1.0;
    bar.volume = 1;
    auto market_evt = events::make_market_event(bar);

    auto order_evt = events::make_order_event(events::OrderEventKind::NewOrder, ts, 1);

    queue.push(order_evt);
    queue.push(market_evt);
    queue.push(system_evt);

    auto first = queue.pop();
    ASSERT_TRUE(first);
    EXPECT_EQ(first->type, events::EventType::System);

    auto second = queue.pop();
    ASSERT_TRUE(second);
    EXPECT_EQ(second->type, events::EventType::Market);

    auto third = queue.pop();
    ASSERT_TRUE(third);
    EXPECT_EQ(third->type, events::EventType::Order);
}

TEST(EventQueueOrdering, FIFOForSameTimestampAndPriority) {
    events::EventQueue queue;
    Timestamp ts(2000);

    data::Bar bar_a;
    bar_a.timestamp = ts;
    bar_a.symbol = SymbolRegistry::instance().intern("AAA");
    bar_a.open = 1.0;
    bar_a.high = 1.0;
    bar_a.low = 1.0;
    bar_a.close = 1.0;
    bar_a.volume = 1;

    data::Bar bar_b = bar_a;
    bar_b.symbol = SymbolRegistry::instance().intern("BBB");

    auto evt_a = events::make_market_event(bar_a);
    auto evt_b = events::make_market_event(bar_b);

    queue.push(evt_a);
    queue.push(evt_b);

    auto first = queue.pop();
    ASSERT_TRUE(first);
    auto second = queue.pop();
    ASSERT_TRUE(second);

    EXPECT_EQ(first->symbol, bar_a.symbol);
    EXPECT_EQ(second->symbol, bar_b.symbol);
}

}  // namespace regimeflow::test
