/**
 * @file risk_factory.h
 * @brief RegimeFlow regimeflow risk factory declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/risk/risk_limits.h"

#include <memory>

namespace regimeflow::risk {

/**
 * @brief Factory for risk managers.
 */
class RiskFactory {
public:
    /**
     * @brief Create a risk manager from config.
     * @param config Risk configuration.
     * @return RiskManager instance.
     */
    static RiskManager create_risk_manager(const Config& config);
};

}  // namespace regimeflow::risk
