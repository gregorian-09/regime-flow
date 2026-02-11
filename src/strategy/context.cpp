#include "regimeflow/strategy/context.h"

#include "regimeflow/engine/event_loop.h"
#include "regimeflow/engine/regime_tracker.h"

namespace regimeflow::strategy {

StrategyContext::StrategyContext(engine::OrderManager* order_manager,
                                 engine::Portfolio* portfolio,
                                 engine::EventLoop* event_loop,
                                 engine::MarketDataCache* market_data,
                                 engine::OrderBookCache* order_books,
                                 engine::TimerService* timer_service,
                                 engine::RegimeTracker* regime_tracker,
                                 Config config)
    : order_manager_(order_manager),
      portfolio_(portfolio),
      event_loop_(event_loop),
      market_data_(market_data),
      order_books_(order_books),
      timer_service_(timer_service),
      regime_tracker_(regime_tracker),
      config_(std::move(config)) {}

Result<engine::OrderId> StrategyContext::submit_order(engine::Order order) {
    if (!order_manager_) {
        return Error(Error::Code::InvalidState, "Order manager not available");
    }
    if (order.created_at.microseconds() == 0) {
        order.created_at = current_time();
        order.updated_at = order.created_at;
    }
    auto regime = current_regime().regime;
    switch (regime) {
        case regime::RegimeType::Bull: order.metadata["regime"] = "bull"; break;
        case regime::RegimeType::Neutral: order.metadata["regime"] = "neutral"; break;
        case regime::RegimeType::Bear: order.metadata["regime"] = "bear"; break;
        case regime::RegimeType::Crisis: order.metadata["regime"] = "crisis"; break;
        case regime::RegimeType::Custom: order.metadata["regime"] = "custom"; break;
    }
    return order_manager_->submit_order(std::move(order));
}

Result<void> StrategyContext::cancel_order(engine::OrderId id) {
    if (!order_manager_) {
        return Error(Error::Code::InvalidState, "Order manager not available");
    }
    return order_manager_->cancel_order(id);
}

engine::Portfolio& StrategyContext::portfolio() {
    return *portfolio_;
}

const engine::Portfolio& StrategyContext::portfolio() const {
    return *portfolio_;
}

std::optional<data::Bar> StrategyContext::latest_bar(SymbolId symbol) const {
    return market_data_ ? market_data_->latest_bar(symbol) : std::nullopt;
}

std::optional<data::Tick> StrategyContext::latest_tick(SymbolId symbol) const {
    return market_data_ ? market_data_->latest_tick(symbol) : std::nullopt;
}

std::optional<data::Quote> StrategyContext::latest_quote(SymbolId symbol) const {
    return market_data_ ? market_data_->latest_quote(symbol) : std::nullopt;
}

std::vector<data::Bar> StrategyContext::recent_bars(SymbolId symbol, size_t count) const {
    return market_data_ ? market_data_->recent_bars(symbol, count) : std::vector<data::Bar>{};
}

std::optional<data::OrderBook> StrategyContext::latest_order_book(SymbolId symbol) const {
    return order_books_ ? order_books_->latest(symbol) : std::nullopt;
}

const regime::RegimeState& StrategyContext::current_regime() const {
    static const regime::RegimeState default_state{};
    return regime_tracker_ ? regime_tracker_->current_state() : default_state;
}

void StrategyContext::schedule_timer(const std::string& id, Duration interval) {
    if (!timer_service_) {
        return;
    }
    timer_service_->schedule(id, interval, current_time());
}

void StrategyContext::cancel_timer(const std::string& id) {
    if (!timer_service_) {
        return;
    }
    timer_service_->cancel(id);
}

Timestamp StrategyContext::current_time() const {
    return event_loop_ ? event_loop_->current_time() : Timestamp::now();
}

}  // namespace regimeflow::strategy
