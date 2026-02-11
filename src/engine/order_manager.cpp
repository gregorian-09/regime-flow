#include "regimeflow/engine/order_manager.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::engine {

Result<OrderId> OrderManager::submit_order(Order order) {
    {
        std::vector<std::function<Result<void>(Order&)>> pre_submit;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pre_submit = pre_submit_callbacks_;
        }
        for (const auto& cb : pre_submit) {
            if (auto result = cb(order); result.is_err()) {
                const auto& err = result.error();
                Error copy(err.code, err.message, err.location);
                copy.details = err.details;
                return copy;
            }
        }
    }

    if (auto validation = validate_order(order); validation.is_err()) {
        const auto& err = validation.error();
        Error copy(err.code, err.message, err.location);
        copy.details = err.details;
        return copy;
    }

    std::vector<std::function<void(const Order&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (order.id == 0) {
            order.id = next_order_id_++;
        }
        if (order.created_at.microseconds() == 0) {
            order.created_at = Timestamp::now();
        }
        order.updated_at = order.created_at;
        order.status = OrderStatus::Created;
        orders_[order.id] = order;
        callbacks = order_callbacks_;
    }

    for (const auto& cb : callbacks) {
        cb(order);
    }

    update_order_status(order.id, OrderStatus::Pending);

    return order.id;
}

Result<void> OrderManager::cancel_order(OrderId id) {
    Order updated;
    std::vector<std::function<void(const Order&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(id);
        if (it == orders_.end()) {
            return Error(Error::Code::NotFound, "Order not found");
        }
        if (!is_open_status(it->second.status)) {
            return Error(Error::Code::InvalidState, "Order not open");
        }
        it->second.status = OrderStatus::Cancelled;
        it->second.updated_at = Timestamp::now();
        updated = it->second;
        callbacks = order_callbacks_;
    }

    for (const auto& cb : callbacks) {
        cb(updated);
    }

    return Ok();
}

Result<void> OrderManager::modify_order(OrderId id, const OrderModification& mod) {
    Order updated;
    std::vector<std::function<void(const Order&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(id);
        if (it == orders_.end()) {
            return Error(Error::Code::NotFound, "Order not found");
        }
        if (!is_open_status(it->second.status)) {
            return Error(Error::Code::InvalidState, "Order not open");
        }
        if (mod.quantity) {
            it->second.quantity = *mod.quantity;
        }
        if (mod.limit_price) {
            it->second.limit_price = *mod.limit_price;
        }
        if (mod.stop_price) {
            it->second.stop_price = *mod.stop_price;
        }
        if (mod.tif) {
            it->second.tif = *mod.tif;
        }
        if (auto validation = validate_order(it->second); validation.is_err()) {
            const auto& err = validation.error();
            Error copy(err.code, err.message, err.location);
            copy.details = err.details;
            return copy;
        }
        it->second.updated_at = Timestamp::now();
        updated = it->second;
        callbacks = order_callbacks_;
    }

    for (const auto& cb : callbacks) {
        cb(updated);
    }

    return Ok();
}

std::optional<Order> OrderManager::get_order(OrderId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<Order> OrderManager::get_open_orders() const {
    std::vector<Order> result;
    std::lock_guard<std::mutex> lock(mutex_);
    result.reserve(orders_.size());
    for (const auto& [id, order] : orders_) {
        if (is_open_status(order.status)) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<Order> OrderManager::get_open_orders(SymbolId symbol) const {
    std::vector<Order> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, order] : orders_) {
        if (order.symbol == symbol && is_open_status(order.status)) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<Order> OrderManager::get_orders_by_strategy(const std::string& strategy_id) const {
    std::vector<Order> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, order] : orders_) {
        if (order.strategy_id == strategy_id) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<Fill> OrderManager::get_fills(OrderId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fills_.find(id);
    if (it == fills_.end()) {
        return {};
    }
    return it->second;
}

std::vector<Fill> OrderManager::get_fills(SymbolId symbol, TimeRange range) const {
    std::vector<Fill> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [order_id, fills] : fills_) {
        for (const auto& fill : fills) {
            if (fill.symbol == symbol && range.contains(fill.timestamp)) {
                result.push_back(fill);
            }
        }
    }
    return result;
}

void OrderManager::on_order_update(std::function<void(const Order&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    order_callbacks_.push_back(std::move(callback));
}

void OrderManager::on_fill(std::function<void(const Fill&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    fill_callbacks_.push_back(std::move(callback));
}

void OrderManager::on_pre_submit(std::function<Result<void>(Order&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    pre_submit_callbacks_.push_back(std::move(callback));
}

void OrderManager::process_fill(Fill fill) {
    Order updated;
    std::vector<std::function<void(const Order&)>> order_callbacks;
    std::vector<std::function<void(const Fill&)>> fill_callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(fill.order_id);
        if (it == orders_.end()) {
            return;
        }
        if (fill.id == 0) {
            fill.id = next_fill_id_++;
        }
        if (fill.timestamp.microseconds() == 0) {
            fill.timestamp = Timestamp::now();
        }
        auto& order = it->second;
        double filled_abs = std::abs(fill.quantity);
        order.filled_quantity += filled_abs;
        if (order.filled_quantity > 0) {
            double total_value = order.avg_fill_price * (order.filled_quantity - filled_abs)
                                 + fill.price * filled_abs;
            order.avg_fill_price = total_value / order.filled_quantity;
        }
        if (order.filled_quantity >= order.quantity) {
            order.status = OrderStatus::Filled;
        } else {
            order.status = OrderStatus::PartiallyFilled;
        }
        order.updated_at = fill.timestamp;
        fills_[fill.order_id].push_back(fill);
        updated = order;
        order_callbacks = order_callbacks_;
        fill_callbacks = fill_callbacks_;
    }

    for (const auto& cb : fill_callbacks) {
        cb(fill);
    }
    for (const auto& cb : order_callbacks) {
        cb(updated);
    }
}

Result<void> OrderManager::update_order_status(OrderId id, OrderStatus status) {
    Order updated;
    std::vector<std::function<void(const Order&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(id);
        if (it == orders_.end()) {
            return Error(Error::Code::NotFound, "Order not found");
        }
        it->second.status = status;
        it->second.updated_at = Timestamp::now();
        updated = it->second;
        callbacks = order_callbacks_;
    }

    for (const auto& cb : callbacks) {
        cb(updated);
    }

    return Ok();
}

Result<void> OrderManager::validate_order(const Order& order) const {
    if (order.symbol == 0) {
        return Error(Error::Code::InvalidArgument, "Order symbol not set");
    }
    if (order.quantity <= 0) {
        return Error(Error::Code::InvalidArgument, "Order quantity must be positive");
    }
    if (order.type == OrderType::Limit && order.limit_price <= 0) {
        return Error(Error::Code::InvalidArgument, "Limit price must be positive");
    }
    if (order.type == OrderType::Stop && order.stop_price <= 0) {
        return Error(Error::Code::InvalidArgument, "Stop price must be positive");
    }
    if (order.type == OrderType::StopLimit) {
        if (order.limit_price <= 0 || order.stop_price <= 0) {
            return Error(Error::Code::InvalidArgument, "Stop limit requires stop and limit price");
        }
    }
    return Ok();
}

bool OrderManager::is_open_status(OrderStatus status) const {
    return status == OrderStatus::Created || status == OrderStatus::Pending
        || status == OrderStatus::PartiallyFilled;
}

}  // namespace regimeflow::engine
