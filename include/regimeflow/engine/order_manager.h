/**
 * @file order_manager.h
 * @brief RegimeFlow regimeflow order manager declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"
#include "regimeflow/engine/order.h"

#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::engine {

/**
 * @brief Fields that can be modified on an existing order.
 */
struct OrderModification {
    /**
     * @brief New quantity, if provided.
     */
    std::optional<Quantity> quantity;
    /**
     * @brief New limit price, if provided.
     */
    std::optional<Price> limit_price;
    /**
     * @brief New stop price, if provided.
     */
    std::optional<Price> stop_price;
    /**
     * @brief New time-in-force, if provided.
     */
    std::optional<TimeInForce> tif;
};

/**
 * @brief Tracks orders, status changes, and fills.
 */
class OrderManager {
public:
    /**
     * @brief Submit a new order.
     * @param order Order to submit.
     * @return Assigned OrderId or error.
     */
    Result<OrderId> submit_order(Order order);
    /**
     * @brief Cancel an order by ID.
     * @param id OrderId to cancel.
     * @return Ok on success, error on failure.
     */
    Result<void> cancel_order(OrderId id);
    /**
     * @brief Modify an existing order.
     * @param id OrderId to modify.
     * @param mod Modification fields.
     * @return Ok on success, error on failure.
     */
    Result<void> modify_order(OrderId id, const OrderModification& mod);

    /**
     * @brief Get an order by ID.
     * @param id OrderId to retrieve.
     * @return Optional order.
     */
    std::optional<Order> get_order(OrderId id) const;
    /**
     * @brief Get all open orders.
     * @return Vector of open orders.
     */
    std::vector<Order> get_open_orders() const;
    /**
     * @brief Get open orders for a symbol.
     * @param symbol Symbol ID.
     * @return Vector of open orders.
     */
    std::vector<Order> get_open_orders(SymbolId symbol) const;
    /**
     * @brief Get orders placed by a strategy.
     * @param strategy_id Strategy identifier.
     * @return Vector of orders.
     */
    std::vector<Order> get_orders_by_strategy(const std::string& strategy_id) const;

    /**
     * @brief Get fills for a specific order.
     * @param id OrderId.
     * @return Vector of fills.
     */
    std::vector<Fill> get_fills(OrderId id) const;
    /**
     * @brief Get fills for a symbol within a time range.
     * @param symbol Symbol ID.
     * @param range Time range filter.
     * @return Vector of fills.
     */
    std::vector<Fill> get_fills(SymbolId symbol, TimeRange range) const;

    /**
     * @brief Register callback on order updates.
     * @param callback Callback invoked with updated order.
     */
    void on_order_update(std::function<void(const Order&)> callback);
    /**
     * @brief Register callback on fills.
     * @param callback Callback invoked with fill.
     */
    void on_fill(std::function<void(const Fill&)> callback);
    /**
     * @brief Register pre-submit validation callback.
     * @param callback Callback invoked before submission.
     */
    void on_pre_submit(std::function<Result<void>(Order&)> callback);

    /**
     * @brief Process a fill and update the order.
     * @param fill Fill to apply.
     */
    void process_fill(Fill fill);
    /**
     * @brief Update an order status.
     * @param id OrderId.
     * @param status New status.
     * @return Ok on success or error.
     */
    Result<void> update_order_status(OrderId id, OrderStatus status);

private:
    Result<void> validate_order(const Order& order) const;
    bool is_open_status(OrderStatus status) const;

    mutable std::mutex mutex_;
    std::unordered_map<OrderId, Order> orders_;
    std::unordered_map<OrderId, std::vector<Fill>> fills_;
    std::vector<std::function<void(const Order&)>> order_callbacks_;
    std::vector<std::function<void(const Fill&)>> fill_callbacks_;
    std::vector<std::function<Result<void>(Order&)>> pre_submit_callbacks_;

    OrderId next_order_id_ = 1;
    FillId next_fill_id_ = 1;
};

}  // namespace regimeflow::engine
