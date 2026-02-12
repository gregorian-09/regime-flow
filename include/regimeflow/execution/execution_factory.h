/**
 * @file execution_factory.h
 * @brief RegimeFlow regimeflow execution factory declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/execution/basic_execution_model.h"
#include "regimeflow/execution/commission.h"
#include "regimeflow/execution/latency_model.h"
#include "regimeflow/execution/market_impact.h"
#include "regimeflow/execution/slippage.h"
#include "regimeflow/execution/transaction_cost.h"

#include <memory>

namespace regimeflow::execution {

/**
 * @brief Factory helpers for execution-related models.
 */
class ExecutionFactory {
public:
    /**
     * @brief Create an execution model from config.
     * @param config Execution configuration.
     * @return Execution model instance.
     */
    static std::unique_ptr<ExecutionModel> create_execution_model(const Config& config);
    /**
     * @brief Create a slippage model from config.
     * @param config Execution configuration.
     * @return Slippage model instance.
     */
    static std::unique_ptr<SlippageModel> create_slippage_model(const Config& config);
    /**
     * @brief Create a commission model from config.
     * @param config Execution configuration.
     * @return Commission model instance.
     */
    static std::unique_ptr<CommissionModel> create_commission_model(const Config& config);
    /**
     * @brief Create a transaction cost model from config.
     * @param config Execution configuration.
     * @return Transaction cost model instance.
     */
    static std::unique_ptr<TransactionCostModel> create_transaction_cost_model(const Config& config);
    /**
     * @brief Create a market impact model from config.
     * @param config Execution configuration.
     * @return Market impact model instance.
     */
    static std::unique_ptr<MarketImpactModel> create_market_impact_model(const Config& config);
    /**
     * @brief Create a latency model from config.
     * @param config Execution configuration.
     * @return Latency model instance.
     */
    static std::unique_ptr<LatencyModel> create_latency_model(const Config& config);
};

}  // namespace regimeflow::execution
