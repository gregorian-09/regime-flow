/**
 * @file api_data_source.h
 * @brief RegimeFlow regimeflow api data source declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/validation_config.h"

#include <functional>
#include <string>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Data source that pulls market data from a REST API.
 */
class ApiDataSource : public DataSource {
public:
    /**
     * @brief Configuration for API endpoints and parsing.
     */
    struct Config {
        /**
         * @brief Base URL for the API.
         */
        std::string base_url;
        /**
         * @brief Endpoint path for bars.
         */
        std::string bars_endpoint = "/bars";
        /**
         * @brief Endpoint path for ticks.
         */
        std::string ticks_endpoint = "/ticks";
        /**
         * @brief API key value.
         */
        std::string api_key;
        /**
         * @brief Header name used for API key.
         */
        std::string api_key_header = "X-API-KEY";
        /**
         * @brief Response format (e.g., csv).
         */
        std::string format = "csv";
        /**
         * @brief Time format in API responses (e.g., epoch_ms).
         */
        std::string time_format = "epoch_ms";
        /**
         * @brief Request timeout in seconds.
         */
        int timeout_seconds = 10;
        /**
         * @brief Symbol list to prefetch or validate.
         */
        std::vector<std::string> symbols;
        /**
         * @brief Validation configuration for incoming data.
         */
        ValidationConfig validation;
        /**
         * @brief Whether to collect a validation report.
         */
        bool collect_validation_report = false;
        /**
         * @brief Whether to fill missing bars during validation.
         */
        bool fill_missing_bars = false;
    };

    /**
     * @brief Construct with configuration.
     * @param config API configuration.
     */
    explicit ApiDataSource(Config config);

    /**
     * @brief Retrieve available symbols from the API.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Retrieve available data range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Fetch bars for a symbol and range.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Fetch ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Create an iterator over bars for multiple symbols.
     */
    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;

    /**
     * @brief Fetch corporate actions for a symbol and range.
     */
    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;
    /**
     * @brief Return the last validation report.
     */
    const ValidationReport& last_report() const { return last_report_; }

private:
    Config config_;
    mutable ValidationReport last_report_;

    std::string build_url(const std::string& endpoint,
                          const std::string& symbol,
                          TimeRange range,
                          BarType bar_type) const;
    std::string format_timestamp(Timestamp ts) const;
};

}  // namespace regimeflow::data
