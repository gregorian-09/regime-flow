#include <gtest/gtest.h>

#include "regimeflow/common/yaml_config.h"

#include <filesystem>
#include "regimeflow/engine/engine_factory.h"

namespace {

TEST(EngineFactoryTest, BuildsEngineFromConfig) {
    auto config = regimeflow::YamlConfigLoader::load_file(
        (std::filesystem::path(REGIMEFLOW_TEST_ROOT) /
         "tests/fixtures/config_hmm_ensemble.yaml").string());
    auto engine = regimeflow::engine::EngineFactory::create(config);
    ASSERT_NE(engine, nullptr);
}

}  // namespace
