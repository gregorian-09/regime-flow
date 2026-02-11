#include <gtest/gtest.h>

#include "regimeflow/regime/ensemble.h"
#include "regimeflow/regime/constant_detector.h"

namespace regimeflow::test {

TEST(RegimeEnsemble, WeightedAverageChoosesHighestWeight) {
    regimeflow::regime::EnsembleRegimeDetector ensemble;
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
                              regimeflow::regime::RegimeType::Bull),
                          2.0);
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
                              regimeflow::regime::RegimeType::Bear),
                          1.0);

    regimeflow::data::Bar bar{};
    bar.timestamp = regimeflow::Timestamp(100);
    auto state = ensemble.on_bar(bar);
    EXPECT_EQ(state.regime, regimeflow::regime::RegimeType::Bull);
}

TEST(RegimeEnsemble, MajorityVotingChoosesMajority) {
    regimeflow::regime::EnsembleRegimeDetector ensemble;
    ensemble.set_voting_method(
        regimeflow::regime::EnsembleRegimeDetector::VotingMethod::Majority);
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
        regimeflow::regime::RegimeType::Bull));
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
        regimeflow::regime::RegimeType::Bull));
    ensemble.add_detector(std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
        regimeflow::regime::RegimeType::Bear));

    regimeflow::data::Bar bar{};
    bar.timestamp = regimeflow::Timestamp(200);
    auto state = ensemble.on_bar(bar);
    EXPECT_EQ(state.regime, regimeflow::regime::RegimeType::Bull);
}

}  // namespace regimeflow::test
