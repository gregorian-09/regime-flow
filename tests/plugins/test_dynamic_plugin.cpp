#include "regimeflow/plugins/plugin.h"
#include "regimeflow/common/result.h"

class DynamicTestPlugin : public regimeflow::plugins::Plugin {
public:
    regimeflow::plugins::PluginInfo info() const override {
        return {"dynamic_test", "1.0", "dynamic", "tests", {}};
    }

    regimeflow::Result<void> on_load() override {
        set_state(regimeflow::plugins::PluginState::Loaded);
        return regimeflow::Ok();
    }

    regimeflow::Result<void> on_initialize(const regimeflow::Config&) override {
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
};

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new DynamicTestPlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "strategy";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "dynamic_test";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
