/**
 * @file pairs_trading.h
 * @brief RegimeFlow regimeflow pairs trading strategy declarations.
 */

#pragma once

#include "regimeflow/strategy/strategy.h"
#include "regimeflow/common/types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace regimeflow::strategy {

/**
 * @brief Mean-reversion pairs trading strategy using z-score of spread.
 */
class PairsTradingStrategy final : public Strategy {
public:
    void initialize(StrategyContext& ctx) override;
    void on_bar(const data::Bar& bar) override;

private:
    bool load_symbols_from_config();
    bool compute_spread(double& spread, double& hedge_ratio) const;
    void submit_spread_trade(double hedge_ratio, double zscore, double price_a, double price_b);
    Quantity scaled_qty(double zscore) const;

    SymbolId symbol_a_id_ = 0;
    SymbolId symbol_b_id_ = 0;
    std::string symbol_a_;
    std::string symbol_b_;

    size_t lookback_ = 120;
    double entry_z_ = 2.0;
    double exit_z_ = 0.5;
    double max_z_ = 4.0;
    bool allow_short_ = true;
    Quantity base_qty_ = 10;
    double min_qty_scale_ = 0.5;
    double max_qty_scale_ = 2.0;
    size_t cooldown_bars_ = 5;
    size_t last_signal_index_ = 0;
};

}  // namespace regimeflow::strategy
