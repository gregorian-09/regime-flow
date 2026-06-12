/**
 * @file execution_quality.h
 * @brief Live execution quality tracking declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/live/broker_adapter.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace regimeflow::live
{
    struct LiveOrder;

    /**
     * @brief One measured execution-quality observation.
     */
    struct ExecutionQualitySample {
        engine::OrderId order_id = 0;
        std::string broker_order_id;
        std::string symbol;
        std::string venue;
        engine::OrderSide side = engine::OrderSide::Buy;
        LiveOrderStatus status = LiveOrderStatus::PendingNew;
        double quantity = 0.0;
        double fill_price = 0.0;
        double reference_price = 0.0;
        double signed_slippage_bps = 0.0;
        double effective_spread_bps = 0.0;
        double reference_mid_price = 0.0;
        double reference_spread_bps = 0.0;
        double ack_latency_ms = 0.0;
        double fill_latency_ms = 0.0;
        Timestamp timestamp;
    };

    /**
     * @brief Venue-level aggregate for comparing routing outcomes.
     */
    struct ExecutionQualityVenueSummary {
        std::string venue;
        uint64_t fills = 0;
        double quantity = 0.0;
        double average_fill_latency_ms = 0.0;
        double average_signed_slippage_bps = 0.0;
        double average_effective_spread_bps = 0.0;
    };

    /**
     * @brief Aggregated execution-quality snapshot for dashboards and audit reports.
     */
    struct ExecutionQualitySnapshot {
        uint64_t submitted = 0;
        uint64_t submit_rejected = 0;
        uint64_t acknowledged = 0;
        uint64_t partially_filled = 0;
        uint64_t filled = 0;
        uint64_t cancelled = 0;
        uint64_t broker_rejected = 0;
        uint64_t errored = 0;
        uint64_t slippage_observations = 0;
        uint64_t spread_observations = 0;
        double average_ack_latency_ms = 0.0;
        double average_fill_latency_ms = 0.0;
        double average_signed_slippage_bps = 0.0;
        double average_absolute_slippage_bps = 0.0;
        double average_effective_spread_bps = 0.0;
        double rejection_rate = 0.0;
        std::vector<ExecutionQualityVenueSummary> venue_summaries;
        Timestamp last_timestamp;
    };

    /**
     * @brief Tracks live order quality without participating in order routing.
     */
    class ExecutionQualityTracker {
    public:
        /**
         * @brief Record that an order was accepted for broker submission.
         */
        void record_submitted(const LiveOrder& order);

        /**
         * @brief Record a local/broker submit rejection before a live order exists.
         */
        void record_submit_rejected(Timestamp timestamp);

        /**
         * @brief Record the quote visible when an order entered the broker boundary.
         */
        void record_reference_quote(engine::OrderId order_id, const data::Quote& quote);

        /**
         * @brief Record a broker execution report after the live order state was updated.
         */
        void record_execution_report(const LiveOrder& order, const ExecutionReport& report);

        /**
         * @brief Return an immutable aggregate snapshot.
         */
        [[nodiscard]] const ExecutionQualitySnapshot& snapshot() const noexcept { return snapshot_; }

        /**
         * @brief Return retained per-report observations.
         */
        [[nodiscard]] const std::vector<ExecutionQualitySample>& samples() const noexcept { return samples_; }

        /**
         * @brief Clear all accumulated metrics.
         */
        void reset();

    private:
        ExecutionQualitySnapshot snapshot_;
        std::vector<ExecutionQualitySample> samples_;
        double total_ack_latency_ms_ = 0.0;
        double total_fill_latency_ms_ = 0.0;
        double total_signed_slippage_bps_ = 0.0;
        double total_absolute_slippage_bps_ = 0.0;
        double total_effective_spread_bps_ = 0.0;
        std::unordered_set<engine::OrderId> acknowledged_orders_;

        struct ReferenceQuote {
            double bid = 0.0;
            double ask = 0.0;
            Timestamp timestamp;
        };
        std::unordered_map<engine::OrderId, ReferenceQuote> reference_quotes_;

        struct VenueAccumulator {
            ExecutionQualityVenueSummary summary;
            double total_fill_latency_ms = 0.0;
            double total_signed_slippage_bps = 0.0;
            double total_effective_spread_bps = 0.0;
            uint64_t slippage_observations = 0;
            uint64_t spread_observations = 0;
        };
        std::unordered_map<std::string, VenueAccumulator> venue_accumulators_;

        void update_rejection_rate();
        void record_venue_fill(const ExecutionQualitySample& sample);
        void rebuild_venue_summaries();
        static std::optional<double> reference_price_for(const LiveOrder& order,
                                                         const ExecutionReport& report);
        static double signed_slippage_bps(engine::OrderSide side,
                                          double fill_price,
                                          double reference_price);
        static double effective_spread_bps(engine::OrderSide side,
                                           double fill_price,
                                           double mid_price);
    };
}  // namespace regimeflow::live
