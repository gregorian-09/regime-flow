#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"

#include <memory>
#include <string>

namespace {
    class MetricsTemplate final : public regimeflow::metrics::PerformanceMetric {
    public:
        [[nodiscard]] std::string name() const override {
            return "template_total_return_pct";
        }

        [[nodiscard]] double compute(const regimeflow::metrics::EquityCurve& curve,
                                     double /*periods_per_year*/) const override {
            return curve.total_return() * 100.0;
        }
    };

    class MetricsTemplatePlugin final : public regimeflow::plugins::MetricsPlugin {
    public:
        [[nodiscard]] regimeflow::plugins::PluginInfo info() const override {
            return {"metrics_template", "0.1.0", "Minimal metrics plugin template", "RegimeFlow", {}};
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

        std::unique_ptr<regimeflow::metrics::PerformanceMetric> create_metric() override {
            (void)config_;
            return std::make_unique<MetricsTemplate>();
        }

    private:
        regimeflow::Config config_;
    };
}  // namespace

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new MetricsTemplatePlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "metrics";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "metrics_template";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
