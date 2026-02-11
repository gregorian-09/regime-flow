#include <gtest/gtest.h>

#include "regimeflow/regime/features.h"

namespace {

TEST(FeatureExtractorTest, ComputesReturnAndVolatility) {
    regimeflow::regime::FeatureExtractor extractor(3);
    extractor.set_features({regimeflow::regime::FeatureType::Return,
                            regimeflow::regime::FeatureType::Volatility});

    regimeflow::data::Bar bar{};
    bar.close = 100.0;
    extractor.on_bar(bar);

    bar.close = 101.0;
    auto v1 = extractor.on_bar(bar);
    ASSERT_EQ(v1.size(), 2u);

    bar.close = 103.0;
    auto v2 = extractor.on_bar(bar);
    ASSERT_EQ(v2.size(), 2u);
    EXPECT_NEAR(v2[0], (103.0 - 101.0) / 101.0, 1e-9);
}

TEST(FeatureExtractorTest, ComputesZScores) {
    regimeflow::regime::FeatureExtractor extractor(3);
    extractor.set_features({regimeflow::regime::FeatureType::VolumeZScore,
                            regimeflow::regime::FeatureType::RangeZScore});

    regimeflow::data::Bar bar{};
    bar.high = 101.0;
    bar.low = 99.0;
    bar.volume = 100;
    extractor.on_bar(bar);

    bar.high = 102.0;
    bar.low = 100.0;
    bar.volume = 200;
    auto v = extractor.on_bar(bar);
    ASSERT_EQ(v.size(), 2u);
}

TEST(FeatureExtractorTest, ComputesVolumeRatiosAndObv) {
    regimeflow::regime::FeatureExtractor extractor(3);
    extractor.set_features({regimeflow::regime::FeatureType::VolumeRatio,
                            regimeflow::regime::FeatureType::VolatilityRatio,
                            regimeflow::regime::FeatureType::OnBalanceVolume,
                            regimeflow::regime::FeatureType::UpDownVolumeRatio});

    regimeflow::data::Bar bar{};
    bar.close = 100.0;
    bar.high = 101.0;
    bar.low = 99.0;
    bar.volume = 100;
    extractor.on_bar(bar);

    bar.close = 101.0;
    bar.high = 102.0;
    bar.low = 100.0;
    bar.volume = 200;
    auto v = extractor.on_bar(bar);
    ASSERT_EQ(v.size(), 4u);
    EXPECT_GT(v[0], 0.0);
    EXPECT_GE(v[3], 0.0);
}

TEST(FeatureExtractorTest, ComputesMicrostructureFeaturesFromBook) {
    regimeflow::regime::FeatureExtractor extractor(3);
    extractor.set_features({regimeflow::regime::FeatureType::BidAskSpread,
                            regimeflow::regime::FeatureType::OrderImbalance});

    regimeflow::data::OrderBook book{};
    book.bids[0].price = 99.0;
    book.asks[0].price = 101.0;
    book.bids[0].quantity = 80;
    book.asks[0].quantity = 20;

    auto v = extractor.on_book(book);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_GT(v[0], 0.0);
    EXPECT_GT(v[1], 0.0);
}

TEST(FeatureExtractorTest, EmitsCrossAssetFeatures) {
    regimeflow::regime::FeatureExtractor extractor(3);
    extractor.set_features({regimeflow::regime::FeatureType::MarketBreadth,
                            regimeflow::regime::FeatureType::SectorRotation,
                            regimeflow::regime::FeatureType::CorrelationEigen,
                            regimeflow::regime::FeatureType::RiskAppetite});
    extractor.update_cross_asset_features(0.6, 0.2, 1.5, -0.1);

    regimeflow::data::Bar bar{};
    bar.close = 100.0;
    auto v = extractor.on_bar(bar);
    ASSERT_EQ(v.size(), 4u);
    EXPECT_NEAR(v[0], 0.6, 1e-9);
    EXPECT_NEAR(v[1], 0.2, 1e-9);
    EXPECT_NEAR(v[2], 1.5, 1e-9);
    EXPECT_NEAR(v[3], -0.1, 1e-9);
}

}  // namespace
