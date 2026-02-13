// Example regime-aware strategy plugin.

#pragma once

#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/strategy/strategy.h"

namespace custom_regime {

class CustomRegimeStrategy final : public regimeflow::strategy::Strategy {
public:
    void initialize(regimeflow::strategy::StrategyContext& ctx) override;
    void on_bar(const regimeflow::data::Bar& bar) override;

private:
    regimeflow::SymbolId symbol_ = 0;
    int base_qty_ = 10;
    int trend_qty_ = 20;
    int stress_qty_ = 5;
};

class CustomRegimeStrategyPlugin final : public regimeflow::plugins::StrategyPlugin {
public:
    regimeflow::plugins::PluginInfo info() const override;
    regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override;
    std::unique_ptr<regimeflow::strategy::Strategy> create_strategy() override;

private:
    regimeflow::Config config_;
};

}  // namespace custom_regime
