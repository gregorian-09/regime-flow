#include "regimeflow/engine/engine_factory.h"

#include "regimeflow/plugins/registry.h"

#include <filesystem>

namespace regimeflow::engine {

std::unique_ptr<BacktestEngine> EngineFactory::create(const Config& config) {
    double initial_capital = config.get_as<double>("engine.initial_capital").value_or(0.0);
    auto currency = config.get_as<std::string>("engine.currency").value_or("USD");

    auto& registry = plugins::PluginRegistry::instance();
    if (auto search_paths = config.get_as<ConfigValue::Array>("plugins.search_paths")) {
        for (const auto& value : *search_paths) {
            if (const auto* path = value.get_if<std::string>()) {
                registry.scan_plugin_directory(*path);
            }
        }
    }
    if (auto load_paths = config.get_as<ConfigValue::Array>("plugins.load")) {
        for (const auto& value : *load_paths) {
            if (const auto* entry = value.get_if<std::string>()) {
                std::filesystem::path candidate(*entry);
                if (candidate.is_relative()) {
                    bool loaded = false;
                    if (auto search_paths = config.get_as<ConfigValue::Array>("plugins.search_paths")) {
                        for (const auto& search : *search_paths) {
                            if (const auto* base = search.get_if<std::string>()) {
                                auto full = std::filesystem::path(*base) / candidate;
                                if (std::filesystem::exists(full)) {
                                    registry.load_dynamic_plugin(full.string());
                                    loaded = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!loaded) {
                        registry.load_dynamic_plugin(candidate.string());
                    }
                } else {
                    registry.load_dynamic_plugin(candidate.string());
                }
            }
        }
    }

    auto engine = std::make_unique<BacktestEngine>(initial_capital, currency);
    if (auto audit_path = config.get_as<std::string>("engine.audit_log_path")) {
        if (!audit_path->empty()) {
            engine->set_audit_log_path(*audit_path);
        }
    }

    if (auto exec_cfg = config.get_as<ConfigValue::Object>("execution")) {
        engine->configure_execution(Config(*exec_cfg));
    }
    if (auto risk_cfg = config.get_as<ConfigValue::Object>("risk")) {
        engine->configure_risk(Config(*risk_cfg));
    }
    if (auto regime_cfg = config.get_as<ConfigValue::Object>("regime")) {
        engine->configure_regime(Config(*regime_cfg));
    }

    if (auto strat_cfg = config.get_as<ConfigValue::Object>("strategy")) {
        Config strat_config(*strat_cfg);
        auto strategy = strategy::StrategyFactory::instance().create(strat_config);
        if (strategy) {
            engine->set_strategy(std::move(strategy), std::move(strat_config));
        }
    }

    return engine;
}

}  // namespace regimeflow::engine
