/**
 * @file csv_reader.h
 * @brief RegimeFlow regimeflow csv reader declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/validation_config.h"
#include "regimeflow/data/memory_data_source.h"

#include <map>
#include <string>
#include <unordered_map>

namespace regimeflow::data {

/**
 * @brief Data source that reads bars/ticks from CSV files.
 */
class CSVDataSource final : public DataSource {
public:
    /**
     * @brief CSV data source configuration.
     */
    struct Config {
        /**
         * @brief Root directory containing data files.
         */
        std::string data_directory;
        /**
         * @brief File pattern for bar data.
         */
        std::string file_pattern = "{symbol}.csv";
        /**
         * @brief Directory for corporate actions.
         */
        std::string actions_directory;
        /**
         * @brief File pattern for corporate actions.
         */
        std::string actions_file_pattern = "{symbol}_actions.csv";
        /**
         * @brief Date format for daily data.
         */
        std::string date_format = "%Y-%m-%d";
        /**
         * @brief Date-time format for intraday data.
         */
        std::string datetime_format = "%Y-%m-%d %H:%M:%S";
        /**
         * @brief Date-time format for corporate actions timestamps.
         */
        std::string actions_datetime_format = "%Y-%m-%d %H:%M:%S";
        /**
         * @brief Date format for corporate actions.
         */
        std::string actions_date_format = "%Y-%m-%d";
        /**
         * @brief Column delimiter.
         */
        char delimiter = ',';
        /**
         * @brief Whether CSV files have a header row.
         */
        bool has_header = true;
        /**
         * @brief Explicit column mapping.
         */
        std::map<std::string, int> column_mapping;
        /**
         * @brief Validation configuration for parsed data.
         */
        ValidationConfig validation;
        /**
         * @brief Whether to collect validation report.
         */
        bool collect_validation_report = false;
        /**
         * @brief Allow a symbol column per row.
         */
        bool allow_symbol_column = false;
        /**
         * @brief Symbol column name if enabled.
         */
        std::string symbol_column = "symbol";
        /**
         * @brief Column alias mapping for flexible input headers.
         */
        std::map<std::string, std::string> column_aliases;
        /**
         * @brief UTC offset to apply to timestamps.
         */
        int utc_offset_seconds = 0;
        /**
         * @brief Fill missing bars if possible.
         */
        bool fill_missing_bars = false;
    };

    /**
     * @brief Construct a CSV data source.
     * @param config CSV configuration.
     */
    explicit CSVDataSource(const Config& config);

    /**
     * @brief Enumerate symbols available on disk.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Determine available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Load bars for a symbol and range.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Load ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Create a multi-symbol iterator for bars.
     */
    std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols,
                                                   TimeRange range,
                                                   BarType bar_type) override;

    /**
     * @brief Load corporate actions for a symbol.
     */
    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;
    /**
     * @brief Last validation report from parsing.
     */
    const ValidationReport& last_report() const { return last_report_; }
    /**
     * @brief Inject corporate actions programmatically.
     * @param symbol Symbol ID.
     * @param actions Action list.
     */
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
