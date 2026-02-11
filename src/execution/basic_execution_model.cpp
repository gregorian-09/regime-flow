#include "regimeflow/execution/basic_execution_model.h"

namespace regimeflow::execution {

BasicExecutionModel::BasicExecutionModel(std::shared_ptr<SlippageModel> slippage_model)
    : simulator_(std::move(slippage_model)) {}

std::vector<engine::Fill> BasicExecutionModel::execute(const engine::Order& order,
                                                       Price reference_price,
                                                       Timestamp timestamp) {
    std::vector<engine::Fill> fills;
    fills.push_back(simulator_.simulate(order, reference_price, timestamp));
    return fills;
}

}  // namespace regimeflow::execution
