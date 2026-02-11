#include <gtest/gtest.h>

#include "regimeflow/data/db_csv_adapter.h"
#include "regimeflow/data/csv_reader.h"

#include <filesystem>

namespace {

TEST(CsvDbClientTest, ReadsBarsFromCsv) {
    regimeflow::data::CSVDataSource::Config cfg;
    cfg.data_directory = (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string();
    cfg.file_pattern = "{symbol}.csv";
    cfg.has_header = true;

    regimeflow::data::CSVDataSource source(cfg);
    regimeflow::data::CsvDbClient client(std::move(source));

    auto symbol = regimeflow::SymbolRegistry::instance().intern("TEST");
    regimeflow::TimeRange range;
    range.start = regimeflow::Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    range.end = regimeflow::Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");

    auto bars = client.query_bars(symbol, range, regimeflow::data::BarType::Time_1Day);
    EXPECT_FALSE(bars.empty());
}

}  // namespace
