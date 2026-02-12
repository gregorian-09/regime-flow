/**
 * @file buy_and_hold.h
 * @brief RegimeFlow regimeflow buy and hold declarations.
 */

#pragma once

#include "regimeflow/strategy/strategy.h"

namespace regimeflow::strategy {

/**
 * @brief Simple buy-and-hold strategy.
 */
class BuyAndHoldStrategy final : public Strategy {
public:
    /**
     * @brief Initialize the strategy with context.
     */
    void initialize(StrategyContext& ctx) override;
    /**
     * @brief On first bar, enter a long position.
     */
    void on_bar(const data::Bar& bar) override;

private:
    bool entered_ = false;
    SymbolId symbol_ = 0;
    double quantity_ = 0.0;
};

}  // namespace regimeflow::strategy
