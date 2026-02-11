#include <gtest/gtest.h>

#include "regimeflow/engine/regime_tracker.h"
#include "regimeflow/regime/constant_detector.h"

namespace regimeflow::test {

TEST(RegimeTracker, MaintainsHistoryRingBuffer) {
    auto detector = std::make_unique<regimeflow::regime::ConstantRegimeDetector>(
        regimeflow::regime::RegimeType::Bull);
    regimeflow::engine::RegimeTracker tracker(std::move(detector));
    tracker.set_history_size(1);

    regimeflow::data::Bar bar1;
    bar1.timestamp = regimeflow::Timestamp(100);
    bar1.symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    regimeflow::data::Bar bar2 = bar1;
    bar2.timestamp = regimeflow::Timestamp(200);

    tracker.on_bar(bar1);
    tracker.on_bar(bar2);

    const auto& history = tracker.history();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history.back().timestamp.microseconds(), 200);
}

}  // namespace regimeflow::test
