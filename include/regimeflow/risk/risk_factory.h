#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/risk/risk_limits.h"

#include <memory>

namespace regimeflow::risk {

class RiskFactory {
public:
    static RiskManager create_risk_manager(const Config& config);
};

}  // namespace regimeflow::risk
