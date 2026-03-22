/**
 * @file order_routing.h
 * @brief RegimeFlow order routing declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/engine/order.h"

#include <optional>
#include <string>
#include <vector>

namespace regimeflow::engine
{
    /**
     * @brief Routing mode for smart order handling.
     */
    enum class RoutingMode : uint8_t {
        None,
        Smart
    };

    /**
     * @brief Split execution mode.
     */
    enum class SplitMode : uint8_t {
        None,
        Parallel,
        Sequential
    };

    /**
     * @brief Aggregation behavior for parent orders.
     */
    enum class ParentAggregation : uint8_t {
        None,
        Final,
        Partial
    };

    /**
     * @brief Venue weighting entry.
     */
    struct RoutingVenue {
        std::string name;
        double weight = 1.0;
        std::optional<double> maker_rebate_bps;
        std::optional<double> taker_fee_bps;
        std::optional<double> price_adjustment_bps;
        std::optional<int64_t> latency_ms;
        std::optional<bool> queue_enabled;
        std::optional<double> queue_progress_fraction;
        std::optional<double> queue_default_visible_qty;
        std::optional<std::string> queue_depth_mode;
    };

    /**
     * @brief Routing configuration parameters.
     */
    struct RoutingConfig {
        bool enabled = false;
        RoutingMode mode = RoutingMode::Smart;
        SplitMode split_mode = SplitMode::None;
        ParentAggregation parent_aggregation = ParentAggregation::Partial;

        std::string default_venue = "lit_primary";
        std::vector<RoutingVenue> venues;

        double max_spread_bps = 8.0;
        double passive_offset_bps = 0.0;
        double aggressive_offset_bps = 0.0;

        double min_child_qty = 1.0;
        int max_children = 8;

        [[nodiscard]] bool split_enabled() const {
            return split_mode != SplitMode::None;
        }

        /**
         * @brief Parse routing config from a Config object.
         * @param config Config root (typically execution config).
         * @param prefix Optional prefix (e.g., "routing").
         */
        static RoutingConfig from_config(const Config& config,
                                         const std::string& prefix = "routing");
    };

    /**
     * @brief Snapshot of market data used for routing decisions.
     */
    struct RoutingContext {
        std::optional<Price> bid;
        std::optional<Price> ask;
        std::optional<Price> last;

        [[nodiscard]] std::optional<Price> mid() const;
        [[nodiscard]] std::optional<double> spread_bps() const;
    };

    /**
     * @brief Routing plan output from a router.
     */
    struct RoutingPlan {
        Order routed_order;
        std::vector<Order> children;
        SplitMode split_mode = SplitMode::None;
        ParentAggregation parent_aggregation = ParentAggregation::Partial;
    };

    /**
     * @brief Interface for order routers.
     */
    class OrderRouter {
    public:
        virtual ~OrderRouter() = default;
        /**
         * @brief Build a routing plan for an order.
         * @param order Input order.
         * @param ctx Routing market context.
         * @return Routing plan with modified order and optional children.
         */
        [[nodiscard]] virtual RoutingPlan route(const Order& order,
                                                const RoutingContext& ctx) const = 0;
    };

    /**
     * @brief Simple smart order router using spread heuristics.
     */
    class SmartOrderRouter final : public OrderRouter {
    public:
        explicit SmartOrderRouter(RoutingConfig config);
        [[nodiscard]] RoutingPlan route(const Order& order,
                                        const RoutingContext& ctx) const override;

    private:
        RoutingConfig config_;
    };
}  // namespace regimeflow::engine
