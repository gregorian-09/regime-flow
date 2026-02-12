/**
 * @file regime_factory.h
 * @brief RegimeFlow regimeflow regime factory declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/regime/constant_detector.h"

#include <memory>

namespace regimeflow::regime {

/**
 * @brief Factory for regime detectors.
 */
class RegimeFactory {
public:
    /**
     * @brief Create a detector from config.
     * @param config Regime configuration.
     * @return Detector instance.
     */
    static std::unique_ptr<RegimeDetector> create_detector(const Config& config);
};

}  // namespace regimeflow::regime
