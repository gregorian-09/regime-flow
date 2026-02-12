/**
 * @file market_data_cache.h
 * @brief RegimeFlow regimeflow market data cache declarations.
 */

#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::engine {

/**
 * @brief In-memory cache of latest market data and short history.
 */
class MarketDataCache {
public:
    /**
     * @brief Update cache with a bar.
     * @param bar Bar data.
     */
    void update(const data::Bar& bar);
    /**
     * @brief Update cache with a tick.
     * @param tick Tick data.
     */
    void update(const data::Tick& tick);
    /**
     * @brief Update cache with a quote.
     * @param quote Quote data.
     */
    void update(const data::Quote& quote);

    /**
     * @brief Latest bar for a symbol.
     * @param symbol Symbol ID.
     * @return Optional bar.
     */
    std::optional<data::Bar> latest_bar(SymbolId symbol) const;
    /**
     * @brief Latest tick for a symbol.
     * @param symbol Symbol ID.
     * @return Optional tick.
     */
    std::optional<data::Tick> latest_tick(SymbolId symbol) const;
    /**
     * @brief Latest quote for a symbol.
     * @param symbol Symbol ID.
     * @return Optional quote.
     */
    std::optional<data::Quote> latest_quote(SymbolId symbol) const;

    /**
     * @brief Recent bar history for a symbol.
     * @param symbol Symbol ID.
     * @param count Number of bars to return.
     * @return Vector of most recent bars (up to count).
     */
    std::vector<data::Bar> recent_bars(SymbolId symbol, size_t count) const;

private:
    std::unordered_map<SymbolId, data::Bar> latest_bars_;
    std::unordered_map<SymbolId, data::Tick> latest_ticks_;
    std::unordered_map<SymbolId, data::Quote> latest_quotes_;
    std::unordered_map<SymbolId, std::vector<data::Bar>> bar_history_;
    size_t max_history_ = 1024;
};

}  // namespace regimeflow::engine
