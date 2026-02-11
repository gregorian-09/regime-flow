#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/regime/types.h"

#include <string>
#include <variant>
#include <vector>

namespace regimeflow::live {

struct Position {
    std::string symbol;
    double quantity = 0.0;
    double average_price = 0.0;
    double market_value = 0.0;
};

struct AccountInfo {
    double equity = 0.0;
    double cash = 0.0;
    double buying_power = 0.0;
};

struct MarketDataUpdate {
    std::variant<data::Bar, data::Tick, data::Quote, data::OrderBook> data;

    Timestamp timestamp() const;
    SymbolId symbol() const;
};

struct Trade {
    std::string symbol;
    double quantity = 0.0;
    double price = 0.0;
    Timestamp timestamp;
};

}  // namespace regimeflow::live
