/**
 * @file dispatcher.h
 * @brief RegimeFlow regimeflow dispatcher declarations.
 */

#pragma once

#include "regimeflow/events/event.h"

#include <functional>

namespace regimeflow::events {

/**
 * @brief Dispatches events to category-specific handlers.
 */
class EventDispatcher {
public:
    /**
     * @brief Handler signature.
     */
    using Handler = std::function<void(const Event&)>;

    /**
     * @brief Set handler for market events.
     * @param handler Callback.
     */
    void set_market_handler(Handler handler) { market_handler_ = std::move(handler); }
    /**
     * @brief Set handler for order events.
     * @param handler Callback.
     */
    void set_order_handler(Handler handler) { order_handler_ = std::move(handler); }
    /**
     * @brief Set handler for system events.
     * @param handler Callback.
     */
    void set_system_handler(Handler handler) { system_handler_ = std::move(handler); }
    /**
     * @brief Set handler for user-defined events.
     * @param handler Callback.
     */
    void set_user_handler(Handler handler) { user_handler_ = std::move(handler); }

    /**
     * @brief Dispatch an event to the appropriate handler.
     * @param event Event to dispatch.
     */
    void dispatch(const Event& event) const {
        switch (event.type) {
            case EventType::Market:
                if (market_handler_) market_handler_(event);
                break;
            case EventType::Order:
                if (order_handler_) order_handler_(event);
                break;
            case EventType::System:
                if (system_handler_) system_handler_(event);
                break;
            case EventType::User:
                if (user_handler_) user_handler_(event);
                break;
        }
    }

private:
    Handler market_handler_;
    Handler order_handler_;
    Handler system_handler_;
    Handler user_handler_;
};

}  // namespace regimeflow::events
