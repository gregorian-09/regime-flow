// Example regime-aware strategy plugin.

#include "custom_regime_strategy.h"

#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/plugins/registry.h"

namespace custom_regime {

void CustomRegimeStrategy::initialize(regimeflow::strategy::StrategyContext& ctx) {
    ctx_ = &ctx;
    auto symbol = ctx.get_as<std::string>("symbol").value_or("AAPL");
    symbol_ = regimeflow::SymbolRegistry::instance().intern(symbol);
    base_qty_ = static_cast<int>(ctx.get_as<int64_t>("base_qty").value_or(10));
    trend_qty_ = static_cast<int>(ctx.get_as<int64_t>("trend_qty").value_or(20));
    stress_qty_ = static_cast<int>(ctx.get_as<int64_t>("stress_qty").value_or(5));
}

void CustomRegimeStrategy::on_bar(const regimeflow::data::Bar& bar) {
    if (!ctx_) return;
    if (bar.symbol != symbol_) return;

    const auto& regime = ctx_->current_regime();
    int qty = base_qty_;
    if (regime.regime == regimeflow::regime::RegimeType::Bull) {
        qty = trend_qty_;
    } else if (regime.regime == regimeflow::regime::RegimeType::Crisis) {
        qty = stress_qty_;
    }

    regimeflow::engine::Order order;
    order.symbol = symbol_;
    order.side = (regime.regime == regimeflow::regime::RegimeType::Bear)
        ? regimeflow::engine::OrderSide::Sell
        : regimeflow::engine::OrderSide::Buy;
    order.type = regimeflow::engine::OrderType::Market;
    order.quantity = qty;
    ctx_->submit_order(order);
}

regimeflow::plugins::PluginInfo CustomRegimeStrategyPlugin::info() const {
    return {"custom_regime_strategy", "0.1.0",
            "Regime-aware strategy with signal routing", "RegimeFlow", {}};
}

regimeflow::Result<void> CustomRegimeStrategyPlugin::on_initialize(
    const regimeflow::Config& config) {
    config_ = config;
    return regimeflow::Ok();
}

std::unique_ptr<regimeflow::strategy::Strategy>
CustomRegimeStrategyPlugin::create_strategy() {
    auto strategy = std::make_unique<CustomRegimeStrategy>();
    if (strategy->context()) {
        strategy->initialize(*strategy->context());
    }
    return strategy;
}

}  // namespace custom_regime

extern "C" {

regimeflow::plugins::REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new custom_regime::CustomRegimeStrategyPlugin();
}

regimeflow::plugins::REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

regimeflow::plugins::REGIMEFLOW_EXPORT const char* plugin_type() {
    return "strategy";
}

regimeflow::plugins::REGIMEFLOW_EXPORT const char* plugin_name() {
    return "custom_regime_strategy";
}

regimeflow::plugins::REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}

}  // extern "C"
