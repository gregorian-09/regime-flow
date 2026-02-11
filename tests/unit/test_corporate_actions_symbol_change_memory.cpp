#include <gtest/gtest.h>

#include "regimeflow/data/memory_data_source.h"

namespace {

regimeflow::data::Bar make_bar(regimeflow::SymbolId symbol,
                               const std::string& ts,
                               double close) {
    regimeflow::data::Bar bar{};
    bar.symbol = symbol;
    bar.timestamp = regimeflow::Timestamp::from_string(ts, "%Y-%m-%d %H:%M:%S");
    bar.open = close;
    bar.high = close;
    bar.low = close;
    bar.close = close;
    bar.volume = 1;
    return bar;
}

}  // namespace

TEST(MemoryDataSourceTest, SymbolChangeAliasesResolve) {
    regimeflow::data::MemoryDataSource source;
    auto old_symbol = regimeflow::SymbolRegistry::instance().intern("OLD");

    source.add_bars(old_symbol, {make_bar(old_symbol, "2020-01-01 00:00:00", 10.0)});
    regimeflow::data::SymbolInfo info;
    info.id = old_symbol;
    info.ticker = "OLD";
    source.add_symbol_info(info);

    regimeflow::data::CorporateAction change;
    change.type = regimeflow::data::CorporateActionType::SymbolChange;
    change.new_symbol = "NEW";
    change.effective_date = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                               "%Y-%m-%d %H:%M:%S");
    source.set_corporate_actions(old_symbol, {change});

    auto symbols = source.get_available_symbols();
    bool has_new = false;
    for (const auto& entry : symbols) {
        if (entry.ticker == "NEW") {
            has_new = true;
            break;
        }
    }
    EXPECT_TRUE(has_new);

    auto new_symbol = regimeflow::SymbolRegistry::instance().intern("NEW");
    auto range = source.get_available_range(new_symbol);
    EXPECT_GT(range.start.microseconds(), 0);

    auto bars = source.get_bars(new_symbol, range);
    ASSERT_EQ(bars.size(), 1u);
    EXPECT_EQ(bars.front().close, 10.0);
}
