/**
 * @file alpaca_data_source.h
 * @brief RegimeFlow alpaca data source declarations.
 */

#pragma once

#include "regimeflow/data/alpaca_data_client.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/merged_iterator.h"
#include "regimeflow/data/memory_data_source.h"

#include <unordered_set>
#include <unordered_map>

namespace regimeflow::data {

/**
 * @brief Data source that pulls bars from Alpaca REST.
 */
class AlpacaDataSource final : public DataSource {
public:
    /**
     * @brief Configuration for Alpaca REST data source.
     */
    struct Config {
        std::string api_key;
        std::string secret_key;
        std::string trading_base_url = "https://paper-api.alpaca.markets";
        std::string data_base_url = "https://data.alpaca.markets";
        int timeout_seconds = 10;
        std::vector<std::string> symbols;
    };

    /**
     * @brief Construct an Alpaca data source.
     */
    explicit AlpacaDataSource(Config config);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;

    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;

    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;

private:
    AlpacaDataClient client_;
    std::vector<std::string> symbols_;
    std::unordered_set<SymbolId> allowed_symbols_;
};

}  // namespace regimeflow::data
