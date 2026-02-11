#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/regime/constant_detector.h"

#include <memory>

namespace regimeflow::regime {

class RegimeFactory {
public:
    static std::unique_ptr<RegimeDetector> create_detector(const Config& config);
};

}  // namespace regimeflow::regime
