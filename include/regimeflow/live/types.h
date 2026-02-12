/**
 * @file types.h
 * @brief RegimeFlow regimeflow types declarations.
 */

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

/**
 * @brief Live position snapshot.
 */
struct Position {
    std::string symbol;
    double quantity = 0.0;
    double average_price = 0.0;
    double market_value = 0.0;
};

/**
 * @brief Account-level information for live trading.
 */
struct AccountInfo {
    double equity = 0.0;
    double cash = 0.0;
    double buying_power = 0.0;
};

/**
 * @brief Market data update wrapper for live feeds.
 */
struct MarketDataUpdate {
    std::variant<data::Bar, data::Tick, data::Quote, data::OrderBook> data;

    /**
     * @brief Extract timestamp from the underlying data.
     */
    Timestamp timestamp() const;
    /**
     * @brief Extract symbol ID from the underlying data.
     */
    SymbolId symbol() const;
};

/**
 * @brief Trade execution record.
 */
struct Trade {
    std::string symbol;
    double quantity = 0.0;
    double price = 0.0;
    Timestamp timestamp;
};

}  // namespace regimeflow::live
