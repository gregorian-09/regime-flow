#include <gtest/gtest.h>

#include "regimeflow/data/corporate_actions.h"

namespace {

TEST(CorporateActionAdjusterTest, AdjustsForSplit) {
    regimeflow::data::CorporateActionAdjuster adjuster;
    auto symbol = regimeflow::SymbolRegistry::instance().intern("TEST");

    regimeflow::data::CorporateAction split;
    split.type = regimeflow::data::CorporateActionType::Split;
    split.factor = 2.0;
    split.effective_date = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                             "%Y-%m-%d %H:%M:%S");
    adjuster.add_actions(symbol, {split});

    regimeflow::data::Bar bar{};
    bar.symbol = symbol;
    bar.timestamp = regimeflow::Timestamp::from_string("2020-01-01 00:00:00",
                                                       "%Y-%m-%d %H:%M:%S");
    bar.open = 100;
    bar.high = 110;
    bar.low = 90;
    bar.close = 105;
    bar.volume = 1000;

    auto adjusted = adjuster.adjust_bar(symbol, bar);
    EXPECT_DOUBLE_EQ(adjusted.close, 52.5);
    EXPECT_EQ(adjusted.volume, 2000u);
}

TEST(CorporateActionAdjusterTest, AdjustsForDividend) {
    regimeflow::data::CorporateActionAdjuster adjuster;
    auto symbol = regimeflow::SymbolRegistry::instance().intern("DIV");

    regimeflow::data::CorporateAction dividend;
    dividend.type = regimeflow::data::CorporateActionType::Dividend;
    dividend.amount = 2.0;
    dividend.effective_date = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                                "%Y-%m-%d %H:%M:%S");
    adjuster.add_actions(symbol, {dividend});

    regimeflow::data::Bar bar{};
    bar.symbol = symbol;
    bar.timestamp = regimeflow::Timestamp::from_string("2020-01-01 00:00:00",
                                                       "%Y-%m-%d %H:%M:%S");
    bar.open = 100;
    bar.high = 110;
    bar.low = 90;
    bar.close = 100;
    bar.volume = 1000;

    auto adjusted = adjuster.adjust_bar(symbol, bar);
    EXPECT_DOUBLE_EQ(adjusted.close, 98.0);
    EXPECT_DOUBLE_EQ(adjusted.open, 98.0);
    EXPECT_DOUBLE_EQ(adjusted.high, 107.8);
    EXPECT_DOUBLE_EQ(adjusted.low, 88.2);
    EXPECT_EQ(adjusted.volume, 1000u);
}

TEST(CorporateActionAdjusterTest, AppliesSymbolChangeFromEffectiveDate) {
    regimeflow::data::CorporateActionAdjuster adjuster;
    auto old_symbol = regimeflow::SymbolRegistry::instance().intern("OLD");

    regimeflow::data::CorporateAction change;
    change.type = regimeflow::data::CorporateActionType::SymbolChange;
    change.new_symbol = "NEW";
    change.effective_date = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                               "%Y-%m-%d %H:%M:%S");
    adjuster.add_actions(old_symbol, {change});

    regimeflow::data::Bar before{};
    before.symbol = old_symbol;
    before.timestamp = regimeflow::Timestamp::from_string("2020-01-01 00:00:00",
                                                          "%Y-%m-%d %H:%M:%S");
    before.open = 10;
    before.high = 11;
    before.low = 9;
    before.close = 10;
    before.volume = 100;

    auto before_adjusted = adjuster.adjust_bar(old_symbol, before);
    EXPECT_EQ(before_adjusted.symbol, old_symbol);

    regimeflow::data::Bar after = before;
    after.timestamp = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                         "%Y-%m-%d %H:%M:%S");
    auto after_adjusted = adjuster.adjust_bar(old_symbol, after);
    auto new_symbol = regimeflow::SymbolRegistry::instance().intern("NEW");
    EXPECT_EQ(after_adjusted.symbol, new_symbol);
}

}  // namespace
