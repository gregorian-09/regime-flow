#pragma once

#include "regimeflow/strategy/strategy.h"

#include <unordered_map>
#include <vector>

namespace regimeflow::strategy {

class MovingAverageCrossStrategy final : public Strategy {
public:
    void initialize(StrategyContext& ctx) override;
    void on_bar(const data::Bar& bar) override;

private:
    double compute_sma(SymbolId symbol, int period) const;

    int fast_period_ = 10;
    int slow_period_ = 30;
    double quantity_ = 0.0;
    std::unordered_map<SymbolId, std::vector<double>> price_history_;
};

}  // namespace regimeflow::strategy
