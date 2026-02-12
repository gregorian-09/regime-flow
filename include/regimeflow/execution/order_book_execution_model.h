/**
 * @file order_book_execution_model.h
 * @brief RegimeFlow regimeflow order book execution model declarations.
 */

#pragma once

#include "regimeflow/data/order_book.h"
#include "regimeflow/execution/execution_model.h"

#include <memory>

namespace regimeflow::execution {

/**
 * @brief Execution model that uses an order book snapshot.
 */
class OrderBookExecutionModel final : public ExecutionModel {
public:
    /**
     * @brief Construct with a shared order book snapshot.
     * @param book Order book snapshot.
     */
    explicit OrderBookExecutionModel(std::shared_ptr<data::OrderBook> book);

    /**
     * @brief Execute an order using order book depth.
     * @param order Order to execute.
     * @param reference_price Reference price.
     * @param timestamp Execution timestamp.
     * @return Vector of fills.
     */
    std::vector<engine::Fill> execute(const engine::Order& order,
                                      Price reference_price,
                                      Timestamp timestamp) override;

private:
    std::shared_ptr<data::OrderBook> book_;
};

}  // namespace regimeflow::execution
