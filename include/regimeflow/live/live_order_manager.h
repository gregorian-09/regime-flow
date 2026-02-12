/**
 * @file live_order_manager.h
 * @brief RegimeFlow regimeflow live order manager declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/regime/types.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::live {

/**
 * @brief Internal tracking of a live order.
 */
struct LiveOrder {
    engine::OrderId internal_id = 0;
    std::string broker_order_id;
    std::string broker_exec_id;

    std::string symbol;
    engine::OrderSide side = engine::OrderSide::Buy;
    engine::OrderType type = engine::OrderType::Market;
    double quantity = 0.0;
    double filled_quantity = 0.0;
    double limit_price = 0.0;
    double stop_price = 0.0;
    double avg_fill_price = 0.0;

    LiveOrderStatus status = LiveOrderStatus::PendingNew;
    std::string status_message;

    Timestamp created_at;
    Timestamp submitted_at;
    Timestamp acked_at;
    Timestamp filled_at;

    std::string strategy_id;
    regime::RegimeType regime_at_creation = regime::RegimeType::Neutral;
};

/**
 * @brief Manages live orders and broker reconciliation.
 */
class LiveOrderManager {
public:
    /**
     * @brief Construct with broker adapter.
     * @param broker Broker adapter.
     */
    explicit LiveOrderManager(BrokerAdapter* broker);

    /**
     * @brief Submit a live order.
     * @param order Order to submit.
     * @return Internal order ID.
     */
    Result<engine::OrderId> submit_order(const engine::Order& order);
    /**
     * @brief Cancel a live order by internal ID.
     * @param id Internal order ID.
     */
    Result<void> cancel_order(engine::OrderId id);
    /**
     * @brief Cancel all open orders.
     */
    Result<void> cancel_all_orders();
    /**
     * @brief Cancel all orders for a symbol.
     * @param symbol Symbol string.
     */
    Result<void> cancel_orders(const std::string& symbol);
    /**
     * @brief Modify an order by internal ID.
     * @param id Internal order ID.
     * @param mod Modification fields.
     */
    Result<void> modify_order(engine::OrderId id, const engine::OrderModification& mod);

    /**
     * @brief Get an order by internal ID.
     */
    std::optional<LiveOrder> get_order(engine::OrderId id) const;
    /**
     * @brief Get all open orders.
     */
    std::vector<LiveOrder> get_open_orders() const;
    /**
     * @brief Get orders filtered by status.
     */
    std::vector<LiveOrder> get_orders_by_status(LiveOrderStatus status) const;

    /**
     * @brief Register execution report callback.
     */
    void on_execution_report(std::function<void(const ExecutionReport&)> cb);
    /**
     * @brief Register order update callback.
     */
    void on_order_update(std::function<void(const LiveOrder&)> cb);

    /**
     * @brief Handle an execution report from broker.
     * @param report Execution report.
     */
    void handle_execution_report(const ExecutionReport& report);
    /**
     * @brief Reconcile internal orders with broker state.
     */
    Result<void> reconcile_with_broker();
    /**
     * @brief Validate order parameters for live trading.
     * @param order Order to validate.
     * @return True if valid.
     */
    bool validate_order(const engine::Order& order) const;
    /**
     * @brief Find internal order ID by broker order ID.
     * @param broker_order_id Broker order ID.
     * @return Optional internal ID.
     */
    std::optional<engine::OrderId> find_order_id_by_broker_id(
        const std::string& broker_order_id) const;

private:
    void update_order_state(LiveOrder& order, const ExecutionReport& report);

    std::unordered_map<engine::OrderId, LiveOrder> orders_;
    BrokerAdapter* broker_ = nullptr;
    engine::OrderId next_order_id_ = 1;

    std::vector<std::function<void(const ExecutionReport&)>> exec_callbacks_;
    std::vector<std::function<void(const LiveOrder&)>> order_callbacks_;
};

}  // namespace regimeflow::live
