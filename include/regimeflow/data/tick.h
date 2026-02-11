#pragma once

#include "regimeflow/common/types.h"

#include <cstdint>

namespace regimeflow::data {

struct Tick {
    Timestamp timestamp;
    SymbolId symbol;
    Price price;
    Quantity quantity;
    uint8_t flags = 0;
};

struct Quote {
    Timestamp timestamp;
    SymbolId symbol;
    Price bid;
    Price ask;
    Quantity bid_size;
    Quantity ask_size;

    Price mid() const { return (bid + ask) / 2; }
    Price spread() const { return ask - bid; }
    Price spread_bps() const { return spread() / mid() * 10000; }
};

}  // namespace regimeflow::data
