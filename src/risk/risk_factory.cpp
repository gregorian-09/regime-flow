#include "regimeflow/risk/risk_factory.h"

#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/registry.h"

namespace regimeflow::risk {

RiskManager RiskFactory::create_risk_manager(const Config& config) {
    if (auto type = config.get_as<std::string>("type")) {
        Config plugin_cfg = config;
        if (auto params = config.get_as<ConfigValue::Object>("params")) {
            plugin_cfg = Config(*params);
        }
        auto plugin = plugins::PluginRegistry::instance()
            .create<plugins::RiskManagerPlugin>("risk_manager", *type, plugin_cfg);
        if (plugin) {
            if (auto manager = plugin->create_risk_manager()) {
                return std::move(*manager);
            }
        }
    }
    RiskManager manager;

    if (auto max_notional = config.get_as<double>("limits.max_notional")) {
        manager.add_limit(std::make_unique<MaxNotionalLimit>(*max_notional));
    }
    if (auto max_position = config.get_as<double>("limits.max_position")) {
        manager.add_limit(std::make_unique<MaxPositionLimit>(static_cast<Quantity>(*max_position)));
    }
    if (auto max_position_pct = config.get_as<double>("limits.max_position_pct")) {
        manager.add_limit(std::make_unique<MaxPositionPctLimit>(*max_position_pct));
    }
    if (auto max_drawdown = config.get_as<double>("limits.max_drawdown")) {
        manager.add_limit(std::make_unique<MaxDrawdownLimit>(*max_drawdown));
    }
    if (auto max_gross = config.get_as<double>("limits.max_gross_exposure")) {
        manager.add_limit(std::make_unique<MaxGrossExposureLimit>(*max_gross));
    }
    if (auto max_net = config.get_as<double>("limits.max_net_exposure")) {
        manager.add_limit(std::make_unique<MaxNetExposureLimit>(*max_net));
    }
    if (auto max_leverage = config.get_as<double>("limits.max_leverage")) {
        manager.add_limit(std::make_unique<MaxLeverageLimit>(*max_leverage));
    }
    if (auto sector_limits = config.get_as<ConfigValue::Object>("limits.sector_limits")) {
        std::unordered_map<std::string, double> limits;
        for (const auto& [key, val] : *sector_limits) {
            if (auto v = val.get_if<double>()) {
                limits[key] = *v;
            }
        }
        std::unordered_map<std::string, std::string> symbol_map;
        if (auto map = config.get_as<ConfigValue::Object>("limits.sector_map")) {
            for (const auto& [key, val] : *map) {
                if (auto v = val.get_if<std::string>()) {
                    symbol_map[key] = *v;
                }
            }
        }
        if (!limits.empty()) {
            manager.add_limit(std::make_unique<MaxSectorExposureLimit>(limits, symbol_map));
        }
    }
    if (auto industry_limits = config.get_as<ConfigValue::Object>("limits.industry_limits")) {
        std::unordered_map<std::string, double> limits;
        for (const auto& [key, val] : *industry_limits) {
            if (auto v = val.get_if<double>()) {
                limits[key] = *v;
            }
        }
        std::unordered_map<std::string, std::string> symbol_map;
        if (auto map = config.get_as<ConfigValue::Object>("limits.industry_map")) {
            for (const auto& [key, val] : *map) {
                if (auto v = val.get_if<std::string>()) {
                    symbol_map[key] = *v;
                }
            }
        }
        if (!limits.empty()) {
            manager.add_limit(std::make_unique<MaxIndustryExposureLimit>(limits, symbol_map));
        }
    }
    if (auto corr_obj = config.get_as<ConfigValue::Object>("limits.correlation")) {
        MaxCorrelationExposureLimit::Config cfg;
        if (auto v = config.get_as<int64_t>("limits.correlation.window")) {
            cfg.window = static_cast<size_t>(*v);
        }
        if (auto v = config.get_as<double>("limits.correlation.max_corr")) {
            cfg.max_corr = *v;
        }
        if (auto v = config.get_as<double>("limits.correlation.max_pair_exposure_pct")) {
            cfg.max_pair_exposure_pct = *v;
        }
        manager.add_limit(std::make_unique<MaxCorrelationExposureLimit>(cfg));
    }

    auto regime_limits_obj = config.get_as<ConfigValue::Object>("limits_by_regime");
    if (regime_limits_obj) {
        std::unordered_map<std::string, std::vector<std::unique_ptr<RiskLimit>>> regime_limits;
        for (const auto& [regime_name, value] : *regime_limits_obj) {
            const auto* regime_cfg = value.get_if<ConfigValue::Object>();
            if (!regime_cfg) {
                continue;
            }
            Config reg_config(*regime_cfg);
            if (auto params = reg_config.get_as<ConfigValue::Object>("params")) {
                reg_config = Config(*params);
            }
            std::vector<std::unique_ptr<RiskLimit>> limits;
            if (auto max_notional = reg_config.get_as<double>("limits.max_notional")) {
                limits.push_back(std::make_unique<MaxNotionalLimit>(*max_notional));
            }
            if (auto max_position = reg_config.get_as<double>("limits.max_position")) {
                limits.push_back(std::make_unique<MaxPositionLimit>(static_cast<Quantity>(*max_position)));
            }
            if (auto max_position_pct = reg_config.get_as<double>("limits.max_position_pct")) {
                limits.push_back(std::make_unique<MaxPositionPctLimit>(*max_position_pct));
            }
            if (auto max_drawdown = reg_config.get_as<double>("limits.max_drawdown")) {
                limits.push_back(std::make_unique<MaxDrawdownLimit>(*max_drawdown));
            }
            if (auto max_gross = reg_config.get_as<double>("limits.max_gross_exposure")) {
                limits.push_back(std::make_unique<MaxGrossExposureLimit>(*max_gross));
            }
            if (auto max_net = reg_config.get_as<double>("limits.max_net_exposure")) {
                limits.push_back(std::make_unique<MaxNetExposureLimit>(*max_net));
            }
            if (auto max_leverage = reg_config.get_as<double>("limits.max_leverage")) {
                limits.push_back(std::make_unique<MaxLeverageLimit>(*max_leverage));
            }
            if (!limits.empty()) {
                regime_limits[regime_name] = std::move(limits);
            }
        }
        if (!regime_limits.empty()) {
            manager.set_regime_limits(std::move(regime_limits));
        }
    }

    return manager;
}

}  // namespace regimeflow::risk
