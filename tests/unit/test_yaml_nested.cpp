#include <gtest/gtest.h>

#include "regimeflow/common/yaml_config.h"

#include <filesystem>

namespace {

TEST(YamlConfigTest, ParsesNestedArrayObjects) {
    auto config = regimeflow::YamlConfigLoader::load_file(
        (std::filesystem::path(REGIMEFLOW_TEST_ROOT) /
         "tests/fixtures/config_hmm_ensemble.yaml").string());

    auto dets = config.get_as<regimeflow::ConfigValue::Array>("regime.ensemble.detectors");
    ASSERT_TRUE(dets.has_value());
    ASSERT_GE(dets->size(), 2u);

    const auto* first_obj = (*dets)[0].get_if<regimeflow::ConfigValue::Object>();
    ASSERT_TRUE(first_obj);
    auto type_it = first_obj->find("type");
    ASSERT_NE(type_it, first_obj->end());
    auto* type = type_it->second.get_if<std::string>();
    ASSERT_TRUE(type);
    EXPECT_EQ(*type, "hmm");

    auto hmm_it = first_obj->find("hmm");
    ASSERT_NE(hmm_it, first_obj->end());
    auto* hmm_obj = hmm_it->second.get_if<regimeflow::ConfigValue::Object>();
    ASSERT_TRUE(hmm_obj);
    auto states_it = hmm_obj->find("states");
    ASSERT_NE(states_it, hmm_obj->end());
    auto* states = states_it->second.get_if<int64_t>();
    ASSERT_TRUE(states);
    EXPECT_EQ(*states, 4);
}

}  // namespace
