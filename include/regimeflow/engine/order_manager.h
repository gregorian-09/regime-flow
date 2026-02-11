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

struct OrderModification {
    std::optional<Quantity> quantity;
    std::optional<Price> limit_price;
    std::optional<Price> stop_price;
    std::optional<TimeInForce> tif;
};

class OrderManager {
public:
    Result<OrderId> submit_order(Order order);
    Result<void> cancel_order(OrderId id);
    Result<void> modify_order(OrderId id, const OrderModification& mod);

    std::optional<Order> get_order(OrderId id) const;
    std::vector<Order> get_open_orders() const;
    std::vector<Order> get_open_orders(SymbolId symbol) const;
    std::vector<Order> get_orders_by_strategy(const std::string& strategy_id) const;

    std::vector<Fill> get_fills(OrderId id) const;
    std::vector<Fill> get_fills(SymbolId symbol, TimeRange range) const;

    void on_order_update(std::function<void(const Order&)> callback);
    void on_fill(std::function<void(const Fill&)> callback);
    void on_pre_submit(std::function<Result<void>(Order&)> callback);

    void process_fill(Fill fill);
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
