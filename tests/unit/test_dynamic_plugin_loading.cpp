#include <gtest/gtest.h>

#include "regimeflow/plugins/registry.h"

#include <filesystem>

namespace regimeflow::test {

TEST(PluginRegistry, LoadsDynamicPlugin) {
    namespace fs = std::filesystem;
    const char* plugin_ext =
#if defined(_WIN32)
        ".dll";
#elif defined(__APPLE__)
        ".dylib";
#else
        ".so";
#endif
    const std::string plugin_name =
#if defined(_WIN32)
        std::string("regimeflow_test_plugin") + plugin_ext;
#else
        std::string("libregimeflow_test_plugin") + plugin_ext;
#endif
    fs::path plugin_path;

    if (plugin_path.empty()) {
        fs::path base = fs::current_path();
        for (int i = 0; i < 6; ++i) {
            auto candidate = base / "build" / "tests" / "plugins" / plugin_name;
            if (fs::exists(candidate)) {
                plugin_path = candidate;
                break;
            }
            candidate = base / "build" / "bin" / plugin_name;
            if (fs::exists(candidate)) {
                plugin_path = candidate;
                break;
            }
            candidate = base / "tests" / "plugins" / plugin_name;
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

    ASSERT_FALSE(plugin_path.empty()) << "Missing plugin " << plugin_name;

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
