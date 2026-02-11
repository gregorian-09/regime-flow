#pragma once

#include "regimeflow/common/time.h"

#include <array>
#include <cstdint>
#include <vector>

namespace regimeflow::regime {

enum class RegimeType : uint8_t {
    Bull = 0,
    Neutral = 1,
    Bear = 2,
    Crisis = 3,
    Custom = 255
};

struct RegimeState {
    RegimeType regime = RegimeType::Neutral;
    double confidence = 0.0;
    std::array<double, 4> probabilities{};
    std::vector<double> probabilities_all;
    size_t state_count = 0;
    Timestamp timestamp;
};

struct RegimeTransition {
    RegimeType from = RegimeType::Neutral;
    RegimeType to = RegimeType::Neutral;
    Timestamp timestamp;
    double confidence = 0.0;
    double duration_in_from = 0.0;
};

}  // namespace regimeflow::regime
