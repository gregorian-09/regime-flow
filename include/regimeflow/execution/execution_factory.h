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

class ExecutionFactory {
public:
    static std::unique_ptr<ExecutionModel> create_execution_model(const Config& config);
    static std::unique_ptr<SlippageModel> create_slippage_model(const Config& config);
    static std::unique_ptr<CommissionModel> create_commission_model(const Config& config);
    static std::unique_ptr<TransactionCostModel> create_transaction_cost_model(const Config& config);
    static std::unique_ptr<MarketImpactModel> create_market_impact_model(const Config& config);
    static std::unique_ptr<LatencyModel> create_latency_model(const Config& config);
};

}  // namespace regimeflow::execution
