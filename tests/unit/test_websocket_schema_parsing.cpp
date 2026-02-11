#include <gtest/gtest.h>

#include "regimeflow/common/types.h"
#include "regimeflow/data/websocket_feed.h"

namespace regimeflow::test {

TEST(WebSocketFeedSchemaParsing, ParsesAlpacaTrade) {
    data::WebSocketFeed::Config cfg;
    cfg.url = "ws://example.com/feed";
    cfg.connect_override = [] { return Ok(); };
    data::WebSocketFeed feed(cfg);

    data::Tick last_tick;
    int tick_count = 0;
    feed.on_tick([&](const data::Tick& tick) {
        last_tick = tick;
        ++tick_count;
    });

    const std::string msg = R"({"T":"t","S":"AAPL","p":101.5,"s":2,"t":1700000})";
    feed.handle_message(msg);

    EXPECT_EQ(tick_count, 1);
    EXPECT_EQ(last_tick.symbol, SymbolRegistry::instance().intern("AAPL"));
    EXPECT_EQ(last_tick.price, 101.5);
    EXPECT_EQ(last_tick.quantity, 2.0);
    EXPECT_EQ(last_tick.timestamp.microseconds(), 1700000);
}

TEST(WebSocketFeedSchemaParsing, ParsesAlpacaBar) {
    data::WebSocketFeed::Config cfg;
    cfg.url = "ws://example.com/feed";
    cfg.connect_override = [] { return Ok(); };
    data::WebSocketFeed feed(cfg);

    data::Bar last_bar;
    int bar_count = 0;
    feed.on_bar([&](const data::Bar& bar) {
        last_bar = bar;
        ++bar_count;
    });

    const std::string msg = R"({"T":"b","S":"AAPL","o":10,"h":11,"l":9,"c":10.5,"v":100,"t":1700001})";
    feed.handle_message(msg);

    EXPECT_EQ(bar_count, 1);
    EXPECT_EQ(last_bar.symbol, SymbolRegistry::instance().intern("AAPL"));
    EXPECT_EQ(last_bar.open, 10.0);
    EXPECT_EQ(last_bar.high, 11.0);
    EXPECT_EQ(last_bar.low, 9.0);
    EXPECT_EQ(last_bar.close, 10.5);
    EXPECT_EQ(last_bar.volume, 100u);
    EXPECT_EQ(last_bar.timestamp.microseconds(), 1700001);
}

TEST(WebSocketFeedSchemaParsing, ParsesIexTradeAndBook) {
    data::WebSocketFeed::Config cfg;
    cfg.url = "ws://example.com/feed";
    cfg.connect_override = [] { return Ok(); };
    data::WebSocketFeed feed(cfg);

    data::Tick last_tick;
    int tick_count = 0;
    feed.on_tick([&](const data::Tick& tick) {
        last_tick = tick;
        ++tick_count;
    });

    data::OrderBook last_book;
    int book_count = 0;
    feed.on_book([&](const data::OrderBook& book) {
        last_book = book;
        ++book_count;
    });

    const std::string trade = R"({"type":"trade","symbol":"AAPL","price":101.7,"size":3,"ts":1700002})";
    const std::string book = R"({"type":"book","symbol":"AAPL","bids":[[101.6,5,2]],"asks":[[101.8,4,1]],"ts":1700003})";
    feed.handle_message(trade);
    feed.handle_message(book);

    EXPECT_EQ(tick_count, 1);
    EXPECT_EQ(last_tick.symbol, SymbolRegistry::instance().intern("AAPL"));
    EXPECT_EQ(last_tick.price, 101.7);
    EXPECT_EQ(last_tick.quantity, 3.0);
    EXPECT_EQ(last_tick.timestamp.microseconds(), 1700002);

    EXPECT_EQ(book_count, 1);
    EXPECT_EQ(last_book.symbol, SymbolRegistry::instance().intern("AAPL"));
    EXPECT_EQ(last_book.timestamp.microseconds(), 1700003);
    EXPECT_EQ(last_book.bids[0].price, 101.6);
    EXPECT_EQ(last_book.bids[0].quantity, 5.0);
    EXPECT_EQ(last_book.bids[0].num_orders, 2);
    EXPECT_EQ(last_book.asks[0].price, 101.8);
    EXPECT_EQ(last_book.asks[0].quantity, 4.0);
    EXPECT_EQ(last_book.asks[0].num_orders, 1);
}

}  // namespace regimeflow::test
