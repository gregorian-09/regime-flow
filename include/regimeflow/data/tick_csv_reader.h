#pragma once

#include "regimeflow/data/csv_reader.h"
#include "regimeflow/data/validation_config.h"

namespace regimeflow::data {

class CSVTickDataSource final : public DataSource {
public:
    struct Config {
        std::string data_directory;
        std::string file_pattern = "{symbol}_ticks.csv";
        std::string datetime_format = "%Y-%m-%d %H:%M:%S";
        char delimiter = ',';
        bool has_header = true;
        std::map<std::string, int> column_mapping;
        bool collect_validation_report = false;
        ValidationConfig validation;
        int utc_offset_seconds = 0;
    };

    explicit CSVTickDataSource(const Config& config);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols,
                                                   TimeRange range,
                                                   BarType bar_type) override;
    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;

    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;

    const ValidationReport& last_report() const { return last_report_; }

private:
    std::string resolve_path(SymbolId symbol) const;
    std::vector<Tick> parse_ticks(SymbolId symbol, const std::string& path, TimeRange range) const;
    void scan_directory();

    Config config_;
    std::unordered_map<SymbolId, std::string> symbol_to_path_;
    mutable ValidationReport last_report_;
};

}  // namespace regimeflow::data
