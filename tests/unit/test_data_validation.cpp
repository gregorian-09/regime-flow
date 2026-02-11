#include <gtest/gtest.h>

#include "regimeflow/data/csv_reader.h"
#include "regimeflow/data/tick_csv_reader.h"
#include "regimeflow/common/time.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace regimeflow;
using namespace regimeflow::data;

namespace {

std::filesystem::path make_temp_dir(const std::string& name) {
    auto base = std::filesystem::temp_directory_path() / "regimeflow_tests" / name;
    std::filesystem::create_directories(base);
    return base;
}

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

}  // namespace

TEST(DataValidation, CsvVolumeBoundsSkipsInvalidRow) {
    auto dir = make_temp_dir("csv_volume_bounds");
    auto path = dir / "AAPL.csv";
    write_file(path,
               "timestamp,open,high,low,close,volume\n"
               "2024-01-01 00:00:00,10,11,9,10.5,1000\n"
               "2024-01-02 00:00:00,10,11,9,10.5,10\n");

    CSVDataSource::Config cfg;
    cfg.data_directory = dir.string();
    cfg.collect_validation_report = true;
    cfg.validation.check_volume_bounds = true;
    cfg.validation.max_volume = 100;
    cfg.validation.on_error = ValidationAction::Skip;

    CSVDataSource source(cfg);
    auto sym = SymbolRegistry::instance().intern("AAPL");
    auto bars = source.get_bars(sym, {});

    EXPECT_EQ(bars.size(), 1u);
    EXPECT_EQ(source.last_report().error_count(), 1);
}

TEST(DataValidation, CsvOutlierAddsWarning) {
    auto dir = make_temp_dir("csv_outliers");
    auto path = dir / "MSFT.csv";
    write_file(path,
               "timestamp,open,high,low,close,volume\n"
               "2024-01-01 00:00:00,100,100,100,100,10\n"
               "2024-01-02 00:00:00,101,101,101,101,10\n"
               "2024-01-03 00:00:00,1000,1000,1000,1000,10\n");

    CSVDataSource::Config cfg;
    cfg.data_directory = dir.string();
    cfg.collect_validation_report = true;
    cfg.validation.check_outliers = true;
    cfg.validation.outlier_zscore = 1.0;
    cfg.validation.outlier_warmup = 2;

    CSVDataSource source(cfg);
    auto sym = SymbolRegistry::instance().intern("MSFT");
    auto bars = source.get_bars(sym, {});

    EXPECT_EQ(bars.size(), 3u);
    EXPECT_GE(source.last_report().warning_count(), 1);
}

TEST(DataValidation, TickFutureTimestampIsRejected) {
    auto dir = make_temp_dir("tick_future_ts");
    auto path = dir / "AAPL_ticks.csv";

    auto future = (Timestamp::now() + Duration::days(1)).to_string();
    write_file(path,
               "timestamp,price,quantity\n" + future + ",10,1\n");

    CSVTickDataSource::Config cfg;
    cfg.data_directory = dir.string();
    cfg.collect_validation_report = true;
    cfg.validation.check_future_timestamps = true;
    cfg.validation.max_future_skew = Duration::seconds(0);
    cfg.validation.on_error = ValidationAction::Skip;

    CSVTickDataSource source(cfg);
    auto sym = SymbolRegistry::instance().intern("AAPL");
    auto ticks = source.get_ticks(sym, {});

    EXPECT_TRUE(ticks.empty());
    EXPECT_EQ(source.last_report().error_count(), 1);
}

TEST(DataValidation, CsvGapFillInsertsMissingBars) {
    auto dir = make_temp_dir("csv_gap_fill");
    auto path = dir / "AAPL.csv";
    write_file(path,
               "timestamp,open,high,low,close,volume\n"
               "2024-01-01 00:00:00,10,10,10,10,100\n"
               "2024-01-03 00:00:00,11,11,11,11,100\n");

    CSVDataSource::Config cfg;
    cfg.data_directory = dir.string();
    cfg.validation.check_gap = true;
    cfg.validation.max_gap = Duration::days(1);
    cfg.validation.on_gap = ValidationAction::Fill;

    CSVDataSource source(cfg);
    auto sym = SymbolRegistry::instance().intern("AAPL");
    auto bars = source.get_bars(sym, {});

    EXPECT_EQ(bars.size(), 3u);
    EXPECT_EQ(bars[1].timestamp,
              Timestamp::from_string("2024-01-02 00:00:00", "%Y-%m-%d %H:%M:%S"));
    EXPECT_EQ(bars[1].volume, 0u);
}
