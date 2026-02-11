#include <gtest/gtest.h>

#include "regimeflow/plugins/registry.h"

namespace regimeflow::test {

class TestPlugin : public regimeflow::plugins::Plugin {
public:
    regimeflow::plugins::PluginInfo info() const override {
        return {"test_plugin", "1.0", "test", "unit", {}};
    }

    Result<void> on_load() override {
        set_state(regimeflow::plugins::PluginState::Loaded);
        return Ok();
    }

    Result<void> on_initialize(const Config&) override {
        set_state(regimeflow::plugins::PluginState::Initialized);
        return Ok();
    }

    Result<void> on_start() override {
        set_state(regimeflow::plugins::PluginState::Active);
        return Ok();
    }

    Result<void> on_stop() override {
        set_state(regimeflow::plugins::PluginState::Stopped);
        return Ok();
    }
};

TEST(PluginLifecycle, RegistryTransitionsState) {
    auto& registry = regimeflow::plugins::PluginRegistry::instance();
    registry.register_plugin<TestPlugin>("strategy", "test_plugin");

    auto plugin = registry.create<TestPlugin>("strategy", "test_plugin");
    ASSERT_TRUE(plugin);
    EXPECT_EQ(plugin->state(), regimeflow::plugins::PluginState::Initialized);

    auto start_res = registry.start_plugin(*plugin);
    ASSERT_TRUE(start_res.is_ok());
    EXPECT_EQ(plugin->state(), regimeflow::plugins::PluginState::Active);

    auto stop_res = registry.stop_plugin(*plugin);
    ASSERT_TRUE(stop_res.is_ok());
    EXPECT_EQ(plugin->state(), regimeflow::plugins::PluginState::Stopped);
}

}  // namespace regimeflow::test
