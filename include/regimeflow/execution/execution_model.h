/**
 * @file execution_model.h
 * @brief RegimeFlow regimeflow execution model declarations.
 */

#pragma once

#include "regimeflow/engine/order.h"

#include <vector>

namespace regimeflow::execution {

/**
 * @brief Base class for execution models.
 */
class ExecutionModel {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~ExecutionModel() = default;
    /**
     * @brief Execute an order and produce fills.
     * @param order Order to execute.
     * @param reference_price Price used for execution modeling.
     * @param timestamp Execution timestamp.
     * @return Vector of fills.
     */
    virtual std::vector<engine::Fill> execute(const engine::Order& order,
                                              Price reference_price,
                                              Timestamp timestamp) = 0;
};

}  // namespace regimeflow::execution
