#include <gtest/gtest.h>

#include "regimeflow/regime/features.h"
#include "regimeflow/regime/hmm.h"

#include <cmath>

namespace
{
    TEST(FeatureExtractorTest, NormalizesFeatures) {
        regimeflow::regime::FeatureExtractor extractor(5);
        extractor.set_features({regimeflow::regime::FeatureType::Return,
                                regimeflow::regime::FeatureType::Volatility});
        extractor.set_normalize(true);

        regimeflow::data::Bar bar{};
        bar.close = 100.0;
        extractor.on_bar(bar);

        bar.close = 102.0;
        auto v = extractor.on_bar(bar);
        ASSERT_EQ(v.size(), 2u);
        EXPECT_NEAR(v[0], std::sqrt(0.5), 1e-12);
        EXPECT_NEAR(v[1], std::sqrt(0.5), 1e-12);
    }

    TEST(HMMRegimeDetectorTest, ExtraHiddenStatesMapToCustomRegime) {
        regimeflow::regime::HMMRegimeDetector detector(5, 3);
        detector.set_transition_matrix({
            {0.0, 0.0, 0.0, 0.0, 1.0},
            {0.0, 0.0, 0.0, 0.0, 1.0},
            {0.0, 0.0, 0.0, 0.0, 1.0},
            {0.0, 0.0, 0.0, 0.0, 1.0},
            {0.0, 0.0, 0.0, 0.0, 1.0},
        });

        regimeflow::data::Bar bar{};
        bar.timestamp = regimeflow::Timestamp(1'000'000);
        bar.close = 100.0;

        const auto state = detector.on_bar(bar);
        EXPECT_EQ(state.regime, regimeflow::regime::RegimeType::Custom);
        ASSERT_EQ(state.probabilities_all.size(), 5u);
        EXPECT_GT(state.probabilities_all[4], 0.99);
    }
}  // namespace
