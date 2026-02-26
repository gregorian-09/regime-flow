#include "regimeflow/strategy/strategy_factory.h"

#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/registry.h"

namespace regimeflow::strategy
{
    StrategyFactory& StrategyFactory::instance() {
        static StrategyFactory factory;
        register_builtin_strategies();
        return factory;
    }

    void StrategyFactory::register_creator(std::string name, Creator creator) {
        creators_[std::move(name)] = std::move(creator);
    }

    std::unique_ptr<Strategy> StrategyFactory::create(const Config& config) const {
        const auto type = config.get_as<std::string>("name")
            .value_or(config.get_as<std::string>("type").value_or(""));
        const auto it = creators_.find(type);
        if (it == creators_.end()) {
            Config plugin_cfg = config;
            if (const auto params = config.get_as<ConfigValue::Object>("params")) {
                plugin_cfg = Config(*params);
            }
            const auto plugin = plugins::PluginRegistry::instance()
                .create<plugins::StrategyPlugin>("strategy", type, plugin_cfg);
            if (plugin) {
                return plugin->create_strategy();
            }
            return nullptr;
        }
        return it->second(config);
    }
}  // namespace regimeflow::strategy
