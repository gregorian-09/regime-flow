#include "regimeflow/common/memory.h"

#include <cstdint>

#include <gtest/gtest.h>

using regimeflow::common::MonotonicArena;

TEST(MonotonicArena, PreservesAlignmentAfterBlockOverflow) {
    MonotonicArena arena(32);

    (void)arena.allocate(24, 8);
    void* ptr = arena.allocate(8, 64);

    const auto address = reinterpret_cast<std::uintptr_t>(ptr);
    EXPECT_EQ(address % 64, 0U);
}

TEST(MonotonicArena, RoundsNonPowerOfTwoAlignment) {
    MonotonicArena arena(64);

    void* ptr = arena.allocate(8, 24);

    const auto address = reinterpret_cast<std::uintptr_t>(ptr);
    EXPECT_EQ(address % 32, 0U);
}
