// Example C++ driver to run a custom regime + strategy backtest.

#include "regimeflow/common/yaml_config.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/engine/backtest_runner.h"
#include "regimeflow/engine/engine_factory.h"
#include "regimeflow/metrics/report.h"
#include "regimeflow/metrics/report_writer.h"
#include "regimeflow/plugins/registry.h"
#include "regimeflow/strategy/strategy_factory.h"

#include <filesystem>
#include <iostream>
#include <string>

using namespace regimeflow;

namespace {

data::BarType parse_bar_type(const std::string& value) {
    if (value == "1m") return data::BarType::Time_1Min;
    if (value == "5m") return data::BarType::Time_5Min;
    if (value == "15m") return data::BarType::Time_15Min;
    if (value == "30m") return data::BarType::Time_30Min;
    if (value == "1h") return data::BarType::Time_1Hour;
    if (value == "4h") return data::BarType::Time_4Hour;
    return data::BarType::Time_1Day;
}

engine::BacktestRunSpec load_spec(const std::string& path) {
    auto config = YamlConfigLoader::load_file(path);
    engine::BacktestRunSpec spec;

    if (auto engine_cfg = config.get_as<ConfigValue::Object>("engine")) {
        spec.engine_config = Config(*engine_cfg);
    }
    if (auto exec_cfg = config.get_as<ConfigValue::Object>("execution")) {
        spec.engine_config.set("execution", ConfigValue(*exec_cfg));
    }
    if (auto risk_cfg = config.get_as<ConfigValue::Object>("risk")) {
        spec.engine_config.set("risk", ConfigValue(*risk_cfg));
    }
    if (auto regime_cfg = config.get_as<ConfigValue::Object>("regime")) {
        spec.engine_config.set("regime", ConfigValue(*regime_cfg));
    }
    if (auto plugins_cfg = config.get_as<ConfigValue::Object>("plugins")) {
        spec.engine_config.set("plugins", ConfigValue(*plugins_cfg));
    }

    if (auto data_cfg = config.get_as<ConfigValue::Object>("data")) {
        spec.data_config = Config(*data_cfg);
    }
    if (auto strategy_cfg = config.get_as<ConfigValue::Object>("strategy")) {
        spec.strategy_config = Config(*strategy_cfg);
    }
    if (auto symbols = config.get_as<ConfigValue::Array>("symbols")) {
        for (const auto& entry : *symbols) {
            if (const auto* s = entry.get_if<std::string>()) {
                spec.symbols.push_back(*s);
            }
        }
    }

    if (auto start = config.get_as<std::string>("start_date")) {
        spec.range.start = Timestamp::from_string(*start, "%Y-%m-%d");
    }
    if (auto end = config.get_as<std::string>("end_date")) {
        spec.range.end = Timestamp::from_string(*end, "%Y-%m-%d");
    }
    if (auto bar_type = config.get_as<std::string>("bar_type")) {
        spec.bar_type = parse_bar_type(*bar_type);
    }

    return spec;
}

void load_plugins(const Config& config) {
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
                                    auto res = registry.load_dynamic_plugin(full.string());
                                    if (res.is_err()
                                        && res.error().code != regimeflow::Error::Code::AlreadyExists) {
                                        throw std::runtime_error(res.error().to_string());
                                    }
                                    loaded = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!loaded) {
                        auto res = registry.load_dynamic_plugin(candidate.string());
                        if (res.is_err()
                            && res.error().code != regimeflow::Error::Code::AlreadyExists) {
                            throw std::runtime_error(res.error().to_string());
                        }
                    }
                } else {
                    auto res = registry.load_dynamic_plugin(candidate.string());
                    if (res.is_err()
                        && res.error().code != regimeflow::Error::Code::AlreadyExists) {
                        throw std::runtime_error(res.error().to_string());
                    }
                }
            }
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string config_path = "examples/custom_regime_ensemble/config.yaml";
    if (argc > 2 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    }

    try {
        auto spec = load_spec(config_path);
        load_plugins(spec.engine_config);
        auto results = engine::BacktestRunner::run_parallel({spec});
        if (!results.empty()) {
            auto report = metrics::build_report(results[0].metrics, results[0].fills, 0.0);
            std::cout << metrics::ReportWriter::to_json(report) << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
