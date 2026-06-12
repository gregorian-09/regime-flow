#include "regimeflow/live/execution_quality.h"

#include "regimeflow/live/live_order_manager.h"

#include <algorithm>
#include <cmath>
#include <ranges>

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

    void ExecutionQualityTracker::record_reference_quote(const engine::OrderId order_id, const data::Quote& quote) {
        if (order_id == 0 || quote.bid <= 0.0 || quote.ask <= 0.0 || quote.ask < quote.bid) {
            return;
        }
        reference_quotes_[order_id] = ReferenceQuote{quote.bid, quote.ask, quote.timestamp};
    }

    void ExecutionQualityTracker::record_execution_report(const LiveOrder& order,
                                                          const ExecutionReport& report) {
        ExecutionQualitySample sample;
        sample.order_id = order.internal_id;
        sample.broker_order_id = order.broker_order_id;
        sample.symbol = order.symbol.empty() ? report.symbol : order.symbol;
        sample.venue = order.venue.empty() ? "unassigned" : order.venue;
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

        if (report.price > 0.0 && report.quantity > 0.0) {
            if (const auto quote = reference_quotes_.find(order.internal_id); quote != reference_quotes_.end()) {
                const double mid = (quote->second.bid + quote->second.ask) / 2.0;
                if (mid > 0.0) {
                    sample.reference_mid_price = mid;
                    sample.reference_spread_bps = (quote->second.ask - quote->second.bid) / mid * 10000.0;
                    sample.effective_spread_bps = effective_spread_bps(order.side, report.price, mid);
                    ++snapshot_.spread_observations;
                    total_effective_spread_bps_ += sample.effective_spread_bps;
                    snapshot_.average_effective_spread_bps = total_effective_spread_bps_
                        / static_cast<double>(snapshot_.spread_observations);
                }
            }
        }
        if (report.status == LiveOrderStatus::Filled && report.quantity > 0.0) {
            record_venue_fill(sample);
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
        total_effective_spread_bps_ = 0.0;
        acknowledged_orders_.clear();
        reference_quotes_.clear();
        venue_accumulators_.clear();
    }

    void ExecutionQualityTracker::update_rejection_rate() {
        if (snapshot_.submitted == 0) {
            snapshot_.rejection_rate = 0.0;
            return;
        }
        const auto rejected = snapshot_.submit_rejected + snapshot_.broker_rejected + snapshot_.errored;
        snapshot_.rejection_rate = static_cast<double>(rejected) / static_cast<double>(snapshot_.submitted);
    }

    void ExecutionQualityTracker::record_venue_fill(const ExecutionQualitySample& sample) {
        auto& aggregate = venue_accumulators_[sample.venue];
        aggregate.summary.venue = sample.venue;
        ++aggregate.summary.fills;
        aggregate.summary.quantity += sample.quantity;
        aggregate.total_fill_latency_ms += sample.fill_latency_ms;
        aggregate.summary.average_fill_latency_ms =
            aggregate.total_fill_latency_ms / static_cast<double>(aggregate.summary.fills);

        if (sample.reference_price > 0.0) {
            ++aggregate.slippage_observations;
            aggregate.total_signed_slippage_bps += sample.signed_slippage_bps;
            aggregate.summary.average_signed_slippage_bps =
                aggregate.total_signed_slippage_bps / static_cast<double>(aggregate.slippage_observations);
        }
        if (sample.reference_mid_price > 0.0) {
            ++aggregate.spread_observations;
            aggregate.total_effective_spread_bps += sample.effective_spread_bps;
            aggregate.summary.average_effective_spread_bps =
                aggregate.total_effective_spread_bps / static_cast<double>(aggregate.spread_observations);
        }
        rebuild_venue_summaries();
    }

    void ExecutionQualityTracker::rebuild_venue_summaries() {
        snapshot_.venue_summaries.clear();
        snapshot_.venue_summaries.reserve(venue_accumulators_.size());
        for (const auto& aggregate : venue_accumulators_ | std::views::values) {
            snapshot_.venue_summaries.push_back(aggregate.summary);
        }
        std::ranges::sort(snapshot_.venue_summaries, {}, &ExecutionQualityVenueSummary::venue);
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

    double ExecutionQualityTracker::effective_spread_bps(const engine::OrderSide side,
                                                         const double fill_price,
                                                         const double mid_price) {
        if (mid_price <= 0.0) {
            return 0.0;
        }
        const double raw = (fill_price - mid_price) / mid_price * 10000.0;
        return side == engine::OrderSide::Buy ? raw : -raw;
    }
}  // namespace regimeflow::live
