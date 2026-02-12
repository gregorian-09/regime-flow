/**
 * @file basic_execution_model.h
 * @brief RegimeFlow regimeflow basic execution model declarations.
 */

#pragma once

#include "regimeflow/execution/execution_model.h"
#include "regimeflow/execution/fill_simulator.h"

#include <memory>
#include <vector>

namespace regimeflow::execution {

/**
 * @brief Simple execution model using a fill simulator.
 */
class BasicExecutionModel final : public ExecutionModel {
public:
    /**
     * @brief Construct with a slippage model.
     * @param slippage_model Slippage model instance.
     */
    explicit BasicExecutionModel(std::shared_ptr<SlippageModel> slippage_model);

    /**
     * @brief Execute an order with simulated slippage.
     * @param order Order to execute.
     * @param reference_price Reference price.
     * @param timestamp Execution timestamp.
     * @return Vector of fills.
     */
    std::vector<engine::Fill> execute(const engine::Order& order,
                                      Price reference_price,
                                      Timestamp timestamp) override;

private:
    FillSimulator simulator_;
};

}  // namespace regimeflow::execution
