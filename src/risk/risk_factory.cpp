#include "regimeflow/risk/risk_factory.h"

#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/registry.h"

#include <optional>
#include <unordered_map>

namespace regimeflow::risk
{

    namespace {
        std::optional<double> object_double(const ConfigValue::Object& object, const std::string& key) {
            const auto it = object.find(key);
            if (it == object.end()) {
                return std::nullopt;
            }
            if (const auto* value = it->second.get_if<double>()) {
                return *value;
            }
            if (const auto* value = it->second.get_if<int64_t>()) {
                return static_cast<double>(*value);
            }
            return std::nullopt;
        }

        std::optional<bool> object_bool(const ConfigValue::Object& object, const std::string& key) {
            const auto it = object.find(key);
            if (it == object.end()) {
                return std::nullopt;
            }
            if (const auto* value = it->second.get_if<bool>()) {
                return *value;
            }
            return std::nullopt;
        }

        std::optional<std::unordered_map<std::string, RegimeRiskOverlayProfile>> parse_regime_overlays(
            const ConfigValue::Object& overlays) {
            std::unordered_map<std::string, RegimeRiskOverlayProfile> profiles;
            for (const auto& [name, value] : overlays) {
                const auto* object = value.get_if<ConfigValue::Object>();
                if (object == nullptr) {
                    continue;
                }
                RegimeRiskOverlayProfile profile;
                if (auto allow = object_bool(*object, "allow_new_exposure")) {
                    profile.allow_new_exposure = *allow;
                }
                if (auto max_notional = object_double(*object, "max_order_notional")) {
                    profile.max_order_notional = *max_notional;
                }
                if (auto max_position_pct = object_double(*object, "max_position_pct")) {
                    profile.max_position_pct = *max_position_pct;
                }
                if (auto allow_market_orders = object_bool(*object, "allow_market_orders")) {
                    profile.allow_market_orders = *allow_market_orders;
                }
                if (auto allow_aggressive_tif = object_bool(*object, "allow_aggressive_tif")) {
                    profile.allow_aggressive_tif = *allow_aggressive_tif;
                }
                profiles.emplace(name, profile);
            }
            if (profiles.empty()) {
                return std::nullopt;
            }
            return profiles;
        }
    }  // namespace

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

        if (auto overlays = config.get_as<ConfigValue::Object>("regime_overlays")) {
            if (auto profiles = parse_regime_overlays(*overlays)) {
                manager.add_limit(std::make_unique<RegimeRiskOverlayLimit>(std::move(*profiles)));
            }
        } else if (auto overlays = config.get_as<ConfigValue::Object>("limits.regime_overlays")) {
            if (auto profiles = parse_regime_overlays(*overlays)) {
                manager.add_limit(std::make_unique<RegimeRiskOverlayLimit>(std::move(*profiles)));
            }
        }

        if (auto regime_limits_obj = config.get_as<ConfigValue::Object>("limits_by_regime")) {
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
