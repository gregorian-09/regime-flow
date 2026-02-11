#include <gtest/gtest.h>

#include "regimeflow/data/csv_reader.h"

#include <filesystem>

namespace {

TEST(CsvDataSourceTest, AppliesCorporateActions) {
    regimeflow::data::CSVDataSource::Config cfg;
    cfg.data_directory = (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string();
    cfg.file_pattern = "{symbol}.csv";
    cfg.has_header = true;
    cfg.actions_directory = (std::filesystem::path(REGIMEFLOW_TEST_ROOT)
                             / "tests/fixtures/no_actions").string();

    regimeflow::data::CSVDataSource source(cfg);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("TEST");

    regimeflow::data::CorporateAction split;
    split.type = regimeflow::data::CorporateActionType::Split;
    split.factor = 2.0;
    split.effective_date = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                             "%Y-%m-%d %H:%M:%S");
    source.set_corporate_actions(symbol, {split});

    regimeflow::TimeRange range;
    range.start = regimeflow::Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    range.end = regimeflow::Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");

    auto bars = source.get_bars(symbol, range, regimeflow::data::BarType::Time_1Day);
    ASSERT_EQ(bars.size(), 3u);
    EXPECT_DOUBLE_EQ(bars.front().close, 50.0);
    EXPECT_DOUBLE_EQ(bars[1].close, 101.0);
}

}  // namespace
