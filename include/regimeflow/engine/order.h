/**
 * @file order.h
 * @brief RegimeFlow regimeflow order declarations.
 */

#pragma once

#include "regimeflow/common/types.h"

#include <cstdint>
#include <map>
#include <string>

namespace regimeflow::engine {

/**
 * @brief Unique order identifier.
 */
using OrderId = uint64_t;
/**
 * @brief Unique fill identifier.
 */
using FillId = uint64_t;

/**
 * @brief Order side (buy/sell).
 */
enum class OrderSide : uint8_t {
    Buy,
    Sell
};

/**
 * @brief Order type.
 */
enum class OrderType : uint8_t {
    Market,
    Limit,
    Stop,
    StopLimit,
    MarketOnClose,
    MarketOnOpen
};

/**
 * @brief Time-in-force for orders.
 */
enum class TimeInForce : uint8_t {
    Day,
    GTC,
    IOC,
    FOK,
    GTD
};

/**
 * @brief Order lifecycle status.
 */
enum class OrderStatus : uint8_t {
    Created,
    Pending,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected,
    Invalid
};

/**
 * @brief Order representation used by engine and execution models.
 */
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

    /**
     * @brief Factory for a market order.
     * @param symbol Symbol ID.
     * @param side Buy or sell.
     * @param qty Quantity.
     * @return Order with market type.
     */
    static Order market(SymbolId symbol, OrderSide side, Quantity qty);
    /**
     * @brief Factory for a limit order.
     * @param symbol Symbol ID.
     * @param side Buy or sell.
     * @param qty Quantity.
     * @param price Limit price.
     * @return Order with limit type.
     */
    static Order limit(SymbolId symbol, OrderSide side, Quantity qty, Price price);
    /**
     * @brief Factory for a stop order.
     * @param symbol Symbol ID.
     * @param side Buy or sell.
     * @param qty Quantity.
     * @param stop Stop price.
     * @return Order with stop type.
     */
    static Order stop(SymbolId symbol, OrderSide side, Quantity qty, Price stop);
};

/**
 * @brief Execution fill information.
 */
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
