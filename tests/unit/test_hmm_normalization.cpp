#include <gtest/gtest.h>

#include "regimeflow/regime/features.h"

namespace {

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
    EXPECT_NEAR(v[0], 0.0, 2.0);
}

}  // namespace
