/**
 * @file event.h
 * @brief RegimeFlow regimeflow event declarations.
 */

#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"

#include <cstdint>
#include <string>
#include <variant>

namespace regimeflow::events {

/**
 * @brief High-level event category.
 */
enum class EventType : uint8_t {
    Market,
    Order,
    System,
    User
};

/**
 * @brief Market event subtypes.
 */
enum class MarketEventKind : uint8_t {
    Bar,
    Tick,
    Quote,
    Book
};

/**
 * @brief Order event subtypes.
 */
enum class OrderEventKind : uint8_t {
    NewOrder,
    Fill,
    Cancel,
    Reject,
    Update
};

/**
 * @brief System event subtypes.
 */
enum class SystemEventKind : uint8_t {
    DayStart,
    EndOfDay,
    Timer,
    RegimeChange
};

/**
 * @brief Order ID for event payloads.
 */
using OrderId = uint64_t;
/**
 * @brief Fill ID for event payloads.
 */
using FillId = uint64_t;

/**
 * @brief Payload for market events.
 */
struct MarketEventPayload {
    MarketEventKind kind;
    std::variant<data::Bar, data::Tick, data::Quote, data::OrderBook> data;
};

/**
 * @brief Payload for order events.
 */
struct OrderEventPayload {
    OrderEventKind kind;
    OrderId order_id = 0;
    FillId fill_id = 0;
    Quantity quantity = 0;
    Price price = 0;
    double commission = 0.0;
};

/**
 * @brief Payload for system events.
 */
struct SystemEventPayload {
    SystemEventKind kind;
    int64_t code = 0;
    std::string id;
};

/**
 * @brief Variant for all event payloads.
 */
using EventPayload = std::variant<std::monostate, MarketEventPayload, OrderEventPayload, SystemEventPayload>;

/**
 * @brief Default priority levels by event category (lower first).
 */
constexpr uint8_t kSystemPriority = 0;
constexpr uint8_t kMarketPriority = 10;
constexpr uint8_t kOrderPriority = 20;
constexpr uint8_t kUserPriority = 30;

/**
 * @brief Unified event structure used across the engine.
 */
struct Event {
    Timestamp timestamp;
    EventType type = EventType::Market;
    uint8_t priority = kMarketPriority;
    uint64_t sequence = 0;
    SymbolId symbol = 0;
    EventPayload payload;
};

/**
 * @brief Map event type to default priority.
 * @param type Event type.
 * @return Default priority.
 */
inline uint8_t default_priority(EventType type) {
    switch (type) {
        case EventType::System:
            return kSystemPriority;
        case EventType::Market:
            return kMarketPriority;
        case EventType::Order:
            return kOrderPriority;
        case EventType::User:
            return kUserPriority;
    }
    return kUserPriority;
}

/**
 * @brief Create a market event for a bar.
 * @param bar Bar data.
 * @return Event instance.
 */
inline Event make_market_event(const data::Bar& bar) {
    Event event;
    event.timestamp = bar.timestamp;
    event.type = EventType::Market;
    event.priority = kMarketPriority;
    event.symbol = bar.symbol;
    event.payload = MarketEventPayload{MarketEventKind::Bar, bar};
    return event;
}

/**
 * @brief Create a market event for a tick.
 * @param tick Tick data.
 * @return Event instance.
 */
inline Event make_market_event(const data::Tick& tick) {
    Event event;
    event.timestamp = tick.timestamp;
    event.type = EventType::Market;
    event.priority = kMarketPriority;
    event.symbol = tick.symbol;
    event.payload = MarketEventPayload{MarketEventKind::Tick, tick};
    return event;
}

/**
 * @brief Create a market event for a quote.
 * @param quote Quote data.
 * @return Event instance.
 */
inline Event make_market_event(const data::Quote& quote) {
    Event event;
    event.timestamp = quote.timestamp;
    event.type = EventType::Market;
    event.priority = kMarketPriority;
    event.symbol = quote.symbol;
    event.payload = MarketEventPayload{MarketEventKind::Quote, quote};
    return event;
}

/**
 * @brief Create a market event for an order book snapshot.
 * @param book Order book data.
 * @return Event instance.
 */
inline Event make_market_event(const data::OrderBook& book) {
    Event event;
    event.timestamp = book.timestamp;
    event.type = EventType::Market;
    event.priority = kMarketPriority;
    event.symbol = book.symbol;
    event.payload = MarketEventPayload{MarketEventKind::Book, book};
    return event;
}

/**
 * @brief Create a system event.
 * @param kind System event kind.
 * @param timestamp Event time.
 * @param code Optional code.
 * @param id Optional identifier (e.g., timer id).
 * @return Event instance.
 */
inline Event make_system_event(SystemEventKind kind, Timestamp timestamp, int64_t code = 0,
                               std::string id = {}) {
    Event event;
    event.timestamp = timestamp;
    event.type = EventType::System;
    event.priority = kSystemPriority;
    event.payload = SystemEventPayload{kind, code, std::move(id)};
    return event;
}

/**
 * @brief Create an order event.
 * @param kind Order event kind.
 * @param timestamp Event time.
 * @param order_id Order ID.
 * @param fill_id Fill ID.
 * @param quantity Fill quantity.
 * @param price Fill price.
 * @param symbol Symbol ID.
 * @param commission Commission amount.
 * @return Event instance.
 */
inline Event make_order_event(OrderEventKind kind, Timestamp timestamp, OrderId order_id,
                              FillId fill_id = 0, Quantity quantity = 0, Price price = 0,
                              SymbolId symbol = 0, double commission = 0.0) {
    Event event;
    event.timestamp = timestamp;
    event.type = EventType::Order;
    event.priority = kOrderPriority;
    event.symbol = symbol;
    event.payload = OrderEventPayload{kind, order_id, fill_id, quantity, price, commission};
    return event;
}

}  // namespace regimeflow::events
