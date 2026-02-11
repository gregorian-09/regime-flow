#pragma once

#include "regimeflow/common/types.h"

namespace regimeflow::data {

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

    Price mid() const { return (high + low) / 2; }
    Price typical() const { return (high + low + close) / 3; }
    Price range() const { return high - low; }
    bool is_bullish() const { return close > open; }
    bool is_bearish() const { return close < open; }
};

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
