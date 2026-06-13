#include <gtest/gtest.h>

#include "regimeflow/plugins/registry.h"

#include <filesystem>

namespace regimeflow::test
{
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
        fs::path plugin_path =
#if defined(REGIMEFLOW_TEST_PLUGIN_DIR)
            fs::path(REGIMEFLOW_TEST_PLUGIN_DIR) / plugin_name;
#else
            fs::current_path() / "tests" / "plugins" / plugin_name;
#endif

        ASSERT_TRUE(fs::exists(plugin_path)) << "Missing plugin " << plugin_path;

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
