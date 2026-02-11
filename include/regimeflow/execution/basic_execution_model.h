#pragma once

#include "regimeflow/execution/execution_model.h"
#include "regimeflow/execution/fill_simulator.h"

#include <memory>
#include <vector>

namespace regimeflow::execution {

class BasicExecutionModel final : public ExecutionModel {
public:
    explicit BasicExecutionModel(std::shared_ptr<SlippageModel> slippage_model);

    std::vector<engine::Fill> execute(const engine::Order& order,
                                      Price reference_price,
                                      Timestamp timestamp) override;

private:
    FillSimulator simulator_;
};

}  // namespace regimeflow::execution
