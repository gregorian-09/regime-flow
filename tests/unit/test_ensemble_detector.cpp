#include <gtest/gtest.h>

#include "regimeflow/regime/ensemble.h"
#include "regimeflow/regime/constant_detector.h"
#include "regimeflow/data/bar.h"

namespace {

TEST(EnsembleRegimeDetectorTest, ChoosesWeightedMajority) {
    regimeflow::regime::EnsembleRegimeDetector ensemble;
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
        regimeflow::regime::RegimeType::Bull), 2.0);
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
        regimeflow::regime::RegimeType::Bear), 1.0);

    regimeflow::data::Bar bar{};
    bar.timestamp = regimeflow::Timestamp::now();
    bar.close = 100.0;

    auto state = ensemble.on_bar(bar);
    EXPECT_EQ(state.regime, regimeflow::regime::RegimeType::Bull);
}

}  // namespace
