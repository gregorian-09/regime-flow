#include "regimeflow/live/execution_quality.h"

#include "regimeflow/live/live_order_manager.h"

#include <cmath>

namespace regimeflow::live
{
    namespace {
        double elapsed_ms(const Timestamp start, const Timestamp end) {
            if (start.microseconds() == 0 || end.microseconds() == 0 || end < start) {
                return 0.0;
            }
            return static_cast<double>((end - start).total_microseconds()) / 1000.0;
        }
    }  // namespace

    void ExecutionQualityTracker::record_submitted(const LiveOrder& order) {
        ++snapshot_.submitted;
        snapshot_.last_timestamp = order.submitted_at.microseconds() != 0 ? order.submitted_at : order.created_at;
        update_rejection_rate();
    }

    void ExecutionQualityTracker::record_submit_rejected(const Timestamp timestamp) {
        ++snapshot_.submitted;
        ++snapshot_.submit_rejected;
        snapshot_.last_timestamp = timestamp;
        update_rejection_rate();
    }

    void ExecutionQualityTracker::record_execution_report(const LiveOrder& order,
                                                          const ExecutionReport& report) {
        ExecutionQualitySample sample;
        sample.order_id = order.internal_id;
        sample.broker_order_id = order.broker_order_id;
        sample.symbol = order.symbol.empty() ? report.symbol : order.symbol;
        sample.side = order.side;
        sample.status = report.status;
        sample.quantity = report.quantity;
        sample.fill_price = report.price;
        sample.timestamp = report.timestamp;

        const Timestamp submitted_at = order.submitted_at.microseconds() != 0
            ? order.submitted_at
            : order.created_at;

        if ((report.status == LiveOrderStatus::New || report.status == LiveOrderStatus::PartiallyFilled
             || report.status == LiveOrderStatus::Filled)
            && acknowledged_orders_.insert(order.internal_id).second) {
            ++snapshot_.acknowledged;
            sample.ack_latency_ms = elapsed_ms(submitted_at, report.timestamp);
            total_ack_latency_ms_ += sample.ack_latency_ms;
            snapshot_.average_ack_latency_ms = total_ack_latency_ms_ / static_cast<double>(snapshot_.acknowledged);
        }

        switch (report.status) {
        case LiveOrderStatus::PartiallyFilled:
            ++snapshot_.partially_filled;
            break;
        case LiveOrderStatus::Filled:
            ++snapshot_.filled;
            sample.fill_latency_ms = elapsed_ms(submitted_at, report.timestamp);
            total_fill_latency_ms_ += sample.fill_latency_ms;
            snapshot_.average_fill_latency_ms = total_fill_latency_ms_ / static_cast<double>(snapshot_.filled);
            break;
        case LiveOrderStatus::Cancelled:
            ++snapshot_.cancelled;
            break;
        case LiveOrderStatus::Rejected:
            ++snapshot_.broker_rejected;
            break;
        case LiveOrderStatus::Error:
            ++snapshot_.errored;
            break;
        default:
            break;
        }

        if (const auto reference = reference_price_for(order, report)) {
            sample.reference_price = *reference;
            sample.signed_slippage_bps = signed_slippage_bps(order.side, report.price, *reference);
            ++snapshot_.slippage_observations;
            total_signed_slippage_bps_ += sample.signed_slippage_bps;
            total_absolute_slippage_bps_ += std::abs(sample.signed_slippage_bps);
            snapshot_.average_signed_slippage_bps = total_signed_slippage_bps_
                / static_cast<double>(snapshot_.slippage_observations);
            snapshot_.average_absolute_slippage_bps = total_absolute_slippage_bps_
                / static_cast<double>(snapshot_.slippage_observations);
        }

        snapshot_.last_timestamp = report.timestamp;
        samples_.push_back(sample);
        update_rejection_rate();
    }

    void ExecutionQualityTracker::reset() {
        snapshot_ = ExecutionQualitySnapshot{};
        samples_.clear();
        total_ack_latency_ms_ = 0.0;
        total_fill_latency_ms_ = 0.0;
        total_signed_slippage_bps_ = 0.0;
        total_absolute_slippage_bps_ = 0.0;
        acknowledged_orders_.clear();
    }

    void ExecutionQualityTracker::update_rejection_rate() {
        if (snapshot_.submitted == 0) {
            snapshot_.rejection_rate = 0.0;
            return;
        }
        const auto rejected = snapshot_.submit_rejected + snapshot_.broker_rejected + snapshot_.errored;
        snapshot_.rejection_rate = static_cast<double>(rejected) / static_cast<double>(snapshot_.submitted);
    }

    std::optional<double> ExecutionQualityTracker::reference_price_for(const LiveOrder& order,
                                                                       const ExecutionReport& report) {
        if (report.price <= 0.0 || report.quantity <= 0.0) {
            return std::nullopt;
        }
        if (order.limit_price > 0.0) {
            return order.limit_price;
        }
        if (order.stop_price > 0.0) {
            return order.stop_price;
        }
        return std::nullopt;
    }

    double ExecutionQualityTracker::signed_slippage_bps(const engine::OrderSide side,
                                                        const double fill_price,
                                                        const double reference_price) {
        if (reference_price <= 0.0) {
            return 0.0;
        }
        const double raw = (fill_price - reference_price) / reference_price * 10000.0;
        return side == engine::OrderSide::Buy ? raw : -raw;
    }
}  // namespace regimeflow::live
