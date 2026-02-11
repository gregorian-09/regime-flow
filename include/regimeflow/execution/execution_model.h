#pragma once

#include "regimeflow/engine/order.h"

#include <vector>

namespace regimeflow::execution {

class ExecutionModel {
public:
    virtual ~ExecutionModel() = default;
    virtual std::vector<engine::Fill> execute(const engine::Order& order,
                                              Price reference_price,
                                              Timestamp timestamp) = 0;
};

}  // namespace regimeflow::execution
