#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"
#include "regimeflow/strategy/strategy.h"

#include <memory>

namespace {
    class StrategyTemplate final : public regimeflow::strategy::Strategy {
    public:
        void initialize(regimeflow::strategy::StrategyContext& ctx) override {
            set_context(&ctx);
        }

        void on_bar(const regimeflow::data::Bar& bar) override {
            (void)bar;
            // Add strategy logic here. Keep broker/risk assumptions outside the plugin.
        }
    };

    class StrategyTemplatePlugin final : public regimeflow::plugins::StrategyPlugin {
    public:
        [[nodiscard]] regimeflow::plugins::PluginInfo info() const override {
            return {"strategy_template", "0.1.0", "Minimal strategy plugin template", "RegimeFlow", {}};
        }

        regimeflow::Result<void> on_load() override {
            set_state(regimeflow::plugins::PluginState::Loaded);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override {
            config_ = config;
            set_state(regimeflow::plugins::PluginState::Initialized);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_start() override {
            set_state(regimeflow::plugins::PluginState::Active);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_stop() override {
            set_state(regimeflow::plugins::PluginState::Stopped);
            return regimeflow::Ok();
        }

        std::unique_ptr<regimeflow::strategy::Strategy> create_strategy() override {
            (void)config_;
            return std::make_unique<StrategyTemplate>();
        }

    private:
        regimeflow::Config config_;
    };
}  // namespace

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new StrategyTemplatePlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "strategy";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "strategy_template";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
