#include "regimeflow/risk/stop_loss.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::risk {

void StopLossManager::configure(StopLossConfig config) {
    config_ = std::move(config);
}

void StopLossManager::on_position_update(const engine::Position& position) {
    auto& state = states_[position.symbol];
    Quantity qty = position.quantity;
    if (qty == 0) {
        state = StopState{};
        return;
    }
    if (state.last_qty == 0 || (state.last_qty > 0 && qty < 0) ||
        (state.last_qty < 0 && qty > 0)) {
        state.entry_price = position.avg_cost;
        state.entry_time = position.last_update;
        state.highest = position.current_price;
        state.lowest = position.current_price;
        state.exit_requested = false;
        state.prev_close = position.current_price;
        state.true_ranges.clear();
    } else {
        state.entry_price = position.avg_cost;
        state.highest = std::max(state.highest, position.current_price);
        state.lowest = std::min(state.lowest, position.current_price);
    }
    state.last_qty = qty;
}

void StopLossManager::on_bar(const data::Bar& bar, engine::OrderManager& order_manager) {
    update_atr(bar.symbol, bar);
    update_from_price(bar.symbol, bar.close, bar.timestamp, order_manager);
}

void StopLossManager::on_tick(const data::Tick& tick, engine::OrderManager& order_manager) {
    update_from_price(tick.symbol, tick.price, tick.timestamp, order_manager);
}

void StopLossManager::update_atr(SymbolId symbol, const data::Bar& bar) {
    auto it = states_.find(symbol);
    if (it == states_.end()) {
        return;
    }
    auto& state = it->second;
    double tr = bar.high - bar.low;
    if (state.prev_close > 0.0) {
        tr = std::max(tr, std::abs(bar.high - state.prev_close));
        tr = std::max(tr, std::abs(bar.low - state.prev_close));
    }
    state.prev_close = bar.close;
    state.true_ranges.push_back(tr);
    if (static_cast<int>(state.true_ranges.size()) > config_.atr_window) {
        state.true_ranges.pop_front();
    }
    double sum = 0.0;
    for (double v : state.true_ranges) sum += v;
    state.last_atr = state.true_ranges.empty() ? 0.0
                                               : sum / static_cast<double>(state.true_ranges.size());
}

void StopLossManager::update_from_price(SymbolId symbol, Price price, Timestamp ts,
                                        engine::OrderManager& order_manager) {
    auto it = states_.find(symbol);
    if (it == states_.end()) {
        return;
    }
    auto& state = it->second;
    if (state.last_qty == 0) {
        return;
    }
    if (price > 0.0) {
        state.highest = std::max(state.highest, price);
        state.lowest = std::min(state.lowest, price);
    }
    maybe_exit(symbol, price, ts, order_manager);
}

void StopLossManager::maybe_exit(SymbolId symbol, Price price, Timestamp ts,
                                 engine::OrderManager& order_manager) {
    auto it = states_.find(symbol);
    if (it == states_.end()) {
        return;
    }
    auto& state = it->second;
    if (state.exit_requested || state.last_qty == 0) {
        return;
    }

    bool is_long = state.last_qty > 0;
    bool trigger = false;

    if (config_.enable_fixed && state.entry_price > 0.0) {
        double stop = is_long
            ? state.entry_price * (1.0 - config_.stop_loss_pct)
            : state.entry_price * (1.0 + config_.stop_loss_pct);
        if ((is_long && price <= stop) || (!is_long && price >= stop)) {
            trigger = true;
        }
    }

    if (!trigger && config_.enable_trailing && price > 0.0) {
        double stop = is_long
            ? state.highest * (1.0 - config_.trailing_stop_pct)
            : state.lowest * (1.0 + config_.trailing_stop_pct);
        if ((is_long && price <= stop) || (!is_long && price >= stop)) {
            trigger = true;
        }
    }

    if (!trigger && config_.enable_atr && state.entry_price > 0.0 && state.last_atr > 0.0) {
        double stop = is_long
            ? state.entry_price - (state.last_atr * config_.atr_multiplier)
            : state.entry_price + (state.last_atr * config_.atr_multiplier);
        if ((is_long && price <= stop) || (!is_long && price >= stop)) {
            trigger = true;
        }
    }

    if (!trigger && config_.enable_time && config_.max_holding_seconds > 0) {
        if (ts.microseconds() - state.entry_time.microseconds()
            >= config_.max_holding_seconds * 1'000'000LL) {
            trigger = true;
        }
    }

    if (!trigger) {
        return;
    }

    engine::Order order = engine::Order::market(
        symbol,
        is_long ? engine::OrderSide::Sell : engine::OrderSide::Buy,
        std::abs(state.last_qty));
    order.metadata["risk_exit"] = "1";
    order.created_at = ts;
    order.updated_at = ts;
    order_manager.submit_order(order);
    state.exit_requested = true;
}

}  // namespace regimeflow::risk
