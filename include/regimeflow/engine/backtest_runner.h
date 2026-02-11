#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/engine/backtest_results.h"
#include "regimeflow/common/config.h"
#include "regimeflow/strategy/strategy.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace regimeflow::engine {

struct BacktestRunSpec {
    Config engine_config;
    Config data_config;
    Config strategy_config;
    TimeRange range;
    std::vector<std::string> symbols;
    data::BarType bar_type = data::BarType::Time_1Day;
};

class BacktestRunner {
public:
    BacktestRunner(BacktestEngine* engine, data::DataSource* data_source);

    BacktestResults run(std::unique_ptr<strategy::Strategy> strategy,
                        const TimeRange& range,
                        const std::vector<SymbolId>& symbols,
                        data::BarType bar_type = data::BarType::Time_1Day);

    static std::vector<BacktestResults> run_parallel(const std::vector<BacktestRunSpec>& runs,
                                                     int num_threads = -1);

private:
    BacktestEngine* engine_ = nullptr;
    data::DataSource* data_source_ = nullptr;
};

}  // namespace regimeflow::engine
