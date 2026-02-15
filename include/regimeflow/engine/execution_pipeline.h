/**
 * @file execution_pipeline.h
 * @brief RegimeFlow regimeflow execution pipeline declarations.
 */

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

/**
 * @brief Composes execution, commission, cost, impact, and latency models.
 */
class ExecutionPipeline {
public:
    /**
     * @brief Construct the pipeline.
     * @param order_manager Order manager to update fills/orders.
     * @param market_data Market data cache.
     * @param order_books Order book cache.
     * @param event_queue Event queue for fill events.
     */
    ExecutionPipeline(MarketDataCache* market_data,
                      OrderBookCache* order_books,
                      events::EventQueue* event_queue);

    /**
     * @brief Set the execution model.
     * @param model Execution model instance.
     */
    void set_execution_model(std::unique_ptr<execution::ExecutionModel> model);
    /**
     * @brief Set the commission model.
     * @param model Commission model instance.
     */
    void set_commission_model(std::unique_ptr<execution::CommissionModel> model);
    /**
     * @brief Set the transaction cost model.
     * @param model Transaction cost model instance.
     */
    void set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model);
    /**
     * @brief Set the market impact model.
     * @param model Market impact model instance.
     */
    void set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model);
    /**
     * @brief Set the latency model.
     * @param model Latency model instance.
     */
    void set_latency_model(std::unique_ptr<execution::LatencyModel> model);

    /**
     * @brief Handle order submission and generate fill events.
     * @param order Submitted order.
     */
    void on_order_submitted(const Order& order);

private:
    Price reference_price(const Order& order) const;

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
