#pragma once

#include "regimeflow/events/event.h"

#include <functional>

namespace regimeflow::events {

class EventDispatcher {
public:
    using Handler = std::function<void(const Event&)>;

    void set_market_handler(Handler handler) { market_handler_ = std::move(handler); }
    void set_order_handler(Handler handler) { order_handler_ = std::move(handler); }
    void set_system_handler(Handler handler) { system_handler_ = std::move(handler); }
    void set_user_handler(Handler handler) { user_handler_ = std::move(handler); }

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
