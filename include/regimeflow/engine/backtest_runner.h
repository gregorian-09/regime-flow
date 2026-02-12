/**
 * @file backtest_runner.h
 * @brief RegimeFlow regimeflow backtest runner declarations.
 */

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

/**
 * @brief Specification for a single backtest run.
 */
struct BacktestRunSpec {
    /**
     * @brief Engine configuration for execution/risk/regime.
     */
    Config engine_config;
    /**
     * @brief Data source configuration.
     */
    Config data_config;
    /**
     * @brief Strategy configuration.
     */
    Config strategy_config;
    /**
     * @brief Backtest time range.
     */
    TimeRange range;
    /**
     * @brief Symbols to trade.
     */
    std::vector<std::string> symbols;
    /**
     * @brief Bar type for aggregated data.
     */
    data::BarType bar_type = data::BarType::Time_1Day;
};

/**
 * @brief Helper for running backtests with a data source.
 */
class BacktestRunner {
public:
    /**
     * @brief Construct a runner.
     * @param engine Backtest engine instance.
     * @param data_source Data source for iterators.
     */
    BacktestRunner(BacktestEngine* engine, data::DataSource* data_source);

    /**
     * @brief Run a single backtest.
     * @param strategy Strategy instance.
     * @param range Backtest time range.
     * @param symbols Symbols to trade.
     * @param bar_type Bar type for data aggregation.
     * @return Backtest results.
     */
    BacktestResults run(std::unique_ptr<strategy::Strategy> strategy,
                        const TimeRange& range,
                        const std::vector<SymbolId>& symbols,
                        data::BarType bar_type = data::BarType::Time_1Day);

    /**
     * @brief Run multiple backtests in parallel.
     * @param runs Run specifications.
     * @param num_threads Worker threads (-1 uses default).
     * @return Results for each run.
     */
    static std::vector<BacktestResults> run_parallel(const std::vector<BacktestRunSpec>& runs,
                                                     int num_threads = -1);

private:
    BacktestEngine* engine_ = nullptr;
    data::DataSource* data_source_ = nullptr;
};

}  // namespace regimeflow::engine
