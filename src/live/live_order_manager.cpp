#include "regimeflow/live/live_order_manager.h"

#include <algorithm>

namespace regimeflow::live {

namespace {

const char* status_name(LiveOrderStatus status) {
    switch (status) {
        case LiveOrderStatus::PendingNew: return "PendingNew";
        case LiveOrderStatus::New: return "New";
        case LiveOrderStatus::PartiallyFilled: return "PartiallyFilled";
        case LiveOrderStatus::Filled: return "Filled";
        case LiveOrderStatus::PendingCancel: return "PendingCancel";
        case LiveOrderStatus::Cancelled: return "Cancelled";
        case LiveOrderStatus::Rejected: return "Rejected";
        case LiveOrderStatus::Expired: return "Expired";
        case LiveOrderStatus::Error: return "Error";
        default: return "Unknown";
    }
}

bool is_valid_transition(LiveOrderStatus from, LiveOrderStatus to) {
    if (from == to) {
        return true;
    }
    switch (from) {
        case LiveOrderStatus::PendingNew:
            return to == LiveOrderStatus::New || to == LiveOrderStatus::PartiallyFilled
                || to == LiveOrderStatus::Filled || to == LiveOrderStatus::Rejected
                || to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Expired
                || to == LiveOrderStatus::Error;
        case LiveOrderStatus::New:
            return to == LiveOrderStatus::PartiallyFilled || to == LiveOrderStatus::Filled
                || to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Rejected
                || to == LiveOrderStatus::Expired || to == LiveOrderStatus::Error;
        case LiveOrderStatus::PartiallyFilled:
            return to == LiveOrderStatus::PartiallyFilled || to == LiveOrderStatus::Filled
                || to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Rejected
                || to == LiveOrderStatus::Expired || to == LiveOrderStatus::Error;
        case LiveOrderStatus::PendingCancel:
            return to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Rejected
                || to == LiveOrderStatus::Expired || to == LiveOrderStatus::Error;
        case LiveOrderStatus::Cancelled:
        case LiveOrderStatus::Rejected:
        case LiveOrderStatus::Filled:
        case LiveOrderStatus::Expired:
        case LiveOrderStatus::Error:
            return false;
        default:
            return false;
    }
}

}  // namespace

LiveOrderManager::LiveOrderManager(BrokerAdapter* broker) : broker_(broker) {}

Result<engine::OrderId> LiveOrderManager::submit_order(const engine::Order& order) {
    if (!broker_) {
        return Error(Error::Code::InvalidState, "Broker adapter not configured");
    }
    if (!validate_order(order)) {
        return Error(Error::Code::InvalidArgument, "Order validation failed");
    }
    engine::OrderId id = order.id != 0 ? order.id : next_order_id_++;
    if (id >= next_order_id_) {
        next_order_id_ = id + 1;
    }
    auto broker_id = broker_->submit_order(order);
    if (broker_id.is_err()) {
        const auto& err = broker_id.error();
        Error copy(err.code, err.message, err.location);
        copy.details = err.details;
        return copy;
    }

    LiveOrder live;
    live.internal_id = id;
    live.broker_order_id = broker_id.value();
    live.symbol = SymbolRegistry::instance().lookup(order.symbol);
    live.side = order.side;
    live.type = order.type;
    live.quantity = order.quantity;
    live.limit_price = order.limit_price;
    live.stop_price = order.stop_price;
    live.created_at = order.created_at.microseconds() ? order.created_at : Timestamp::now();
    live.submitted_at = Timestamp::now();
    live.status = LiveOrderStatus::PendingNew;
    orders_[id] = live;

    for (const auto& cb : order_callbacks_) {
        cb(live);
    }

    return id;
}

Result<void> LiveOrderManager::cancel_order(engine::OrderId id) {
    if (!broker_) {
        return Error(Error::Code::InvalidState, "Broker adapter not configured");
    }
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return Error(Error::Code::NotFound, "Order not found");
    }
    if (auto res = broker_->cancel_order(it->second.broker_order_id); res.is_err()) {
        return res;
    }
    it->second.status = LiveOrderStatus::PendingCancel;
    for (const auto& cb : order_callbacks_) {
        cb(it->second);
    }
    return Ok();
}

Result<void> LiveOrderManager::cancel_all_orders() {
    for (const auto& [id, order] : orders_) {
        if (order.status == LiveOrderStatus::New || order.status == LiveOrderStatus::PendingNew ||
            order.status == LiveOrderStatus::PartiallyFilled) {
            auto res = broker_->cancel_order(order.broker_order_id);
            if (res.is_err()) {
                return res;
            }
        }
    }
    return Ok();
}

Result<void> LiveOrderManager::cancel_orders(const std::string& symbol) {
    for (const auto& [id, order] : orders_) {
        if (order.symbol != symbol) {
            continue;
        }
        auto res = broker_->cancel_order(order.broker_order_id);
        if (res.is_err()) {
            return res;
        }
    }
    return Ok();
}

Result<void> LiveOrderManager::modify_order(engine::OrderId id,
                                           const engine::OrderModification& mod) {
    if (!broker_) {
        return Error(Error::Code::InvalidState, "Broker adapter not configured");
    }
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return Error(Error::Code::NotFound, "Order not found");
    }
    auto res = broker_->modify_order(it->second.broker_order_id, mod);
    if (res.is_err()) {
        return res;
    }
    return Ok();
}

std::optional<LiveOrder> LiveOrderManager::get_order(engine::OrderId id) const {
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<LiveOrder> LiveOrderManager::get_open_orders() const {
    std::vector<LiveOrder> out;
    for (const auto& [id, order] : orders_) {
        if (order.status == LiveOrderStatus::PendingNew || order.status == LiveOrderStatus::New ||
            order.status == LiveOrderStatus::PartiallyFilled ||
            order.status == LiveOrderStatus::PendingCancel) {
            out.push_back(order);
        }
    }
    return out;
}

std::vector<LiveOrder> LiveOrderManager::get_orders_by_status(LiveOrderStatus status) const {
    std::vector<LiveOrder> out;
    for (const auto& [id, order] : orders_) {
        if (order.status == status) {
            out.push_back(order);
        }
    }
    return out;
}

void LiveOrderManager::on_execution_report(std::function<void(const ExecutionReport&)> cb) {
    exec_callbacks_.push_back(std::move(cb));
}

void LiveOrderManager::on_order_update(std::function<void(const LiveOrder&)> cb) {
    order_callbacks_.push_back(std::move(cb));
}

void LiveOrderManager::handle_execution_report(const ExecutionReport& report) {
    for (auto& [id, order] : orders_) {
        if (order.broker_order_id == report.broker_order_id) {
            update_order_state(order, report);
            for (const auto& cb : exec_callbacks_) {
                cb(report);
            }
            for (const auto& cb : order_callbacks_) {
                cb(order);
            }
            break;
        }
    }
}

Result<void> LiveOrderManager::reconcile_with_broker() {
    if (!broker_) {
        return Error(Error::Code::InvalidState, "Broker adapter not configured");
    }
    auto reports = broker_->get_open_orders();
    for (const auto& report : reports) {
        bool found = false;
        for (auto& [id, order] : orders_) {
            if (order.broker_order_id == report.broker_order_id) {
                update_order_state(order, report);
                for (const auto& cb : exec_callbacks_) {
                    cb(report);
                }
                for (const auto& cb : order_callbacks_) {
                    cb(order);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            LiveOrder live;
            live.internal_id = next_order_id_++;
            live.broker_order_id = report.broker_order_id;
            live.broker_exec_id = report.broker_exec_id;
            live.symbol = report.symbol;
            live.side = report.side;
            live.quantity = report.quantity;
            live.avg_fill_price = report.price;
            live.status = report.status;
            live.status_message = report.text;
            live.acked_at = report.timestamp;
            orders_[live.internal_id] = live;
            for (const auto& cb : order_callbacks_) {
                cb(live);
            }
        }
    }
    return Ok();
}

bool LiveOrderManager::validate_order(const engine::Order& order) const {
    if (order.symbol == 0) {
        return false;
    }
    if (order.quantity <= 0) {
        return false;
    }
    if (order.type == engine::OrderType::Limit && order.limit_price <= 0) {
        return false;
    }
    if (order.type == engine::OrderType::Stop && order.stop_price <= 0) {
        return false;
    }
    if (order.type == engine::OrderType::StopLimit) {
        if (order.limit_price <= 0 || order.stop_price <= 0) {
            return false;
        }
    }
    return true;
}

std::optional<engine::OrderId> LiveOrderManager::find_order_id_by_broker_id(
    const std::string& broker_order_id) const {
    for (const auto& [id, order] : orders_) {
        if (order.broker_order_id == broker_order_id) {
            return id;
        }
    }
    return std::nullopt;
}

void LiveOrderManager::update_order_state(LiveOrder& order, const ExecutionReport& report) {
    if (!is_valid_transition(order.status, report.status)) {
        auto from = order.status;
        order.status = LiveOrderStatus::Error;
        order.status_message = std::string("Invalid transition from ")
            + status_name(from) + " to " + status_name(report.status);
        return;
    }
    order.broker_exec_id = report.broker_exec_id;
    order.status = report.status;
    order.status_message = report.text;
    if (report.timestamp.microseconds() > 0) {
        order.acked_at = report.timestamp;
    }
    if (report.status == LiveOrderStatus::PartiallyFilled ||
        report.status == LiveOrderStatus::Filled) {
        double prev_filled = order.filled_quantity;
        order.filled_quantity += report.quantity;
        if (order.filled_quantity > 0) {
            double total_value = order.avg_fill_price * prev_filled
                                 + report.price * report.quantity;
            order.avg_fill_price = total_value / order.filled_quantity;
        }
        if (report.status == LiveOrderStatus::Filled) {
            order.filled_at = report.timestamp;
        }
    }
}

}  // namespace regimeflow::live
