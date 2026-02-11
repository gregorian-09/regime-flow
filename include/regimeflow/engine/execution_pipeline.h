#pragma once

#include "regimeflow/execution/execution_model.h"
#include "regimeflow/execution/basic_execution_model.h"
#include "regimeflow/execution/order_book_execution_model.h"
#include "regimeflow/execution/commission.h"
#include "regimeflow/execution/market_impact.h"
#include "regimeflow/execution/latency_model.h"
#include "regimeflow/execution/slippage.h"
#include "regimeflow/execution/transaction_cost.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/market_data_cache.h"
#include "regimeflow/engine/order_book_cache.h"
#include "regimeflow/events/event_queue.h"

#include <memory>

namespace regimeflow::engine {

class ExecutionPipeline {
public:
    ExecutionPipeline(OrderManager* order_manager,
                      MarketDataCache* market_data,
                      OrderBookCache* order_books,
                      events::EventQueue* event_queue);

    void set_execution_model(std::unique_ptr<execution::ExecutionModel> model);
    void set_commission_model(std::unique_ptr<execution::CommissionModel> model);
    void set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model);
    void set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model);
    void set_latency_model(std::unique_ptr<execution::LatencyModel> model);

    void on_order_submitted(const Order& order);

private:
    Price reference_price(const Order& order) const;

    OrderManager* order_manager_ = nullptr;
    MarketDataCache* market_data_ = nullptr;
    OrderBookCache* order_books_ = nullptr;
    events::EventQueue* event_queue_ = nullptr;
    std::unique_ptr<execution::ExecutionModel> execution_model_;
    std::unique_ptr<execution::CommissionModel> commission_model_;
    std::unique_ptr<execution::TransactionCostModel> transaction_cost_model_;
    std::unique_ptr<execution::MarketImpactModel> market_impact_model_;
    std::unique_ptr<execution::LatencyModel> latency_model_;
};

}  // namespace regimeflow::engine
