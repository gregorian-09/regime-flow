/**
 * @file tick.h
 * @brief RegimeFlow regimeflow tick declarations.
 */

#pragma once

#include "regimeflow/common/types.h"

#include <cstdint>

namespace regimeflow::data {

/**
 * @brief Trade tick representation.
 */
struct Tick {
    Timestamp timestamp;
    SymbolId symbol;
    Price price;
    Quantity quantity;
    uint8_t flags = 0;
};

/**
 * @brief Quote snapshot (best bid/ask).
 */
struct Quote {
    Timestamp timestamp;
    SymbolId symbol;
    Price bid;
    Price ask;
    Quantity bid_size;
    Quantity ask_size;

    /**
     * @brief Mid price between bid and ask.
     */
    Price mid() const { return (bid + ask) / 2; }
    /**
     * @brief Absolute spread.
     */
    Price spread() const { return ask - bid; }
    /**
     * @brief Spread in basis points relative to mid.
     */
    Price spread_bps() const { return spread() / mid() * 10000; }
};

}  // namespace regimeflow::data
