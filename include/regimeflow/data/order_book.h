/**
 * @file order_book.h
 * @brief RegimeFlow regimeflow order book declarations.
 */

#pragma once

#include "regimeflow/common/types.h"

#include <array>

namespace regimeflow::data {

/**
 * @brief Single level in the order book.
 */
struct BookLevel {
    Price price = 0;
    Quantity quantity = 0;
    int num_orders = 0;
};

/**
 * @brief Snapshot of top-of-book depth.
 */
struct OrderBook {
    Timestamp timestamp;
    SymbolId symbol = 0;
    std::array<BookLevel, 10> bids{};
    std::array<BookLevel, 10> asks{};
};

}  // namespace regimeflow::data
