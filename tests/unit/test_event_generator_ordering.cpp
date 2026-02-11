#include <gtest/gtest.h>

#include "regimeflow/common/types.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/engine/event_generator.h"
#include "regimeflow/events/event_queue.h"

namespace regimeflow::test {

TEST(EventGeneratorOrdering, TickIteratorOrdersByTimestampThenSymbol) {
    data::MemoryDataSource source;
    auto sym_a = SymbolRegistry::instance().intern("AAA");
    auto sym_b = SymbolRegistry::instance().intern("BBB");

    data::Tick t1;
    t1.symbol = sym_b;
    t1.timestamp = Timestamp(100);
    t1.price = 10.0;
    t1.quantity = 1.0;

    data::Tick t2;
    t2.symbol = sym_a;
    t2.timestamp = Timestamp(200);
    t2.price = 10.5;
    t2.quantity = 2.0;

    data::Tick t3;
    t3.symbol = sym_a;
    t3.timestamp = Timestamp(100);
    t3.price = 9.5;
    t3.quantity = 1.5;

    source.add_ticks(sym_a, {t2, t3});
    source.add_ticks(sym_b, {t1});

    TimeRange range{Timestamp(0), Timestamp(1000)};
    auto iterator = source.create_tick_iterator({sym_b, sym_a}, range);
    ASSERT_TRUE(iterator);

    std::vector<data::Tick> out;
    while (iterator->has_next()) {
        out.push_back(iterator->next());
    }

    ASSERT_EQ(out.size(), 3u);
    auto first_symbol = std::min(sym_a, sym_b);
    auto second_symbol = std::max(sym_a, sym_b);
    EXPECT_EQ(out[0].timestamp.microseconds(), 100);
    EXPECT_EQ(out[0].symbol, first_symbol);
    EXPECT_EQ(out[1].timestamp.microseconds(), 100);
    EXPECT_EQ(out[1].symbol, second_symbol);
    EXPECT_EQ(out[2].timestamp.microseconds(), 200);
    EXPECT_EQ(out[2].symbol, sym_a);
}

TEST(EventGeneratorOrdering, BookIteratorOrdersByTimestampThenSymbol) {
    data::MemoryDataSource source;
    auto sym_a = SymbolRegistry::instance().intern("AAA");
    auto sym_b = SymbolRegistry::instance().intern("BBB");

    data::OrderBook b1;
    b1.symbol = sym_b;
    b1.timestamp = Timestamp(100);
    b1.bids[0].price = 10.0;
    b1.asks[0].price = 10.1;

    data::OrderBook b2;
    b2.symbol = sym_a;
    b2.timestamp = Timestamp(200);
    b2.bids[0].price = 9.9;
    b2.asks[0].price = 10.0;

    data::OrderBook b3;
    b3.symbol = sym_a;
    b3.timestamp = Timestamp(100);
    b3.bids[0].price = 9.8;
    b3.asks[0].price = 9.9;

    source.add_order_books(sym_a, {b2, b3});
    source.add_order_books(sym_b, {b1});

    TimeRange range{Timestamp(0), Timestamp(1000)};
    auto iterator = source.create_book_iterator({sym_b, sym_a}, range);
    ASSERT_TRUE(iterator);

    std::vector<data::OrderBook> out;
    while (iterator->has_next()) {
        out.push_back(iterator->next());
    }

    ASSERT_EQ(out.size(), 3u);
    auto first_symbol = std::min(sym_a, sym_b);
    auto second_symbol = std::max(sym_a, sym_b);
    EXPECT_EQ(out[0].timestamp.microseconds(), 100);
    EXPECT_EQ(out[0].symbol, first_symbol);
    EXPECT_EQ(out[1].timestamp.microseconds(), 100);
    EXPECT_EQ(out[1].symbol, second_symbol);
    EXPECT_EQ(out[2].timestamp.microseconds(), 200);
    EXPECT_EQ(out[2].symbol, sym_a);
}

TEST(EventGeneratorOrdering, MarketEventsOrderedBySymbolAndKind) {
    data::MemoryDataSource source;
    auto sym_a = SymbolRegistry::instance().intern("AAA");
    auto sym_b = SymbolRegistry::instance().intern("BBB");

    data::Bar bar_a;
    bar_a.symbol = sym_a;
    bar_a.timestamp = Timestamp(100);
    bar_a.open = 10.0;
    bar_a.high = 10.5;
    bar_a.low = 9.8;
    bar_a.close = 10.2;
    bar_a.volume = 100;

    data::Bar bar_b = bar_a;
    bar_b.symbol = sym_b;

    data::Tick tick_a;
    tick_a.symbol = sym_a;
    tick_a.timestamp = Timestamp(100);
    tick_a.price = 10.1;
    tick_a.quantity = 5.0;

    data::OrderBook book_a;
    book_a.symbol = sym_a;
    book_a.timestamp = Timestamp(100);
    book_a.bids[0].price = 10.0;
    book_a.asks[0].price = 10.2;

    source.add_bars(sym_a, {bar_a});
    source.add_bars(sym_b, {bar_b});
    source.add_ticks(sym_a, {tick_a});
    source.add_order_books(sym_a, {book_a});

    TimeRange range{Timestamp(0), Timestamp(1000)};
    auto bar_it = source.create_iterator({sym_b, sym_a}, range, data::BarType::Time_1Day);
    auto tick_it = source.create_tick_iterator({sym_b, sym_a}, range);
    auto book_it = source.create_book_iterator({sym_b, sym_a}, range);

    events::EventQueue queue;
    engine::EventGenerator::Config cfg;
    cfg.emit_start_of_day = false;
    cfg.emit_end_of_day = false;
    cfg.emit_regime_check = false;
    engine::EventGenerator generator(std::move(bar_it), std::move(tick_it), std::move(book_it),
                                     &queue, cfg);
    generator.enqueue_all();

    std::vector<events::MarketEventKind> kinds;
    std::vector<SymbolId> symbols;
    while (auto evt = queue.pop()) {
        auto payload = std::get_if<events::MarketEventPayload>(&evt->payload);
        ASSERT_TRUE(payload);
        kinds.push_back(payload->kind);
        symbols.push_back(evt->symbol);
    }

    ASSERT_EQ(kinds.size(), 4u);
    auto first_symbol = std::min(sym_a, sym_b);
    if (first_symbol == sym_a) {
        EXPECT_EQ(symbols[0], sym_a);
        EXPECT_EQ(kinds[0], events::MarketEventKind::Bar);
        EXPECT_EQ(symbols[1], sym_a);
        EXPECT_EQ(kinds[1], events::MarketEventKind::Tick);
        EXPECT_EQ(symbols[2], sym_a);
        EXPECT_EQ(kinds[2], events::MarketEventKind::Book);
        EXPECT_EQ(symbols[3], sym_b);
        EXPECT_EQ(kinds[3], events::MarketEventKind::Bar);
    } else {
        EXPECT_EQ(symbols[0], sym_b);
        EXPECT_EQ(kinds[0], events::MarketEventKind::Bar);
        EXPECT_EQ(symbols[1], sym_a);
        EXPECT_EQ(kinds[1], events::MarketEventKind::Bar);
        EXPECT_EQ(symbols[2], sym_a);
        EXPECT_EQ(kinds[2], events::MarketEventKind::Tick);
        EXPECT_EQ(symbols[3], sym_a);
        EXPECT_EQ(kinds[3], events::MarketEventKind::Book);
    }
}

}  // namespace regimeflow::test
