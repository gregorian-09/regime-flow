#include <gtest/gtest.h>

#include "regimeflow/plugins/registry.h"

#include <filesystem>

namespace regimeflow::test {

TEST(PluginRegistry, LoadsDynamicPlugin) {
    namespace fs = std::filesystem;
    fs::path plugin_path;
    std::error_code ec;
    auto exe_path = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        auto candidate = exe_path.parent_path() / "plugins" / "libregimeflow_test_plugin.so";
        if (fs::exists(candidate)) {
            plugin_path = candidate;
        }
    }

    if (plugin_path.empty()) {
        fs::path base = fs::current_path();
        for (int i = 0; i < 6; ++i) {
            auto candidate = base / "build" / "tests" / "plugins" / "libregimeflow_test_plugin.so";
            if (fs::exists(candidate)) {
                plugin_path = candidate;
                break;
            }
            candidate = base / "tests" / "plugins" / "libregimeflow_test_plugin.so";
            if (fs::exists(candidate)) {
                plugin_path = candidate;
                break;
            }
            if (!base.has_parent_path()) {
                break;
            }
            base = base.parent_path();
        }
    }

    ASSERT_FALSE(plugin_path.empty()) << "Missing plugin libregimeflow_test_plugin.so";

    auto& registry = regimeflow::plugins::PluginRegistry::instance();
    auto load_res = registry.load_dynamic_plugin(plugin_path.string());
    ASSERT_TRUE(load_res.is_ok()) << load_res.error().to_string();

    auto plugin = registry.create<regimeflow::plugins::Plugin>("strategy", "dynamic_test");
    ASSERT_TRUE(plugin);
    EXPECT_EQ(plugin->info().name, "dynamic_test");

    plugin.reset();

    auto unload_res = registry.unload_dynamic_plugin("dynamic_test");
    EXPECT_TRUE(unload_res.is_ok());
}

}  // namespace regimeflow::test
