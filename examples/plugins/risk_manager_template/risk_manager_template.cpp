#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"
#include "regimeflow/risk/risk_limits.h"

#include <memory>

namespace {
    class TemplateRiskLimit final : public regimeflow::risk::RiskLimit {
    public:
        explicit TemplateRiskLimit(double max_quantity) : max_quantity_(max_quantity) {}

        [[nodiscard]] regimeflow::Result<void> validate(
            const regimeflow::engine::Order& order,
            const regimeflow::engine::Portfolio&) const override {
            if (max_quantity_ > 0.0 && order.quantity > max_quantity_) {
                return regimeflow::Result<void>(regimeflow::Error(
                    regimeflow::Error::Code::OutOfRange,
                    "Template risk limit rejected oversized order"));
            }
            return regimeflow::Ok();
        }

    private:
        double max_quantity_ = 0.0;
    };

    class RiskManagerTemplatePlugin final : public regimeflow::plugins::RiskManagerPlugin {
    public:
        [[nodiscard]] regimeflow::plugins::PluginInfo info() const override {
            return {"risk_manager_template", "0.1.0", "Minimal risk manager plugin template", "RegimeFlow", {}};
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

        std::unique_ptr<regimeflow::risk::RiskManager> create_risk_manager() override {
            auto manager = std::make_unique<regimeflow::risk::RiskManager>();
            const double max_quantity = config_.get_as<double>("max_quantity").value_or(0.0);
            manager->add_limit(std::make_unique<TemplateRiskLimit>(max_quantity));
            return manager;
        }

    private:
        regimeflow::Config config_;
    };
}  // namespace

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new RiskManagerTemplatePlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "risk_manager";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "risk_manager_template";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
