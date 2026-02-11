#include <gtest/gtest.h>

#include "regimeflow/data/merged_iterator.h"
#include "regimeflow/data/memory_data_source.h"

using namespace regimeflow;
using namespace regimeflow::data;

namespace {

Bar make_bar(SymbolId symbol, const std::string& ts, double price) {
    Bar bar;
    bar.symbol = symbol;
    bar.timestamp = Timestamp::from_string(ts, "%Y-%m-%d %H:%M:%S");
    bar.open = price;
    bar.high = price;
    bar.low = price;
    bar.close = price;
    bar.volume = 1;
    return bar;
}  // namespace

}

TEST(MergedIterator, OrdersByTimestampThenSymbol) {
    SymbolId sym_a = SymbolRegistry::instance().intern("AAA");
    SymbolId sym_b = SymbolRegistry::instance().intern("BBB");

    std::vector<Bar> bars_a{
        make_bar(sym_a, "2024-01-01 00:00:00", 10.0),
        make_bar(sym_a, "2024-01-01 00:02:00", 12.0)
    };
    std::vector<Bar> bars_b{
        make_bar(sym_b, "2024-01-01 00:01:00", 20.0),
        make_bar(sym_b, "2024-01-01 00:02:00", 21.0)
    };

    std::vector<std::unique_ptr<DataIterator>> iterators;
    iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars_a)));
    iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars_b)));

    MergedBarIterator merged(std::move(iterators));

    ASSERT_TRUE(merged.has_next());
    auto first = merged.next();
    EXPECT_EQ(first.timestamp.to_string(), "2024-01-01 00:00:00");
    EXPECT_EQ(first.symbol, sym_a);

    auto second = merged.next();
    EXPECT_EQ(second.timestamp.to_string(), "2024-01-01 00:01:00");
    EXPECT_EQ(second.symbol, sym_b);

    auto third = merged.next();
    EXPECT_EQ(third.timestamp.to_string(), "2024-01-01 00:02:00");
    EXPECT_EQ(third.symbol, sym_a);

    auto fourth = merged.next();
    EXPECT_EQ(fourth.timestamp.to_string(), "2024-01-01 00:02:00");
    EXPECT_EQ(fourth.symbol, sym_b);

    EXPECT_FALSE(merged.has_next());

    merged.reset();
    EXPECT_TRUE(merged.has_next());
    auto reset_first = merged.next();
    EXPECT_EQ(reset_first.timestamp.to_string(), "2024-01-01 00:00:00");
    EXPECT_EQ(reset_first.symbol, sym_a);
}
