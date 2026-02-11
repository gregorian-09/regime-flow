#pragma once

#include "regimeflow/data/order_book.h"
#include "regimeflow/execution/execution_model.h"

#include <memory>

namespace regimeflow::execution {

class OrderBookExecutionModel final : public ExecutionModel {
public:
    explicit OrderBookExecutionModel(std::shared_ptr<data::OrderBook> book);

    std::vector<engine::Fill> execute(const engine::Order& order,
                                      Price reference_price,
                                      Timestamp timestamp) override;

private:
    std::shared_ptr<data::OrderBook> book_;
};

}  // namespace regimeflow::execution
