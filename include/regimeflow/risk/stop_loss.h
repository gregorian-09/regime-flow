/**
 * @file stop_loss.h
 * @brief RegimeFlow regimeflow stop loss declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/portfolio.h"

#include <deque>
#include <unordered_map>

namespace regimeflow::risk {

/**
 * @brief Configuration for stop-loss policies.
 */
struct StopLossConfig {
    /**
     * @brief Enable fixed percentage stop.
     */
    bool enable_fixed = false;
    /**
     * @brief Enable trailing stop.
     */
    bool enable_trailing = false;
    /**
     * @brief Enable ATR-based stop.
     */
    bool enable_atr = false;
    /**
     * @brief Enable time-based stop.
     */
    bool enable_time = false;

    /**
     * @brief Fixed stop-loss percent.
     */
    double stop_loss_pct = 0.05;
    /**
     * @brief Trailing stop percent.
     */
    double trailing_stop_pct = 0.03;

    /**
     * @brief ATR lookback window.
     */
    int atr_window = 14;
    /**
     * @brief ATR multiplier.
     */
    double atr_multiplier = 2.0;

    /**
     * @brief Max holding time in seconds (0 disables).
     */
    int64_t max_holding_seconds = 0;
};

/**
 * @brief Manages stop-loss logic for live/backtest positions.
 */
class StopLossManager {
public:
    /**
     * @brief Configure stop-loss settings.
     * @param config Stop-loss configuration.
     */
    void configure(StopLossConfig config);
    /**
     * @brief Update position state for stop-loss tracking.
     * @param position Updated position.
     */
    void on_position_update(const engine::Position& position);
    /**
     * @brief Process a bar update for stop-loss checks.
     * @param bar Bar data.
     * @param order_manager Order manager for exit orders.
     */
    void on_bar(const data::Bar& bar, engine::OrderManager& order_manager);
    /**
     * @brief Process a tick update for stop-loss checks.
     * @param tick Tick data.
     * @param order_manager Order manager for exit orders.
     */
    void on_tick(const data::Tick& tick, engine::OrderManager& order_manager);

private:
    /**
     * @brief Per-symbol stop-loss state.
     */
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
