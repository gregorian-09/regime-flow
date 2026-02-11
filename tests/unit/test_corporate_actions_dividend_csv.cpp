#include <gtest/gtest.h>

#include "regimeflow/data/csv_reader.h"

#include <filesystem>
#include <fstream>

namespace {

TEST(CsvDataSourceTest, ParsesDividendCorporateActions) {
    regimeflow::data::CSVDataSource::Config cfg;
    cfg.data_directory = (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string();
    cfg.file_pattern = "{symbol}.csv";
    cfg.actions_directory = cfg.data_directory;
    cfg.actions_file_pattern = "{symbol}_actions.csv";
    cfg.has_header = true;
    EXPECT_EQ(cfg.delimiter, ',');
    EXPECT_EQ(cfg.actions_file_pattern, "{symbol}_actions.csv");

    regimeflow::data::CSVDataSource source(cfg);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("TEST");
    EXPECT_EQ(regimeflow::SymbolRegistry::instance().lookup(symbol), "TEST");

    auto expected_path = std::filesystem::path(cfg.actions_directory) / "TEST_actions.csv";
    EXPECT_TRUE(std::filesystem::exists(expected_path));
    std::ifstream probe(expected_path);
    EXPECT_TRUE(probe.good());

    regimeflow::TimeRange range;
    range.start = regimeflow::Timestamp::from_string("2020-01-01 00:00:00",
                                                     "%Y-%m-%d %H:%M:%S");
    range.end = regimeflow::Timestamp::from_string("2020-01-03 00:00:00",
                                                   "%Y-%m-%d %H:%M:%S");

    auto actions = source.get_corporate_actions(symbol, range);
    ASSERT_FALSE(actions.empty());

    bool has_dividend = false;
    for (const auto& action : actions) {
        if (action.type == regimeflow::data::CorporateActionType::Dividend) {
            EXPECT_DOUBLE_EQ(action.amount, 2.0);
            has_dividend = true;
        }
    }
    EXPECT_TRUE(has_dividend);
}

}  // namespace
