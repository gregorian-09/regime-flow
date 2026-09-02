#include "regimeflow/live/live_order_manager.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <optional>
#include <ranges>
#include <sstream>

namespace regimeflow::live
{
    namespace {
        constexpr size_t kMaxPendingReportsPerBrokerOrder = 64;

        bool is_valid_transition(const LiveOrderStatus from, const LiveOrderStatus to) {
            if (from == to) {
                return true;
            }
            switch (from) {
            case LiveOrderStatus::PendingNew:
                return to == LiveOrderStatus::New || to == LiveOrderStatus::PartiallyFilled
                    || to == LiveOrderStatus::Filled || to == LiveOrderStatus::Rejected
                    || to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Expired
                    || to == LiveOrderStatus::Inactive
                    || to == LiveOrderStatus::Error;
            case LiveOrderStatus::New:
                return to == LiveOrderStatus::PartiallyFilled || to == LiveOrderStatus::Filled
                    || to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Rejected
                    || to == LiveOrderStatus::Inactive
                    || to == LiveOrderStatus::Expired || to == LiveOrderStatus::Error;
            case LiveOrderStatus::PartiallyFilled:
                return to == LiveOrderStatus::PartiallyFilled || to == LiveOrderStatus::Filled
                    || to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Rejected
                    || to == LiveOrderStatus::Inactive
                    || to == LiveOrderStatus::Expired || to == LiveOrderStatus::Error;
            case LiveOrderStatus::PendingCancel:
                return to == LiveOrderStatus::Cancelled || to == LiveOrderStatus::Rejected
                    || to == LiveOrderStatus::Expired || to == LiveOrderStatus::Inactive
                    || to == LiveOrderStatus::Error;
            case LiveOrderStatus::Cancelled:
            case LiveOrderStatus::Rejected:
            case LiveOrderStatus::Filled:
            case LiveOrderStatus::Expired:
            case LiveOrderStatus::Inactive:
            case LiveOrderStatus::Error:
                return false;
            default:
                return false;
            }
        }

        bool is_terminal_status(const LiveOrderStatus status) {
            return status == LiveOrderStatus::Filled || status == LiveOrderStatus::Cancelled
                || status == LiveOrderStatus::Rejected || status == LiveOrderStatus::Expired
                || status == LiveOrderStatus::Inactive || status == LiveOrderStatus::Error;
        }

        std::optional<double> metadata_double(const engine::Order& order, const std::string& key) {
            const auto it = order.metadata.find(key);
            if (it == order.metadata.end() || it->second.empty()) {
                return std::nullopt;
            }
            errno = 0;
            char* end = nullptr;
            const double value = std::strtod(it->second.c_str(), &end);
            if (errno == ERANGE || end == it->second.c_str() || *end != '\0') {
                return std::nullopt;
            }
            return value;
        }

    }  // namespace

    LiveOrderManager::LiveOrderManager(BrokerAdapter* broker) : broker_(broker) {}

    Result<engine::OrderId> LiveOrderManager::submit_order(const engine::Order& order) {
        if (!broker_) {
            return Result<engine::OrderId>(Error(Error::Code::InvalidState, "Broker adapter not configured"));
        }
        if (!validate_order(order)) {
            return Result<engine::OrderId>(Error(Error::Code::InvalidArgument, "Order validation failed"));
        }
        const Timestamp now = Timestamp::now();
        const std::string order_key = duplicate_key(order);
        engine::OrderId id = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (order.id != 0) {
                const auto existing = orders_.find(order.id);
                if ((existing != orders_.end() && !is_terminal_status(existing->second.status)) ||
                    pending_submission_keys_.contains(order.id)) {
                    execution_quality_.record_submit_rejected(now);
                    Error error(Error::Code::AlreadyExists, "Duplicate live order id rejected");
                    error.details = "order_id=" + std::to_string(order.id);
                    return Result<engine::OrderId>(std::move(error));
                }
            }
            if (is_duplicate_order(order, now)) {
                execution_quality_.record_submit_rejected(now);
                Error error(Error::Code::AlreadyExists, "Duplicate live order rejected");
                error.details = order_key;
                return Result<engine::OrderId>(std::move(error));
            }

            id = order.id != 0 ? order.id : next_order_id_++;
            if (id >= next_order_id_) {
                next_order_id_ = id + 1;
            }
            // Reserve the ID while the broker call executes outside the mutex.
            pending_submission_keys_.emplace(id, order_key);
            if (duplicate_order_window_us_ > 0) {
                recent_order_keys_[order_key] = now;
            }
        }

        auto broker_id = broker_->submit_order(order);
        if (broker_id.is_err()) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending_submission_keys_.erase(id);
                if (duplicate_order_window_us_ > 0) {
                    recent_order_keys_.erase(order_key);
                }
                execution_quality_.record_submit_rejected(Timestamp::now());
            }
            const auto& err = broker_id.error();
            Error copy(err.code, err.message, err.location);
            copy.details = err.details;
            return Result<engine::OrderId>(copy);
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
        if (const auto venue = order.metadata.find("venue"); venue != order.metadata.end()) {
            live.venue = venue->second;
        }
        live.expected_queue_delay_ms = metadata_double(order, "expected_queue_delay_ms").value_or(0.0);
        live.queue_position = metadata_double(order, "queue_position").value_or(0.0);
        live.created_at = order.created_at.microseconds() ? order.created_at : Timestamp::now();
        live.submitted_at = Timestamp::now();
        live.status = LiveOrderStatus::PendingNew;
        std::vector<ExecutionReport> pending_reports;
        std::vector<std::function<void(const ExecutionReport&)>> exec_callbacks;
        std::vector<std::function<void(const LiveOrder&)>> order_callbacks;
        std::vector<LiveOrder> order_updates;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_submission_keys_.erase(id);
            orders_[id] = live;
            execution_quality_.record_submitted(live);
            order_updates.push_back(live);
            if (auto reports = pending_reports_.find(live.broker_order_id);
                reports != pending_reports_.end()) {
                pending_reports = std::move(reports->second);
                pending_reports_.erase(reports);
                for (const auto& report : pending_reports) {
                    update_order_state(orders_.at(id), report);
                    execution_quality_.record_execution_report(orders_.at(id), report);
                    order_updates.push_back(orders_.at(id));
                }
            }
            exec_callbacks = exec_callbacks_;
            order_callbacks = order_callbacks_;
        }
        for (const auto& updated : order_updates) {
            for (const auto& cb : order_callbacks) {
                cb(updated);
            }
        }
        for (const auto& report : pending_reports) {
            for (const auto& cb : exec_callbacks) {
                cb(report);
            }
        }

        return Result<engine::OrderId>(id);
    }

    void LiveOrderManager::set_duplicate_order_window(const Duration window) {
        std::lock_guard<std::mutex> lock(mutex_);
        duplicate_order_window_us_ = std::max<int64_t>(0, window.total_microseconds());
        if (duplicate_order_window_us_ == 0) {
            recent_order_keys_.clear();
        }
    }

    Result<void> LiveOrderManager::cancel_order(engine::OrderId id) {
        if (!broker_) {
            return Result<void>(Error(Error::Code::InvalidState, "Broker adapter not configured"));
        }
        std::string broker_order_id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = orders_.find(id);
            if (it == orders_.end()) {
                return Result<void>(Error(Error::Code::NotFound, "Order not found"));
            }
            broker_order_id = it->second.broker_order_id;
        }
        if (auto res = broker_->cancel_order(broker_order_id); res.is_err()) {
            return res;
        }
        LiveOrder updated;
        std::vector<std::function<void(const LiveOrder&)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = orders_.find(id);
            if (it == orders_.end()) {
                return Result<void>(Error(Error::Code::NotFound, "Order disappeared during cancellation"));
            }
            if (is_terminal_status(it->second.status)) {
                return Ok();
            }
            it->second.status = LiveOrderStatus::PendingCancel;
            updated = it->second;
            callbacks = order_callbacks_;
        }
        for (const auto& cb : callbacks) {
            cb(updated);
        }
        return Ok();
    }

    Result<void> LiveOrderManager::cancel_all_orders() {
        std::vector<engine::OrderId> ids;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [id, order] : orders_) {
                if (order.status == LiveOrderStatus::New || order.status == LiveOrderStatus::PendingNew ||
                    order.status == LiveOrderStatus::PartiallyFilled) {
                    ids.push_back(id);
                }
            }
        }
        for (const auto id : ids) {
            if (auto res = cancel_order(id); res.is_err()) {
                return res;
            }
        }
        return Ok();
    }

    Result<void> LiveOrderManager::cancel_orders(const std::string& symbol) {
        std::vector<engine::OrderId> ids;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [id, order] : orders_) {
                if (order.symbol == symbol && !is_terminal_status(order.status)) {
                    ids.push_back(id);
                }
            }
        }
        for (const auto id : ids) {
            if (auto res = cancel_order(id); res.is_err()) {
                return res;
            }
        }
        return Ok();
    }

    Result<void> LiveOrderManager::modify_order(engine::OrderId id,
                                               const engine::OrderModification& mod) {
        if (!broker_) {
            return Result<void>(Error(Error::Code::InvalidState, "Broker adapter not configured"));
        }
        std::string broker_order_id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = orders_.find(id);
            if (it == orders_.end()) {
                return Result<void>(Error(Error::Code::NotFound, "Order not found"));
            }
            broker_order_id = it->second.broker_order_id;
        }
        if (auto res = broker_->modify_order(broker_order_id, mod); res.is_err()) {
            return res;
        }
        return Ok();
    }

    std::optional<LiveOrder> LiveOrderManager::get_order(engine::OrderId id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = orders_.find(id);
        if (it == orders_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<LiveOrder> LiveOrderManager::get_open_orders() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LiveOrder> out;
        for (const auto& order : orders_ | std::views::values) {
            if (order.status == LiveOrderStatus::PendingNew || order.status == LiveOrderStatus::New ||
                order.status == LiveOrderStatus::PartiallyFilled ||
                order.status == LiveOrderStatus::PendingCancel) {
                out.push_back(order);
                }
        }
        return out;
    }

    std::vector<LiveOrder> LiveOrderManager::get_orders_by_status(LiveOrderStatus status) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LiveOrder> out;
        for (const auto& order : orders_ | std::views::values) {
            if (order.status == status) {
                out.push_back(order);
            }
        }
        return out;
    }

    void LiveOrderManager::on_execution_report(std::function<void(const ExecutionReport&)> cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        exec_callbacks_.push_back(std::move(cb));
    }

    void LiveOrderManager::on_order_update(std::function<void(const LiveOrder&)> cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        order_callbacks_.push_back(std::move(cb));
    }

    void LiveOrderManager::handle_execution_report(const ExecutionReport& report) {
        std::vector<std::function<void(const ExecutionReport&)>> exec_callbacks;
        std::vector<std::function<void(const LiveOrder&)>> order_callbacks;
        std::optional<LiveOrder> updated;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto order_it = std::ranges::find_if(
                orders_, [&report](const auto& entry) { return entry.second.broker_order_id == report.broker_order_id; });
            if (order_it == orders_.end()) {
                auto& pending = pending_reports_[report.broker_order_id];
                if (pending.size() < kMaxPendingReportsPerBrokerOrder) {
                    pending.push_back(report);
                }
                return;
            }
            update_order_state(order_it->second, report);
            execution_quality_.record_execution_report(order_it->second, report);
            updated = order_it->second;
            exec_callbacks = exec_callbacks_;
            order_callbacks = order_callbacks_;
        }
        for (const auto& cb : exec_callbacks) {
            cb(report);
        }
        for (const auto& cb : order_callbacks) {
            cb(*updated);
        }
    }

    Result<void> LiveOrderManager::reconcile_with_broker() {
        if (!broker_) {
            return Result<void>(Error(Error::Code::InvalidState, "Broker adapter not configured"));
        }
        const auto reports = broker_->get_open_orders();
        std::vector<std::pair<ExecutionReport, LiveOrder>> updated;
        std::vector<LiveOrder> added;
        std::vector<std::function<void(const ExecutionReport&)>> exec_callbacks;
        std::vector<std::function<void(const LiveOrder&)>> order_callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& report : reports) {
                const auto order_it = std::ranges::find_if(
                    orders_, [&report](const auto& entry) { return entry.second.broker_order_id == report.broker_order_id; });
                if (order_it != orders_.end()) {
                    update_order_state(order_it->second, report);
                    execution_quality_.record_execution_report(order_it->second, report);
                    updated.emplace_back(report, order_it->second);
                    continue;
                }
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
                added.push_back(std::move(live));
            }
            exec_callbacks = exec_callbacks_;
            order_callbacks = order_callbacks_;
        }
        for (const auto& [report, order] : updated) {
            for (const auto& cb : exec_callbacks) {
                cb(report);
            }
            for (const auto& cb : order_callbacks) {
                cb(order);
            }
        }
        for (const auto& order : added) {
            for (const auto& cb : order_callbacks) {
                cb(order);
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
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, order] : orders_) {
            if (order.broker_order_id == broker_order_id) {
                return id;
            }
        }
        return std::nullopt;
    }

    void LiveOrderManager::restore_order(const engine::OrderId internal_id,
                                         std::string broker_order_id,
                                         std::string symbol,
                                         const LiveOrderStatus status) {
        if (broker_order_id.empty()) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        LiveOrder live;
        if (const auto it = orders_.find(internal_id); it != orders_.end()) {
            live = it->second;
        }
        live.internal_id = internal_id;
        live.broker_order_id = std::move(broker_order_id);
        if (!symbol.empty()) {
            live.symbol = std::move(symbol);
        }
        live.status = status;
        if (internal_id >= next_order_id_) {
            next_order_id_ = internal_id + 1;
        }
        orders_[internal_id] = std::move(live);
    }


    const ExecutionQualitySnapshot& LiveOrderManager::execution_quality() const noexcept {
        thread_local ExecutionQualitySnapshot snapshot;
        snapshot = execution_quality_snapshot();
        return snapshot;
    }

    ExecutionQualitySnapshot LiveOrderManager::execution_quality_snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return execution_quality_.snapshot();
    }

    const std::vector<ExecutionQualitySample>& LiveOrderManager::execution_quality_samples() const noexcept {
        thread_local std::vector<ExecutionQualitySample> samples;
        samples = execution_quality_samples_snapshot();
        return samples;
    }

    std::vector<ExecutionQualitySample> LiveOrderManager::execution_quality_samples_snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return execution_quality_.samples();
    }

    void LiveOrderManager::record_reference_quote(const engine::OrderId id, const data::Quote& quote) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!orders_.contains(id)) {
            return;
        }
        execution_quality_.record_reference_quote(id, quote);
    }

    void LiveOrderManager::prune_duplicate_keys(const Timestamp now) {
        if (duplicate_order_window_us_ <= 0) {
            recent_order_keys_.clear();
            return;
        }
        for (auto it = recent_order_keys_.begin(); it != recent_order_keys_.end();) {
            if ((now - it->second).total_microseconds() > duplicate_order_window_us_) {
                it = recent_order_keys_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::string LiveOrderManager::duplicate_key(const engine::Order& order) const {
        std::ostringstream out;
        out << order.symbol << '|'
            << static_cast<int>(order.side) << '|'
            << static_cast<int>(order.type) << '|'
            << static_cast<int>(order.tif) << '|'
            << order.quantity << '|'
            << order.limit_price << '|'
            << order.stop_price << '|'
            << order.strategy_id;
        if (const auto it = order.metadata.find("client_order_id"); it != order.metadata.end()) {
            out << "|client=" << it->second;
        }
        if (const auto it = order.metadata.find("idempotency_key"); it != order.metadata.end()) {
            out << "|idem=" << it->second;
        }
        return out.str();
    }

    bool LiveOrderManager::is_duplicate_order(const engine::Order& order, const Timestamp now) {
        if (duplicate_order_window_us_ <= 0) {
            return false;
        }
        prune_duplicate_keys(now);
        return recent_order_keys_.contains(duplicate_key(order));
    }

    void LiveOrderManager::update_order_state(LiveOrder& order, const ExecutionReport& report) {
        if (!is_valid_transition(order.status, report.status)) {
            const auto from = order.status;
            order.status = LiveOrderStatus::Error;
            order.status_message = std::string("Invalid transition from ")
                + std::string(live_order_status_name(from)) + " to "
                + std::string(live_order_status_name(report.status));
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
            const double prev_filled = order.filled_quantity;
            order.filled_quantity += report.quantity;
            if (order.filled_quantity > 0) {
                const double total_value = order.avg_fill_price * prev_filled
                                     + report.price * report.quantity;
                order.avg_fill_price = total_value / order.filled_quantity;
            }
            if (report.status == LiveOrderStatus::Filled) {
                order.filled_at = report.timestamp;
            }
            }
    }
}  // namespace regimeflow::live
