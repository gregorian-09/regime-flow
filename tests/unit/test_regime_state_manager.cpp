#include <gtest/gtest.h>

#include "regimeflow/regime/state_manager.h"

namespace regimeflow::test {

TEST(RegimeStateManager, TracksTransitionsAndStats) {
    regimeflow::regime::RegimeStateManager manager(10);

    regimeflow::regime::RegimeState s1;
    s1.regime = regimeflow::regime::RegimeType::Bull;
    s1.timestamp = regimeflow::Timestamp(0);
    manager.update(s1);

    regimeflow::regime::RegimeState s2 = s1;
    s2.regime = regimeflow::regime::RegimeType::Bear;
    s2.timestamp = regimeflow::Timestamp(10'000'000);
    manager.update(s2);

    auto transitions = manager.recent_transitions(10);
    ASSERT_EQ(transitions.size(), 1u);
    EXPECT_EQ(transitions[0].from, regimeflow::regime::RegimeType::Bull);
    EXPECT_EQ(transitions[0].to, regimeflow::regime::RegimeType::Bear);
    EXPECT_EQ(transitions[0].duration_in_from, 10.0);

    auto freq = manager.regime_frequencies();
    EXPECT_GT(freq[regimeflow::regime::RegimeType::Bull], 0.0);
    EXPECT_GT(freq[regimeflow::regime::RegimeType::Bear], 0.0);

    auto matrix = manager.empirical_transition_matrix();
    ASSERT_EQ(matrix.size(), 4u);
    EXPECT_NEAR(matrix[0][2], 1.0, 1e-9);
}

}  // namespace regimeflow::test
