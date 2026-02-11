#include <gtest/gtest.h>

#include "regimeflow/data/csv_reader.h"

#include <filesystem>

namespace {

TEST(CsvDataSourceTest, SymbolChangeAliasesAvailableSymbolsAndRange) {
    regimeflow::data::CSVDataSource::Config cfg;
    cfg.data_directory = (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string();
    cfg.file_pattern = "{symbol}.csv";
    cfg.has_header = true;

    regimeflow::data::CSVDataSource source(cfg);
    auto old_symbol = regimeflow::SymbolRegistry::instance().intern("TEST");

    regimeflow::data::CorporateAction change;
    change.type = regimeflow::data::CorporateActionType::SymbolChange;
    change.new_symbol = "TEST2";
    change.effective_date = regimeflow::Timestamp::from_string("2020-01-02 00:00:00",
                                                               "%Y-%m-%d %H:%M:%S");
    source.set_corporate_actions(old_symbol, {change});

    auto symbols = source.get_available_symbols();
    bool has_new = false;
    for (const auto& info : symbols) {
        if (info.ticker == "TEST2") {
            has_new = true;
            break;
        }
    }
    EXPECT_TRUE(has_new);

    auto new_symbol = regimeflow::SymbolRegistry::instance().intern("TEST2");
    auto range = source.get_available_range(new_symbol);
    EXPECT_GT(range.start.microseconds(), 0);
    EXPECT_GT(range.end.microseconds(), 0);
}

}  // namespace
