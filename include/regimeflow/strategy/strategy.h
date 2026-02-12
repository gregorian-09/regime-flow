/**
 * @file strategy.h
 * @brief RegimeFlow regimeflow strategy declarations.
 */

#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/regime/types.h"
#include "regimeflow/strategy/context.h"

#include <string>

namespace regimeflow::strategy {

namespace engine = ::regimeflow::engine;

/**
 * @brief Base class for trading strategies.
 */
class Strategy {
public:
    virtual ~Strategy() = default;

    /**
     * @brief Attach a strategy context.
     * @param ctx Context pointer.
     */
    void set_context(StrategyContext* ctx) { ctx_ = ctx; }
    /**
     * @brief Get the current context.
     */
    StrategyContext* context() const { return ctx_; }

    /**
     * @brief Initialize the strategy with a context.
     */
    virtual void initialize(StrategyContext& ctx) = 0;
    /**
     * @brief Called when the strategy starts.
     */
    virtual void on_start() {}
    /**
     * @brief Called when the strategy stops.
     */
    virtual void on_stop() {}

    /**
     * @brief Handle a bar event.
     */
    virtual void on_bar([[maybe_unused]] const data::Bar& bar) {}
    /**
     * @brief Handle a tick event.
     */
    virtual void on_tick([[maybe_unused]] const data::Tick& tick) {}
    /**
     * @brief Handle a quote event.
     */
    virtual void on_quote([[maybe_unused]] const data::Quote& quote) {}
    /**
     * @brief Handle an order book event.
     */
    virtual void on_order_book([[maybe_unused]] const data::OrderBook& book) {}

    /**
     * @brief Handle an order update.
     */
    virtual void on_order_update([[maybe_unused]] const engine::Order& order) {}
    /**
     * @brief Handle a fill update.
     */
    virtual void on_fill([[maybe_unused]] const engine::Fill& fill) {}

    /**
     * @brief Handle a regime change event.
     */
    virtual void on_regime_change([[maybe_unused]] const regime::RegimeTransition& transition) {}

    /**
     * @brief Handle end-of-day event.
     */
    virtual void on_end_of_day([[maybe_unused]] const Timestamp& date) {}
    /**
     * @brief Handle timer event.
     */
    virtual void on_timer([[maybe_unused]] const std::string& timer_id) {}

protected:
    StrategyContext* ctx_ = nullptr;
};

}  // namespace regimeflow::strategy
