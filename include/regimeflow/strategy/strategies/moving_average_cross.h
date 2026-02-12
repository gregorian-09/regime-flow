/**
 * @file moving_average_cross.h
 * @brief RegimeFlow regimeflow moving average cross declarations.
 */

#pragma once

#include "regimeflow/strategy/strategy.h"

#include <unordered_map>
#include <vector>

namespace regimeflow::strategy {

/**
 * @brief Moving average crossover strategy.
 */
class MovingAverageCrossStrategy final : public Strategy {
public:
    /**
     * @brief Initialize the strategy with context.
     */
    void initialize(StrategyContext& ctx) override;
    /**
     * @brief Generate signals on each bar.
     */
    void on_bar(const data::Bar& bar) override;

private:
    double compute_sma(SymbolId symbol, int period) const;

    int fast_period_ = 10;
    int slow_period_ = 30;
    double quantity_ = 0.0;
    std::unordered_map<SymbolId, std::vector<double>> price_history_;
};

}  // namespace regimeflow::strategy
