#include "regimeflow/execution/execution_factory.h"
#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/registry.h"

#include <algorithm>
#include <unordered_map>

namespace regimeflow::execution {

namespace {

Config plugin_params_or_full(const Config& config) {
    if (auto params = config.get_as<ConfigValue::Object>("params")) {
        return Config(*params);
    }
    return config;
}

}  // namespace

std::unique_ptr<ExecutionModel> ExecutionFactory::create_execution_model(const Config& config) {
    auto type = config.get_as<std::string>("model")
        .value_or(config.get_as<std::string>("type").value_or("basic"));
    if (type == "basic") {
        auto slippage = create_slippage_model(config);
        return std::make_unique<BasicExecutionModel>(std::move(slippage));
    }
    auto plugin_cfg = plugin_params_or_full(config);
    auto plugin = plugins::PluginRegistry::instance()
        .create<plugins::ExecutionModelPlugin>("execution_model", type, plugin_cfg);
    if (plugin) {
        auto slippage = plugin->create_slippage_model();
        if (slippage) {
            return std::make_unique<BasicExecutionModel>(std::move(slippage));
        }
    }
    return nullptr;
}

std::unique_ptr<SlippageModel> ExecutionFactory::create_slippage_model(const Config& config) {
    auto type = config.get_as<std::string>("slippage.type").value_or("zero");
    if (type == "zero") {
        return std::make_unique<ZeroSlippageModel>();
    }
    if (type == "fixed_bps") {
        double bps = config.get_as<double>("slippage.bps").value_or(0.0);
        return std::make_unique<FixedBpsSlippageModel>(bps);
    }
    if (type == "regime_bps") {
        double default_bps = config.get_as<double>("slippage.default_bps").value_or(0.0);
        std::unordered_map<regime::RegimeType, double> map;
        map[regime::RegimeType::Bull] = config.get_as<double>("slippage.regime_bps.bull").value_or(default_bps);
        map[regime::RegimeType::Neutral] = config.get_as<double>("slippage.regime_bps.neutral").value_or(default_bps);
        map[regime::RegimeType::Bear] = config.get_as<double>("slippage.regime_bps.bear").value_or(default_bps);
        map[regime::RegimeType::Crisis] = config.get_as<double>("slippage.regime_bps.crisis").value_or(default_bps);
        return std::make_unique<RegimeBpsSlippageModel>(default_bps, std::move(map));
    }
    auto model = config.get_as<std::string>("model")
        .value_or(config.get_as<std::string>("type").value_or(""));
    auto plugin_name = !model.empty() ? model : type;
    if (!plugin_name.empty()) {
        auto plugin_cfg = plugin_params_or_full(config);
        auto plugin = plugins::PluginRegistry::instance()
            .create<plugins::ExecutionModelPlugin>("execution_model", plugin_name, plugin_cfg);
        if (plugin) {
            if (auto slippage = plugin->create_slippage_model()) {
                return slippage;
            }
        }
    }
    return std::make_unique<ZeroSlippageModel>();
}

std::unique_ptr<CommissionModel> ExecutionFactory::create_commission_model(const Config& config) {
    auto type = config.get_as<std::string>("commission.type").value_or("zero");
    if (type == "zero") {
        return std::make_unique<ZeroCommissionModel>();
    }
    if (type == "fixed") {
        double amount = config.get_as<double>("commission.amount").value_or(0.0);
        return std::make_unique<FixedPerFillCommissionModel>(amount);
    }
    auto model = config.get_as<std::string>("model")
        .value_or(config.get_as<std::string>("type").value_or(""));
    auto plugin_name = !model.empty() ? model : type;
    if (!plugin_name.empty()) {
        auto plugin_cfg = plugin_params_or_full(config);
        auto plugin = plugins::PluginRegistry::instance()
            .create<plugins::ExecutionModelPlugin>("execution_model", plugin_name, plugin_cfg);
        if (plugin) {
            if (auto commission = plugin->create_commission_model()) {
                return commission;
            }
        }
    }
    return std::make_unique<ZeroCommissionModel>();
}

std::unique_ptr<TransactionCostModel> ExecutionFactory::create_transaction_cost_model(
    const Config& config) {
    auto type = config.get_as<std::string>("transaction_cost.type").value_or("zero");
    if (type == "zero") {
        return std::make_unique<ZeroTransactionCostModel>();
    }
    if (type == "fixed_bps") {
        double bps = config.get_as<double>("transaction_cost.bps").value_or(0.0);
        return std::make_unique<FixedBpsTransactionCostModel>(bps);
    }
    if (type == "per_share") {
        double rate = config.get_as<double>("transaction_cost.per_share").value_or(0.0);
        return std::make_unique<PerShareTransactionCostModel>(rate);
    }
    if (type == "per_order") {
        double cost = config.get_as<double>("transaction_cost.per_order").value_or(0.0);
        return std::make_unique<PerOrderTransactionCostModel>(cost);
    }
    if (type == "tiered") {
        std::vector<TieredTransactionCostTier> tiers;
        if (auto arr = config.get_as<ConfigValue::Array>("transaction_cost.tiers")) {
            tiers.reserve(arr->size());
            for (const auto& entry : *arr) {
                const auto* obj = entry.get_if<ConfigValue::Object>();
                if (!obj) {
                    continue;
                }
                auto it_bps = obj->find("bps");
                auto it_max = obj->find("max_notional");
                if (it_bps == obj->end()) {
                    continue;
                }
                TieredTransactionCostTier tier;
                if (const auto* bps = it_bps->second.get_if<double>()) {
                    tier.bps = *bps;
                } else if (const auto* bps_i = it_bps->second.get_if<int64_t>()) {
                    tier.bps = static_cast<double>(*bps_i);
                } else {
                    continue;
                }
                if (it_max != obj->end()) {
                    if (const auto* max_v = it_max->second.get_if<double>()) {
                        tier.max_notional = *max_v;
                    } else if (const auto* max_i = it_max->second.get_if<int64_t>()) {
                        tier.max_notional = static_cast<double>(*max_i);
                    }
                }
                tiers.push_back(tier);
            }
        }
        if (!tiers.empty()) {
            std::sort(tiers.begin(), tiers.end(),
                      [](const TieredTransactionCostTier& a,
                         const TieredTransactionCostTier& b) {
                          return a.max_notional < b.max_notional;
                      });
            return std::make_unique<TieredBpsTransactionCostModel>(std::move(tiers));
        }
        return std::make_unique<ZeroTransactionCostModel>();
    }
    return std::make_unique<ZeroTransactionCostModel>();
}

std::unique_ptr<MarketImpactModel> ExecutionFactory::create_market_impact_model(
    const Config& config) {
    auto type = config.get_as<std::string>("market_impact.type").value_or("zero");
    if (type == "zero") {
        return std::make_unique<ZeroMarketImpactModel>();
    }
    if (type == "fixed_bps") {
        double bps = config.get_as<double>("market_impact.bps").value_or(0.0);
        return std::make_unique<FixedMarketImpactModel>(bps);
    }
    if (type == "order_book") {
        double max_bps = config.get_as<double>("market_impact.max_bps").value_or(50.0);
        return std::make_unique<OrderBookImpactModel>(max_bps);
    }
    return std::make_unique<ZeroMarketImpactModel>();
}

std::unique_ptr<LatencyModel> ExecutionFactory::create_latency_model(const Config& config) {
    auto ms = config.get_as<int64_t>("latency.ms").value_or(0);
    return std::make_unique<FixedLatencyModel>(Duration::milliseconds(ms));
}

}  // namespace regimeflow::execution
