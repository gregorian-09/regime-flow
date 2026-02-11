#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/portfolio.h"

#include <deque>
#include <unordered_map>

namespace regimeflow::risk {

struct StopLossConfig {
    bool enable_fixed = false;
    bool enable_trailing = false;
    bool enable_atr = false;
    bool enable_time = false;

    double stop_loss_pct = 0.05;
    double trailing_stop_pct = 0.03;

    int atr_window = 14;
    double atr_multiplier = 2.0;

    int64_t max_holding_seconds = 0;
};

class StopLossManager {
public:
    void configure(StopLossConfig config);
    void on_position_update(const engine::Position& position);
    void on_bar(const data::Bar& bar, engine::OrderManager& order_manager);
    void on_tick(const data::Tick& tick, engine::OrderManager& order_manager);

private:
    struct StopState {
        Quantity last_qty = 0;
        double entry_price = 0.0;
        Timestamp entry_time;
        double highest = 0.0;
        double lowest = 0.0;
        double last_atr = 0.0;
        double prev_close = 0.0;
        std::deque<double> true_ranges;
        bool exit_requested = false;
    };

    void update_from_price(SymbolId symbol, Price price, Timestamp ts,
                           engine::OrderManager& order_manager);
    void update_atr(SymbolId symbol, const data::Bar& bar);
    void maybe_exit(SymbolId symbol, Price price, Timestamp ts,
                    engine::OrderManager& order_manager);

    StopLossConfig config_;
    std::unordered_map<SymbolId, StopState> states_;
};

}  // namespace regimeflow::risk
