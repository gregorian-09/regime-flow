#include <gtest/gtest.h>

#include "regimeflow/regime/hmm.h"
#include "temp_path_guard.h"

#include <filesystem>

namespace regimeflow::test
{
    TEST(HMMRegimeDetector, SaveLoadPreservesLikelihood) {
        regimeflow::regime::HMMRegimeDetector detector(3, 5);
        detector.set_features({regimeflow::regime::FeatureType::Return,
                               regimeflow::regime::FeatureType::Volatility});

        std::vector<regimeflow::regime::FeatureVector> data;
        data.reserve(20);
        for (int i = 0; i < 20; ++i) {
            double ret = 0.001 * (i % 3 - 1);
            double vol = 0.01 + 0.001 * (i % 5);
            data.push_back({ret, vol});
        }

        detector.train(data);
        double ll_before = detector.log_likelihood(data);

        regimeflow::test::TempPathGuard temp_model(
            std::filesystem::temp_directory_path() / "regimeflow_hmm_model.txt");
        const auto& path = temp_model.path();
        detector.save(path.string());

        regimeflow::regime::HMMRegimeDetector loaded(1, 5);
        loaded.load(path.string());
        double ll_after = loaded.log_likelihood(data);

        EXPECT_NEAR(ll_before, ll_after, 1e-3);
    }

    TEST(HMMRegimeDetector, SaveLoadPreservesModelGovernanceMetadata) {
        regimeflow::regime::HMMRegimeDetector detector(2, 5);
        detector.set_features({regimeflow::regime::FeatureType::Return,
                               regimeflow::regime::FeatureType::Volatility});

        regimeflow::regime::ModelGovernanceMetadata metadata;
        metadata.detector_type = "hmm";
        metadata.model_version = "test-model-7";
        metadata.training_start_us = 100;
        metadata.training_end_us = 200;
        metadata.feature_schema = "return,volatility";
        metadata.parameter_digest = "sha256:abc123";
        detector.set_model_metadata(metadata);

        regimeflow::test::TempPathGuard temp_model(
            std::filesystem::temp_directory_path() / "regimeflow_hmm_metadata.txt");
        detector.save(temp_model.path().string());

        regimeflow::regime::HMMRegimeDetector loaded(1, 5);
        loaded.load(temp_model.path().string());
        const auto loaded_metadata = loaded.model_metadata();

        EXPECT_EQ(loaded_metadata.detector_type, "hmm");
        EXPECT_EQ(loaded_metadata.model_version, "test-model-7");
        EXPECT_EQ(loaded_metadata.training_start_us, 100);
        EXPECT_EQ(loaded_metadata.training_end_us, 200);
        EXPECT_EQ(loaded_metadata.feature_schema, "return,volatility");
        EXPECT_EQ(loaded_metadata.parameter_digest, "sha256:abc123");
    }
}  // namespace regimeflow::test
