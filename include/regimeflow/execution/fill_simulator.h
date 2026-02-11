#pragma once

#include "regimeflow/execution/slippage.h"

#include <memory>

namespace regimeflow::execution {

class FillSimulator {
public:
    explicit FillSimulator(std::shared_ptr<SlippageModel> slippage_model);

    engine::Fill simulate(const engine::Order& order, Price reference_price, Timestamp timestamp,
                          bool is_maker = false) const;

private:
    std::shared_ptr<SlippageModel> slippage_model_;
};

}  // namespace regimeflow::execution
