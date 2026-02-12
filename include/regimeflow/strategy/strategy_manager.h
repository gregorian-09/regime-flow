/**
 * @file strategy_manager.h
 * @brief RegimeFlow regimeflow strategy manager declarations.
 */

#pragma once

#include "regimeflow/strategy/strategy.h"

#include <memory>
#include <vector>

namespace regimeflow::strategy {

namespace engine = ::regimeflow::engine;

/**
 * @brief Manages multiple strategy instances.
 */
class StrategyManager {
public:
    /**
     * @brief Add a strategy to the manager.
     * @param strategy Strategy instance.
     */
    void add_strategy(std::unique_ptr<Strategy> strategy);
    /**
     * @brief Remove all strategies.
     */
    void clear();

    /**
     * @brief Initialize all strategies with context.
     * @param ctx Strategy context.
     */
    void initialize(StrategyContext& ctx);
    /**
     * @brief Start all strategies.
     */
    void start();
    /**
     * @brief Stop all strategies.
     */
    void stop();

    /**
     * @brief Dispatch bar events to strategies.
     */
    void on_bar(const data::Bar& bar);
    /**
     * @brief Dispatch tick events to strategies.
     */
    void on_tick(const data::Tick& tick);
    /**
     * @brief Dispatch quote events to strategies.
     */
    void on_quote(const data::Quote& quote);
    /**
     * @brief Dispatch order book events to strategies.
     */
    void on_order_book(const data::OrderBook& book);
    /**
     * @brief Dispatch order updates to strategies.
     */
    void on_order_update(const engine::Order& order);
    /**
     * @brief Dispatch fill events to strategies.
     */
    void on_fill(const engine::Fill& fill);
    /**
     * @brief Dispatch regime change events to strategies.
     */
    void on_regime_change(const regime::RegimeTransition& transition);
    /**
     * @brief Dispatch timer events to strategies.
     */
    void on_timer(const std::string& timer_id);

private:
    std::vector<std::unique_ptr<Strategy>> strategies_;
};

}  // namespace regimeflow::strategy
