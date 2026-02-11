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

class Strategy {
public:
    virtual ~Strategy() = default;

    void set_context(StrategyContext* ctx) { ctx_ = ctx; }
    StrategyContext* context() const { return ctx_; }

    virtual void initialize(StrategyContext& ctx) = 0;
    virtual void on_start() {}
    virtual void on_stop() {}

    virtual void on_bar([[maybe_unused]] const data::Bar& bar) {}
    virtual void on_tick([[maybe_unused]] const data::Tick& tick) {}
    virtual void on_quote([[maybe_unused]] const data::Quote& quote) {}
    virtual void on_order_book([[maybe_unused]] const data::OrderBook& book) {}

    virtual void on_order_update([[maybe_unused]] const engine::Order& order) {}
    virtual void on_fill([[maybe_unused]] const engine::Fill& fill) {}

    virtual void on_regime_change([[maybe_unused]] const regime::RegimeTransition& transition) {}

    virtual void on_end_of_day([[maybe_unused]] const Timestamp& date) {}
    virtual void on_timer([[maybe_unused]] const std::string& timer_id) {}

protected:
    StrategyContext* ctx_ = nullptr;
};

}  // namespace regimeflow::strategy
