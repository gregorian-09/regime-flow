#include "regimeflow/strategy/strategy_manager.h"

namespace regimeflow::strategy {

void StrategyManager::add_strategy(std::unique_ptr<Strategy> strategy) {
    if (!strategy) {
        return;
    }
    strategies_.push_back(std::move(strategy));
}

void StrategyManager::clear() {
    strategies_.clear();
}

void StrategyManager::initialize(StrategyContext& ctx) {
    for (auto& strategy : strategies_) {
        strategy->set_context(&ctx);
        strategy->initialize(ctx);
    }
}

void StrategyManager::start() {
    for (auto& strategy : strategies_) {
        strategy->on_start();
    }
}

void StrategyManager::stop() {
    for (auto& strategy : strategies_) {
        strategy->on_stop();
    }
}

void StrategyManager::on_bar(const data::Bar& bar) {
    for (auto& strategy : strategies_) {
        strategy->on_bar(bar);
    }
}

void StrategyManager::on_tick(const data::Tick& tick) {
    for (auto& strategy : strategies_) {
        strategy->on_tick(tick);
    }
}

void StrategyManager::on_quote(const data::Quote& quote) {
    for (auto& strategy : strategies_) {
        strategy->on_quote(quote);
    }
}

void StrategyManager::on_order_book(const data::OrderBook& book) {
    for (auto& strategy : strategies_) {
        strategy->on_order_book(book);
    }
}

void StrategyManager::on_order_update(const engine::Order& order) {
    for (auto& strategy : strategies_) {
        strategy->on_order_update(order);
    }
}

void StrategyManager::on_fill(const engine::Fill& fill) {
    for (auto& strategy : strategies_) {
        strategy->on_fill(fill);
    }
}

void StrategyManager::on_regime_change(const regime::RegimeTransition& transition) {
    for (auto& strategy : strategies_) {
        strategy->on_regime_change(transition);
    }
}

void StrategyManager::on_timer(const std::string& timer_id) {
    for (auto& strategy : strategies_) {
        strategy->on_timer(timer_id);
    }
}

}  // namespace regimeflow::strategy
