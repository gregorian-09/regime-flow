/**
 * @file order_book_cache.h
 * @brief RegimeFlow regimeflow order book cache declarations.
 */

#pragma once

#include "regimeflow/data/order_book.h"

#include <optional>
#include <unordered_map>

namespace regimeflow::engine {

/**
 * @brief In-memory cache of latest order book snapshots.
 */
class OrderBookCache {
public:
    /**
     * @brief Update the cache with a new order book snapshot.
     * @param book Order book snapshot.
     */
    void update(const data::OrderBook& book);
    /**
     * @brief Retrieve the latest order book for a symbol.
     * @param symbol Symbol ID.
     * @return Optional order book.
     */
    std::optional<data::OrderBook> latest(SymbolId symbol) const;

private:
    std::unordered_map<SymbolId, data::OrderBook> books_;
};

}  // namespace regimeflow::engine
