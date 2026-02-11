#pragma once

#include "regimeflow/strategy/strategy.h"

namespace regimeflow::strategy {

class BuyAndHoldStrategy final : public Strategy {
public:
    void initialize(StrategyContext& ctx) override;
    void on_bar(const data::Bar& bar) override;

private:
    bool entered_ = false;
    SymbolId symbol_ = 0;
    double quantity_ = 0.0;
};

}  // namespace regimeflow::strategy
