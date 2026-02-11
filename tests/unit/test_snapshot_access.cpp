#include <gtest/gtest.h>

#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/snapshot_access.h"
#include "regimeflow/data/time_series_query.h"

namespace regimeflow::test {

TEST(SnapshotAccess, ReturnsLatestOrderBookAtTimestamp) {
    auto source = std::make_shared<regimeflow::data::MemoryDataSource>();
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    regimeflow::data::OrderBook b1;
    b1.symbol = symbol;
    b1.timestamp = regimeflow::Timestamp(100);
    b1.bids[0].price = 10.0;
    b1.asks[0].price = 10.1;

    regimeflow::data::OrderBook b2 = b1;
    b2.timestamp = regimeflow::Timestamp(200);
    b2.bids[0].price = 11.0;
    b2.asks[0].price = 11.1;

    source->add_order_books(symbol, {b1, b2});

    regimeflow::data::SnapshotAccess snapshot(source);
    auto at_150 = snapshot.order_book_at(symbol, regimeflow::Timestamp(150));
    ASSERT_TRUE(at_150);
    EXPECT_EQ(at_150->timestamp.microseconds(), 100);

    auto at_250 = snapshot.order_book_at(symbol, regimeflow::Timestamp(250));
    ASSERT_TRUE(at_250);
    EXPECT_EQ(at_250->timestamp.microseconds(), 200);
}

TEST(TimeSeriesQuery, ReturnsOrderBooksInRange) {
    auto source = std::make_shared<regimeflow::data::MemoryDataSource>();
    auto symbol = regimeflow::SymbolRegistry::instance().intern("BBB");

    regimeflow::data::OrderBook b1;
    b1.symbol = symbol;
    b1.timestamp = regimeflow::Timestamp(100);
    b1.bids[0].price = 20.0;
    b1.asks[0].price = 20.1;

    regimeflow::data::OrderBook b2 = b1;
    b2.timestamp = regimeflow::Timestamp(300);
    b2.bids[0].price = 21.0;
    b2.asks[0].price = 21.1;

    source->add_order_books(symbol, {b1, b2});

    regimeflow::data::TimeSeriesQuery query(source);
    regimeflow::TimeRange range{regimeflow::Timestamp(0), regimeflow::Timestamp(250)};
    auto books = query.order_books(symbol, range);

    ASSERT_EQ(books.size(), 1u);
    EXPECT_EQ(books[0].timestamp.microseconds(), 100);
}

}  // namespace regimeflow::test
