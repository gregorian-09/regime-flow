#include "regimeflow/engine/order_manager.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace regimeflow::engine
{
    Result<OrderId> OrderManager::submit_order(Order order) {
        return submit_order_internal(std::move(order), true);
    }

    Result<void> OrderManager::cancel_order(const OrderId id) {
        Order updated;
        std::vector<OrderId> child_ids;
        std::vector<std::function<void(const Order&)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = orders_.find(id);
            if (it == orders_.end()) {
                return Result<void>(Error(Error::Code::NotFound, "Order not found"));
            }
            if (!is_open_status(it->second.status)) {
                return Result<void>(Error(Error::Code::InvalidState, "Order not open"));
            }
            it->second.status = OrderStatus::Cancelled;
            it->second.updated_at = Timestamp::now();
            updated = it->second;
            callbacks = order_callbacks_;
            if (const auto route_it = routing_states_.find(id);
                route_it != routing_states_.end()) {
                child_ids = route_it->second.submitted_children;
            }
        }

        for (const auto& cb : callbacks) {
            cb(updated);
        }
        for (const auto child_id : child_ids) {
            update_order_status(child_id, OrderStatus::Cancelled);
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
                return Result<void>(Error(Error::Code::NotFound, "Order not found"));
            }
            if (!is_open_status(it->second.status)) {
                return Result<void>(Error(Error::Code::InvalidState, "Order not open"));
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
        if (mod.expire_at) {
            it->second.expire_at = *mod.expire_at;
        }
        if (auto validation = validate_order(it->second); validation.is_err()) {
            const auto& err = validation.error();
            Error copy(err.code, err.message, err.location);
            copy.details = err.details;
            return Result<void>(copy);
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

    std::optional<Order> OrderManager::get_order(const OrderId id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = orders_.find(id);
        if (it == orders_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<Order> OrderManager::get_open_orders() const {
        std::vector<Order> result;
        std::lock_guard<std::mutex> lock(mutex_);
        result.reserve(orders_.size());
        for (const auto& order : orders_ | std::views::values) {
            if (is_open_status(order.status)) {
                result.push_back(order);
            }
        }
        return result;
    }

    std::vector<Order> OrderManager::get_open_orders(SymbolId symbol) const {
        std::vector<Order> result;
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& order : orders_ | std::views::values) {
            if (order.symbol == symbol && is_open_status(order.status)) {
                result.push_back(order);
            }
        }
        return result;
    }

    std::vector<Order> OrderManager::get_orders_by_strategy(const std::string& strategy_id) const {
        std::vector<Order> result;
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& order : orders_ | std::views::values) {
            if (order.strategy_id == strategy_id) {
                result.push_back(order);
            }
        }
        return result;
    }

    std::vector<Fill> OrderManager::get_fills(OrderId id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = fills_.find(id);
        if (it == fills_.end()) {
            return {};
        }
        return it->second;
    }

    std::vector<Fill> OrderManager::get_fills(SymbolId symbol, TimeRange range) const {
        std::vector<Fill> result;
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& fills : fills_ | std::views::values) {
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

    void OrderManager::set_router(std::unique_ptr<OrderRouter> router,
                                  RoutingContextProvider provider) {
        std::lock_guard<std::mutex> lock(mutex_);
        router_ = std::move(router);
        routing_context_provider_ = std::move(provider);
    }

    void OrderManager::clear_router() {
        std::lock_guard<std::mutex> lock(mutex_);
        router_.reset();
        routing_context_provider_ = nullptr;
        routing_states_.clear();
        parent_by_child_.clear();
    }

    Result<void> OrderManager::activate_routed_order(OrderId id) {
        std::vector<Order> to_submit;
        SplitMode split_mode = SplitMode::None;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = routing_states_.find(id);
            if (it == routing_states_.end()) {
                return Ok();
            }
            if (it->second.activated) {
                return Ok();
            }
            it->second.activated = true;
            split_mode = it->second.split_mode;
            if (split_mode == SplitMode::Parallel) {
                to_submit = it->second.pending_children;
                it->second.pending_children.clear();
            } else if (split_mode == SplitMode::Sequential) {
                if (!it->second.pending_children.empty()) {
                    to_submit.push_back(it->second.pending_children.front());
                    it->second.pending_children.erase(it->second.pending_children.begin());
                }
            }
        }

        if (split_mode == SplitMode::None) {
            return Ok();
        }

        for (auto& child : to_submit) {
            if (auto res = submit_child_order(child, id); res.is_err()) {
                update_order_status(id, OrderStatus::Rejected);
                return res;
            }
        }
        return Ok();
    }

    bool OrderManager::is_routing_parent(OrderId id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return routing_states_.contains(id);
    }

    bool OrderManager::is_routing_child(OrderId id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return parent_by_child_.contains(id);
    }

    std::vector<OrderId> OrderManager::expired_order_ids(const Timestamp now) const {
        std::vector<OrderId> result;
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& order : orders_ | std::views::values) {
            if (!is_open_status(order.status)) {
                continue;
            }
            if (order.tif == TimeInForce::GTD && order.expire_at.has_value()
                && order.expire_at.value() <= now) {
                result.emplace_back(order.id);
            }
        }
        return result;
    }

    void OrderManager::process_fill(Fill fill) {
        Order updated;
        std::vector<Order> parent_updates;
        std::vector<Order> next_children;
        std::vector<std::function<void(const Order&)>> order_callbacks;
        std::vector<std::function<void(const Fill&)>> fill_callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = orders_.find(fill.order_id);
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
            const double filled_abs = std::abs(fill.quantity);
            order.filled_quantity += filled_abs;
            if (order.filled_quantity > 0) {
                const double total_value = order.avg_fill_price * (order.filled_quantity - filled_abs)
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

            if (const auto parent_it = parent_by_child_.find(fill.order_id);
                parent_it != parent_by_child_.end()) {
                handle_child_fill(parent_it->second, fill, parent_updates);
                if (order.status == OrderStatus::Filled) {
                    handle_child_terminal(parent_it->second,
                                          fill.order_id,
                                          order.status,
                                          fill.timestamp,
                                          parent_updates,
                                          next_children);
                }
            }
        }

        for (const auto& cb : fill_callbacks) {
            cb(fill);
        }
        for (const auto& cb : order_callbacks) {
            cb(updated);
        }
        if (!parent_updates.empty()) {
            for (const auto& parent : parent_updates) {
                for (const auto& cb : order_callbacks) {
                    cb(parent);
                }
            }
        }
        for (auto& child : next_children) {
            submit_child_order(child, child.parent_id);
        }
    }

    Result<void> OrderManager::update_order_status(OrderId id, OrderStatus status) {
        Order updated;
        std::vector<Order> parent_updates;
        std::vector<Order> next_children;
        std::vector<std::function<void(const Order&)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = orders_.find(id);
            if (it == orders_.end()) {
                return Result<void>(Error(Error::Code::NotFound, "Order not found"));
            }
            it->second.status = status;
            it->second.updated_at = Timestamp::now();
            updated = it->second;
            callbacks = order_callbacks_;

            if (const auto parent_it = parent_by_child_.find(id);
                parent_it != parent_by_child_.end()) {
                handle_child_terminal(parent_it->second,
                                      id,
                                      status,
                                      updated.updated_at,
                                      parent_updates,
                                      next_children);
            }
        }

        for (const auto& cb : callbacks) {
            cb(updated);
        }
        if (!parent_updates.empty()) {
            for (const auto& parent : parent_updates) {
                for (const auto& cb : callbacks) {
                    cb(parent);
                }
            }
        }
        for (auto& child : next_children) {
            submit_child_order(child, child.parent_id);
        }

        return Ok();
    }

    Result<OrderId> OrderManager::submit_order_internal(Order order, const bool apply_routing) {
        RoutingPlan plan;
        bool has_plan = false;
        if (apply_routing) {
            RoutingContext ctx;
            std::lock_guard<std::mutex> lock(mutex_);
            if (router_) {
                if (routing_context_provider_) {
                    ctx = routing_context_provider_(order);
                }
                plan = router_->route(order, ctx);
                order = plan.routed_order;
                has_plan = !plan.children.empty();
            }
        }

        if (has_plan) {
            order.is_parent = true;
        }

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
                    return Result<OrderId>(copy);
                }
            }
        }

        if (auto validation = validate_order(order); validation.is_err()) {
            const auto& err = validation.error();
            Error copy(err.code, err.message, err.location);
            copy.details = err.details;
            return Result<OrderId>(copy);
        }

        std::vector<std::function<void(const Order&)>> callbacks;
        if (has_plan) {
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
                RoutingState state;
                state.split_mode = plan.split_mode;
                state.aggregation = plan.parent_aggregation;
                state.pending_children.reserve(plan.children.size());
                for (auto child : plan.children) {
                    child.parent_id = order.id;
                    child.is_parent = false;
                    state.pending_children.push_back(std::move(child));
                }
                routing_states_[order.id] = std::move(state);
                callbacks = order_callbacks_;
            }
            for (const auto& cb : callbacks) {
                cb(order);
            }
            update_order_status(order.id, OrderStatus::Pending);
            return Result<OrderId>(order.id);
        }

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

        return Result<OrderId>(order.id);
    }

    Result<void> OrderManager::submit_child_order(Order order, const OrderId parent_id) {
        order.parent_id = parent_id;
        const auto result = submit_order_internal(std::move(order), false);
        if (result.is_err()) {
            return Result<void>(result.error());
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            parent_by_child_[result.value()] = parent_id;
            routing_states_[parent_id].submitted_children.push_back(result.value());
        }
        return Ok();
    }

    void OrderManager::handle_child_terminal(OrderId parent_id,
                                             OrderId child_id,
                                             OrderStatus status,
                                             Timestamp timestamp,
                                             std::vector<Order>& parent_updates,
                                             std::vector<Order>& next_children) {
        auto state_it = routing_states_.find(parent_id);
        if (state_it == routing_states_.end()) {
            return;
        }
        auto& state = state_it->second;
        bool launched_next_child = false;
        state.terminal_children.insert(child_id);
        if (status == OrderStatus::Rejected) {
            state.had_reject = true;
        }
        if (status == OrderStatus::Cancelled) {
            state.had_cancel = true;
        }
        if (state.split_mode == SplitMode::Sequential && state.activated
            && !state.pending_children.empty()) {
            const auto parent_it = orders_.find(parent_id);
            double remaining = 0.0;
            if (parent_it != orders_.end()) {
                remaining = parent_it->second.quantity - state.filled_quantity;
            }
            if (remaining > 0.0) {
                Order next = std::move(state.pending_children.front());
                state.pending_children.erase(state.pending_children.begin());
                if (next.quantity > remaining) {
                    next.quantity = remaining;
                }
                next_children.push_back(std::move(next));
                launched_next_child = true;
            } else {
                state.pending_children.clear();
            }
        }
        if (!launched_next_child) {
            finalize_parent_if_complete(parent_id, timestamp, parent_updates);
        }
    }

    void OrderManager::handle_child_fill(OrderId parent_id,
                                         const Fill& fill,
                                         std::vector<Order>& parent_updates) {
        auto state_it = routing_states_.find(parent_id);
        if (state_it == routing_states_.end()) {
            return;
        }
        auto& state = state_it->second;
        const double filled_abs = std::abs(fill.quantity);
        const double total_value = state.avg_fill_price * state.filled_quantity
                                 + fill.price * filled_abs;
        state.filled_quantity += filled_abs;
        if (state.filled_quantity > 0.0) {
            state.avg_fill_price = total_value / state.filled_quantity;
        }
        if (state.aggregation != ParentAggregation::Partial) {
            return;
        }
        const auto parent_it = orders_.find(parent_id);
        if (parent_it == orders_.end()) {
            return;
        }
        auto& parent = parent_it->second;
        parent.filled_quantity = state.filled_quantity;
        parent.avg_fill_price = state.avg_fill_price;
        if (parent.filled_quantity >= parent.quantity) {
            parent.status = OrderStatus::Filled;
        } else {
            parent.status = OrderStatus::PartiallyFilled;
        }
        parent.updated_at = fill.timestamp;
        parent_updates.push_back(parent);
    }

    void OrderManager::finalize_parent_if_complete(OrderId parent_id,
                                                   Timestamp timestamp,
                                                   std::vector<Order>& parent_updates) {
        auto state_it = routing_states_.find(parent_id);
        if (state_it == routing_states_.end()) {
            return;
        }
        auto& state = state_it->second;
        if (!state.pending_children.empty()) {
            return;
        }
        if (state.terminal_children.size() < state.submitted_children.size()) {
            return;
        }
        const auto parent_it = orders_.find(parent_id);
        if (parent_it == orders_.end()) {
            return;
        }
        auto& parent = parent_it->second;
        if (state.aggregation == ParentAggregation::Final) {
            parent.filled_quantity = state.filled_quantity;
            parent.avg_fill_price = state.avg_fill_price;
        }
        if (parent.filled_quantity >= parent.quantity) {
            parent.status = OrderStatus::Filled;
        } else if (parent.filled_quantity > 0.0) {
            parent.status = OrderStatus::Cancelled;
        } else if (state.had_reject) {
            parent.status = OrderStatus::Rejected;
        } else {
            parent.status = OrderStatus::Cancelled;
        }
        parent.updated_at = timestamp;
        parent_updates.push_back(parent);
    }

    Result<void> OrderManager::validate_order(const Order& order) const {
        if (order.symbol == 0) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Order symbol not set"));
        }
        if (order.quantity <= 0) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Order quantity must be positive"));
        }
        if (order.type == OrderType::Limit && order.limit_price <= 0) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Limit price must be positive"));
        }
        if (order.type == OrderType::Stop && order.stop_price <= 0) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Stop price must be positive"));
        }
        if (order.type == OrderType::StopLimit) {
            if (order.limit_price <= 0 || order.stop_price <= 0) {
                return Result<void>(Error(Error::Code::InvalidArgument, "Stop limit requires stop and limit price"));
            }
        }
        if (order.tif == TimeInForce::GTD && !order.expire_at.has_value()) {
            return Result<void>(Error(Error::Code::InvalidArgument, "GTD requires expire_at timestamp"));
        }
        return Ok();
    }

    bool OrderManager::is_open_status(const OrderStatus status) const {
        return status == OrderStatus::Created || status == OrderStatus::Pending
            || status == OrderStatus::PartiallyFilled;
    }
}  // namespace regimeflow::engine
