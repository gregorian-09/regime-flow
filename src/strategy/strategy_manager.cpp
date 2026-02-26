#include "regimeflow/strategy/strategy_manager.h"

namespace regimeflow::strategy
{
    void StrategyManager::add_strategy(std::unique_ptr<Strategy> strategy) {
        if (!strategy) {
            return;
        }
        strategies_.push_back(std::move(strategy));
    }

    void StrategyManager::clear() {
        strategies_.clear();
    }

    void StrategyManager::initialize(StrategyContext& ctx) const
    {
        for (const auto& strategy : strategies_) {
            strategy->set_context(&ctx);
            strategy->initialize(ctx);
        }
    }

    void StrategyManager::start() const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_start();
        }
    }

    void StrategyManager::stop() const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_stop();
        }
    }

    void StrategyManager::on_bar(const data::Bar& bar) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_bar(bar);
        }
    }

    void StrategyManager::on_tick(const data::Tick& tick) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_tick(tick);
        }
    }

    void StrategyManager::on_quote(const data::Quote& quote) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_quote(quote);
        }
    }

    void StrategyManager::on_order_book(const data::OrderBook& book) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_order_book(book);
        }
    }

    void StrategyManager::on_order_update(const engine::Order& order) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_order_update(order);
        }
    }

    void StrategyManager::on_fill(const engine::Fill& fill) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_fill(fill);
        }
    }

    void StrategyManager::on_regime_change(const regime::RegimeTransition& transition) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_regime_change(transition);
        }
    }

    void StrategyManager::on_timer(const std::string& timer_id) const
    {
        for (const auto& strategy : strategies_) {
            strategy->on_timer(timer_id);
        }
    }
}  // namespace regimeflow::strategy
