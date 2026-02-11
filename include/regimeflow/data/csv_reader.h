#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/validation_config.h"
#include "regimeflow/data/memory_data_source.h"

#include <map>
#include <string>
#include <unordered_map>

namespace regimeflow::data {

class CSVDataSource final : public DataSource {
public:
    struct Config {
        std::string data_directory;
        std::string file_pattern = "{symbol}.csv";
        std::string actions_directory;
        std::string actions_file_pattern = "{symbol}_actions.csv";
        std::string date_format = "%Y-%m-%d";
        std::string datetime_format = "%Y-%m-%d %H:%M:%S";
        std::string actions_datetime_format = "%Y-%m-%d %H:%M:%S";
        std::string actions_date_format = "%Y-%m-%d";
        char delimiter = ',';
        bool has_header = true;
        std::map<std::string, int> column_mapping;
        ValidationConfig validation;
        bool collect_validation_report = false;
        bool allow_symbol_column = false;
        std::string symbol_column = "symbol";
        std::map<std::string, std::string> column_aliases;
        int utc_offset_seconds = 0;
        bool fill_missing_bars = false;
    };

    explicit CSVDataSource(const Config& config);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols,
                                                   TimeRange range,
                                                   BarType bar_type) override;

    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;
    const ValidationReport& last_report() const { return last_report_; }
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

private:
    void scan_directory();
    std::string resolve_path(SymbolId symbol) const;
    std::vector<Bar> parse_bars(SymbolId symbol, const std::string& path, TimeRange range,
                                BarType bar_type) const;
    std::map<std::string, int> resolve_mapping(const std::string& header) const;
    void ensure_actions_loaded(SymbolId symbol) const;

    Config config_;
    std::unordered_map<SymbolId, std::string> symbol_to_path_;
    mutable ValidationReport last_report_;
    mutable CorporateActionAdjuster adjuster_;
    mutable std::unordered_map<SymbolId, std::vector<CorporateAction>> action_cache_;
};

}  // namespace regimeflow::data
