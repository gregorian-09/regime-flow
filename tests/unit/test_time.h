#pragma once

#include "regimeflow/common/time.h"

#include <cstdint>

namespace regimeflow::test
{
    inline constexpr int64_t kFixedTimestampUs = 1'700'000'000'000'000;

    inline Timestamp fixed_timestamp(const int64_t offset_us = 0) {
        return Timestamp(kFixedTimestampUs + offset_us);
    }
}  // namespace regimeflow::test
