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

class EngineFactory {
public:
    static std::unique_ptr<BacktestEngine> create(const Config& config);
};

}  // namespace regimeflow::engine
