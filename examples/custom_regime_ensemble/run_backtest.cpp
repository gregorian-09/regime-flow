// Example C++ driver to run a custom regime + strategy backtest.

#include "regimeflow/common/yaml_config.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/engine/backtest_runner.h"
#include "regimeflow/engine/engine_factory.h"
#include "regimeflow/strategy/strategy_factory.h"

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

}  // namespace

int main(int argc, char** argv) {
    std::string config_path = "examples/custom_regime_ensemble/config.yaml";
    if (argc > 2 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    }

    try {
        auto spec = load_spec(config_path);
        auto results = engine::BacktestRunner::run_parallel({spec});
        if (!results.empty()) {
            std::cout << results[0].report_json() << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
