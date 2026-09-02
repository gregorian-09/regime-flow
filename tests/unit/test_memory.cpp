#include "regimeflow/common/memory.h"

#include <cstdint>

#include <gtest/gtest.h>

using regimeflow::common::MonotonicArena;
using regimeflow::common::PoolAllocator;

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

TEST(PoolAllocator, ReleasesFullyIdleExpansionAtLifecycleBoundary) {
    PoolAllocator<int> pool(2);
    int* first = pool.allocate();
    int* second = pool.allocate();
    int* third = pool.allocate();
    ASSERT_GT(pool.retained_capacity(), 2U);

    pool.deallocate(first);
    pool.deallocate(second);
    pool.deallocate(third);

    EXPECT_TRUE(pool.release_unused());
    EXPECT_EQ(pool.retained_capacity(), 2U);
}
