#include <gtest/gtest.h>

#include "regimeflow/common/yaml_config.h"

#include <filesystem>

namespace {

TEST(YamlConfigTest, LoadsSimpleConfig) {
    auto config = regimeflow::YamlConfigLoader::load_file(
        (std::filesystem::path(REGIMEFLOW_TEST_ROOT) /
         "tests/fixtures/config_hmm_ensemble.yaml").string());

    auto type = config.get_as<std::string>("strategy.type");
    ASSERT_TRUE(type.has_value());
    EXPECT_EQ(*type, "moving_average_cross");

    auto max_dd = config.get_as<double>("risk.limits.max_drawdown");
    ASSERT_TRUE(max_dd.has_value());
    EXPECT_DOUBLE_EQ(*max_dd, 0.2);
}

}  // namespace
