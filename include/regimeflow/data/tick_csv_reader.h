/**
 * @file tick_csv_reader.h
 * @brief RegimeFlow regimeflow tick csv reader declarations.
 */

#pragma once

#include "regimeflow/data/csv_reader.h"
#include "regimeflow/data/validation_config.h"

namespace regimeflow::data {

/**
 * @brief Data source for tick data stored in CSV files.
 */
class CSVTickDataSource final : public DataSource {
public:
    /**
     * @brief CSV tick data source configuration.
     */
    struct Config {
        /**
         * @brief Root directory for tick files.
         */
        std::string data_directory;
        /**
         * @brief File pattern for tick files.
         */
        std::string file_pattern = "{symbol}_ticks.csv";
        /**
         * @brief Date-time format for timestamps.
         */
        std::string datetime_format = "%Y-%m-%d %H:%M:%S";
        /**
         * @brief Column delimiter.
         */
        char delimiter = ',';
        /**
         * @brief Whether CSV has a header row.
         */
        bool has_header = true;
        /**
         * @brief Explicit column mapping.
         */
        std::map<std::string, int> column_mapping;
        /**
         * @brief Collect validation report if true.
         */
        bool collect_validation_report = false;
        /**
         * @brief Validation configuration.
         */
        ValidationConfig validation;
        /**
         * @brief UTC offset in seconds to apply to timestamps.
         */
        int utc_offset_seconds = 0;
    };

    /**
     * @brief Construct a tick CSV data source.
     * @param config Configuration.
     */
    explicit CSVTickDataSource(const Config& config);

    /**
     * @brief List available symbols.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Bars not supported; returns empty.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Load ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Bars not supported; returns empty iterator.
     */
    std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols,
                                                   TimeRange range,
                                                   BarType bar_type) override;
    /**
     * @brief Create a tick iterator for multiple symbols.
     */
    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;

    /**
     * @brief Corporate actions not supported; returns empty.
     */
    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;

    /**
     * @brief Last validation report.
     */
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
