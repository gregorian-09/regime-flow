/**
 * @file alpaca_data_client.h
 * @brief RegimeFlow alpaca data REST client declarations.
 */

#pragma once

#include "regimeflow/common/result.h"

#include <string>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Lightweight Alpaca REST client for assets and bars.
 */
class AlpacaDataClient {
public:
    /**
     * @brief Alpaca REST client configuration.
     */
    struct Config {
        std::string api_key;
        std::string secret_key;
        std::string trading_base_url;
        std::string data_base_url;
        int timeout_seconds = 10;
    };

    /**
     * @brief Construct a client.
     * @param config REST client config.
     */
    explicit AlpacaDataClient(Config config);

    /**
     * @brief Fetch assets list.
     * @param status Asset status filter (default: active).
     * @param asset_class Asset class filter (default: us_equity).
     */
    Result<std::string> list_assets(const std::string& status = "active",
                                    const std::string& asset_class = "us_equity") const;

    /**
     * @brief Fetch historical bars for symbols.
     * @param symbols Symbols to request.
     * @param timeframe Timeframe (e.g., 1Day, 1Min).
     * @param start ISO timestamp or date (YYYY-MM-DD).
     * @param end ISO timestamp or date (YYYY-MM-DD).
     * @param limit Optional limit.
     * @param page_token Optional pagination token.
     */
    Result<std::string> get_bars(const std::vector<std::string>& symbols,
                                 const std::string& timeframe,
                                 const std::string& start,
                                 const std::string& end,
                                 int limit = 0,
                                 const std::string& page_token = "") const;

    /**
     * @brief Fetch trades (ticks) for symbols.
     * @param symbols Symbols to request.
     * @param start ISO timestamp or date (YYYY-MM-DD).
     * @param end ISO timestamp or date (YYYY-MM-DD).
     * @param limit Optional limit.
     * @param page_token Optional pagination token.
     */
    Result<std::string> get_trades(const std::vector<std::string>& symbols,
                                   const std::string& start,
                                   const std::string& end,
                                   int limit = 0,
                                   const std::string& page_token = "") const;

    /**
     * @brief Fetch latest snapshot for a symbol.
     * @param symbol Symbol to request.
     */
    Result<std::string> get_snapshot(const std::string& symbol) const;

private:
    Result<std::string> rest_get(const std::string& base_url,
                                 const std::string& path,
                                 const std::string& query) const;

    Config config_;
};

}  // namespace regimeflow::data
