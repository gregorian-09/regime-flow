/**
 * @file engine_factory.h
 * @brief RegimeFlow regimeflow engine factory declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/execution/execution_factory.h"
#include "regimeflow/regime/regime_factory.h"
#include "regimeflow/risk/risk_factory.h"
#include "regimeflow/strategy/strategy_factory.h"

#include <memory>

namespace regimeflow::engine {

/**
 * @brief Factory for creating backtest engines from config.
 */
class EngineFactory {
public:
    /**
     * @brief Create a configured BacktestEngine.
     * @param config Root configuration.
     * @return Engine instance.
     */
    static std::unique_ptr<BacktestEngine> create(const Config& config);
};

}  // namespace regimeflow::engine
