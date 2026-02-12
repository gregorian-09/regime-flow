/**
 * @file bar.h
 * @brief RegimeFlow regimeflow bar declarations.
 */

#pragma once

#include "regimeflow/common/types.h"

namespace regimeflow::data {

/**
 * @brief OHLCV bar representation.
 */
struct Bar {
    Timestamp timestamp;
    SymbolId symbol;
    Price open;
    Price high;
    Price low;
    Price close;
    Volume volume;
    Volume trade_count = 0;
    Price vwap = 0;

    /**
     * @brief Mid price between high and low.
     */
    Price mid() const { return (high + low) / 2; }
    /**
     * @brief Typical price (high+low+close)/3.
     */
    Price typical() const { return (high + low + close) / 3; }
    /**
     * @brief High-low range.
     */
    Price range() const { return high - low; }
    /**
     * @brief True if close > open.
     */
    bool is_bullish() const { return close > open; }
    /**
     * @brief True if close < open.
     */
    bool is_bearish() const { return close < open; }
};

/**
 * @brief Bar aggregation type.
 */
enum class BarType {
    Time_1Min,
    Time_5Min,
    Time_15Min,
    Time_30Min,
    Time_1Hour,
    Time_4Hour,
    Time_1Day,
    Volume,
    Tick,
    Dollar
};

}  // namespace regimeflow::data
