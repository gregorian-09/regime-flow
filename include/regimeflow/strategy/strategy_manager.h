#pragma once

#include "regimeflow/strategy/strategy.h"

#include <memory>
#include <vector>

namespace regimeflow::strategy {

namespace engine = ::regimeflow::engine;

class StrategyManager {
public:
    void add_strategy(std::unique_ptr<Strategy> strategy);
    void clear();

    void initialize(StrategyContext& ctx);
    void start();
    void stop();

    void on_bar(const data::Bar& bar);
    void on_tick(const data::Tick& tick);
    void on_quote(const data::Quote& quote);
    void on_order_book(const data::OrderBook& book);
    void on_order_update(const engine::Order& order);
    void on_fill(const engine::Fill& fill);
    void on_regime_change(const regime::RegimeTransition& transition);
    void on_timer(const std::string& timer_id);

private:
    std::vector<std::unique_ptr<Strategy>> strategies_;
};

}  // namespace regimeflow::strategy
