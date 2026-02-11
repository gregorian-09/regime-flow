#pragma once

#include "regimeflow/common/types.h"

#include <cstdint>
#include <map>
#include <string>

namespace regimeflow::engine {

using OrderId = uint64_t;
using FillId = uint64_t;

enum class OrderSide : uint8_t {
    Buy,
    Sell
};

enum class OrderType : uint8_t {
    Market,
    Limit,
    Stop,
    StopLimit,
    MarketOnClose,
    MarketOnOpen
};

enum class TimeInForce : uint8_t {
    Day,
    GTC,
    IOC,
    FOK,
    GTD
};

enum class OrderStatus : uint8_t {
    Created,
    Pending,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected,
    Invalid
};

struct Order {
    OrderId id = 0;
    SymbolId symbol = 0;
    OrderSide side = OrderSide::Buy;
    OrderType type = OrderType::Market;
    TimeInForce tif = TimeInForce::Day;
    Quantity quantity = 0;
    Quantity filled_quantity = 0;
    Price limit_price = 0;
    Price stop_price = 0;
    Price avg_fill_price = 0;
    OrderStatus status = OrderStatus::Created;
    Timestamp created_at;
    Timestamp updated_at;
    std::string strategy_id;
    std::map<std::string, std::string> metadata;

    static Order market(SymbolId symbol, OrderSide side, Quantity qty);
    static Order limit(SymbolId symbol, OrderSide side, Quantity qty, Price price);
    static Order stop(SymbolId symbol, OrderSide side, Quantity qty, Price stop);
};

struct Fill {
    FillId id = 0;
    OrderId order_id = 0;
    SymbolId symbol = 0;
    Quantity quantity = 0;
    Price price = 0;
    Timestamp timestamp;
    double commission = 0;
    double slippage = 0;
    bool is_maker = false;
};

}  // namespace regimeflow::engine
