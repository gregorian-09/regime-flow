#include <gtest/gtest.h>

#include "regimeflow/data/csv_reader.h"

#include <filesystem>
#include <fstream>

namespace regimeflow::test {

TEST(CSVNormalization, AppliesUtcOffset) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "regimeflow_csv_norm";
    fs::create_directories(dir);

    const std::string symbol = "AAA";
    fs::path file = dir / (symbol + ".csv");

    std::ofstream out(file);
    out << "timestamp,open,high,low,close,volume\n";
    out << "2024-01-01 10:00:00,1,1,1,1,10\n";
    out.close();

    regimeflow::data::CSVDataSource::Config cfg;
    cfg.data_directory = dir.string();
    cfg.utc_offset_seconds = -3600; // source is UTC-1; normalize to UTC (+1h)

    regimeflow::data::CSVDataSource source(cfg);
    auto sym_id = regimeflow::SymbolRegistry::instance().intern(symbol);
    auto bars = source.get_bars(sym_id, {});

    ASSERT_EQ(bars.size(), 1u);
    EXPECT_EQ(bars[0].timestamp.to_string("%Y-%m-%d %H:%M:%S"), "2024-01-01 11:00:00");
}

TEST(CSVNormalization, FillsMissingDailyBars) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "regimeflow_csv_fill";
    fs::create_directories(dir);

    const std::string symbol = "BBB";
    fs::path file = dir / (symbol + ".csv");

    std::ofstream out(file);
    out << "timestamp,open,high,low,close,volume\n";
    out << "2024-01-01 00:00:00,10,10,10,10,100\n";
    out << "2024-01-03 00:00:00,12,12,12,12,120\n";
    out.close();

    regimeflow::data::CSVDataSource::Config cfg;
    cfg.data_directory = dir.string();
    cfg.fill_missing_bars = true;

    regimeflow::data::CSVDataSource source(cfg);
    auto sym_id = regimeflow::SymbolRegistry::instance().intern(symbol);
    auto bars = source.get_bars(sym_id, {}, regimeflow::data::BarType::Time_1Day);

    ASSERT_EQ(bars.size(), 3u);
    EXPECT_EQ(bars[1].timestamp.to_string("%Y-%m-%d %H:%M:%S"), "2024-01-02 00:00:00");
    EXPECT_EQ(bars[1].open, 10.0);
    EXPECT_EQ(bars[1].close, 10.0);
    EXPECT_EQ(bars[1].volume, 0u);
}

}  // namespace regimeflow::test
