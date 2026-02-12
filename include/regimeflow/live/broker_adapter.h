/**
 * @file broker_adapter.h
 * @brief RegimeFlow regimeflow broker adapter declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/live/types.h"

#include <functional>
#include <string>
#include <vector>

namespace regimeflow::live {

/**
 * @brief Live order status as reported by brokers.
 */
enum class LiveOrderStatus {
    PendingNew,
    New,
    PartiallyFilled,
    Filled,
    PendingCancel,
    Cancelled,
    Rejected,
    Expired,
    Error
};

/**
 * @brief Execution report from broker callbacks.
 */
struct ExecutionReport {
    std::string broker_order_id;
    std::string broker_exec_id;
    std::string symbol;
    engine::OrderSide side = engine::OrderSide::Buy;
    double quantity = 0.0;
    double price = 0.0;
    double commission = 0.0;
    LiveOrderStatus status = LiveOrderStatus::New;
    std::string text;
    Timestamp timestamp;
};

/**
 * @brief Abstract broker adapter for live trading.
 */
class BrokerAdapter {
public:
    virtual ~BrokerAdapter() = default;

    /**
     * @brief Connect to the broker API.
     */
    virtual Result<void> connect() = 0;
    /**
     * @brief Disconnect from the broker API.
     */
    virtual Result<void> disconnect() = 0;
    /**
     * @brief Check connection status.
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief Subscribe to broker market data.
     * @param symbols Symbol strings.
     */
    virtual void subscribe_market_data(const std::vector<std::string>& symbols) = 0;
    /**
     * @brief Unsubscribe from broker market data.
     * @param symbols Symbol strings.
     */
    virtual void unsubscribe_market_data(const std::vector<std::string>& symbols) = 0;

    /**
     * @brief Submit an order.
     * @param order Order to submit.
     * @return Broker order ID.
     */
    virtual Result<std::string> submit_order(const engine::Order& order) = 0;
    /**
     * @brief Cancel an order.
     * @param broker_order_id Broker order ID.
     * @return Ok on success or error.
     */
    virtual Result<void> cancel_order(const std::string& broker_order_id) = 0;
    /**
     * @brief Modify an order.
     * @param broker_order_id Broker order ID.
     * @param mod Modification fields.
     * @return Ok on success or error.
     */
    virtual Result<void> modify_order(const std::string& broker_order_id,
                                      const engine::OrderModification& mod) = 0;

    /**
     * @brief Retrieve account info.
     */
    virtual AccountInfo get_account_info() = 0;
    /**
     * @brief Retrieve current positions.
     */
    virtual std::vector<Position> get_positions() = 0;
    /**
     * @brief Retrieve open orders from broker.
     */
    virtual std::vector<ExecutionReport> get_open_orders() = 0;

    /**
     * @brief Register market data callback.
     */
    virtual void on_market_data(std::function<void(const MarketDataUpdate&)>) = 0;
    /**
     * @brief Register execution report callback.
     */
    virtual void on_execution_report(std::function<void(const ExecutionReport&)>) = 0;
    /**
     * @brief Register position update callback.
     */
    virtual void on_position_update(std::function<void(const Position&)>) = 0;

    /**
     * @brief Rate limit for order submissions.
     */
    virtual int max_orders_per_second() const = 0;
    /**
     * @brief Rate limit for total messages.
     */
    virtual int max_messages_per_second() const = 0;

    /**
     * @brief Poll broker for updates (if required).
     */
    virtual void poll() = 0;
};

}  // namespace regimeflow::live
