#include <gtest/gtest.h>

#include "regimeflow/regime/hmm.h"

#include <filesystem>

namespace regimeflow::test {

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

    auto path = std::filesystem::temp_directory_path() / "regimeflow_hmm_model.txt";
    detector.save(path.string());

    regimeflow::regime::HMMRegimeDetector loaded(1, 5);
    loaded.load(path.string());
    double ll_after = loaded.log_likelihood(data);

    EXPECT_NEAR(ll_before, ll_after, 1e-3);
}

}  // namespace regimeflow::test
