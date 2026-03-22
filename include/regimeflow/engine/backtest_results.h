/**
 * @file backtest_results.h
 * @brief RegimeFlow regimeflow backtest results declarations.
 */

#pragma once

#include "regimeflow/engine/audit_log.h"
#include "regimeflow/engine/dashboard_snapshot.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/metrics/metrics_tracker.h"
#include "regimeflow/regime/types.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace regimeflow::engine
{
    /**
     * @brief Dedicated tester journal row for UI/report surfaces.
     */
    struct TesterJournalEntry {
        Timestamp timestamp;
        std::string source;
        std::string category;
        std::string title;
        std::string detail;
    };

    /**
     * @brief Aggregated venue-level execution diagnostics.
     */
    struct VenueFillAnalytics {
        std::string venue;
        size_t fill_count = 0;
        size_t order_count = 0;
        size_t parent_order_count = 0;
        size_t split_fill_count = 0;
        double signed_quantity = 0.0;
        double absolute_quantity = 0.0;
        double notional = 0.0;
        double commission = 0.0;
        double transaction_cost = 0.0;
        double total_cost = 0.0;
        double avg_slippage_bps = 0.0;
        double maker_fill_ratio = 0.0;
    };

    /**
     * @brief Aggregated results of a backtest run.
     */
    struct BacktestResults {
        /**
         * @brief Total return as a fraction (e.g., 0.15 = 15%).
         */
        double total_return = 0.0;
        /**
         * @brief Explicit tester setup used to produce these results.
         */
        DashboardSetup setup;
        /**
         * @brief Maximum drawdown as a fraction.
         */
        double max_drawdown = 0.0;
        /**
         * @brief Detailed performance metrics.
         */
        metrics::MetricsTracker metrics;
        /**
         * @brief Execution fills captured during the run.
         */
        std::vector<engine::Fill> fills;
        /**
         * @brief Regime history captured during the run.
         */
        std::vector<regime::RegimeState> regime_history;
        /**
         * @brief Tester journal events captured during the run.
         */
        std::vector<AuditEvent> journal_events;
        /**
         * @brief Dedicated tester journal rows for UI/report surfaces.
         */
        [[nodiscard]] std::vector<TesterJournalEntry> tester_journal() const;

        /**
         * @brief Build the shared dashboard snapshot for this run.
         */
        [[nodiscard]] DashboardSnapshot dashboard_snapshot() const;
        /**
         * @brief Serialize the shared dashboard snapshot as JSON.
         */
        [[nodiscard]] std::string dashboard_snapshot_json() const;
        /**
         * @brief Render the shared strategy tester snapshot for terminals.
         */
        [[nodiscard]] std::string dashboard_terminal(
            const DashboardRenderOptions& options = {}) const;

        /**
         * @brief Return the latest recorded account snapshot, if any.
         */
        [[nodiscard]] std::optional<engine::PortfolioSnapshot> latest_account_snapshot() const {
            if (const auto& snapshots = metrics.portfolio_snapshots(); !snapshots.empty()) {
                return snapshots.back();
            }
            return std::nullopt;
        }

        /**
         * @brief Aggregate execution diagnostics by venue.
         */
        [[nodiscard]] std::vector<VenueFillAnalytics> venue_analytics() const {
            struct Aggregate {
                VenueFillAnalytics stats;
                std::unordered_set<OrderId> orders;
                std::unordered_set<OrderId> parents;
                double weighted_slippage_bps = 0.0;
                size_t maker_fills = 0;
            };

            std::unordered_map<std::string, Aggregate> by_venue;
            by_venue.reserve(fills.size());
            for (const auto& fill : fills) {
                const std::string venue = fill.venue.empty() ? "unassigned" : fill.venue;
                auto& aggregate = by_venue[venue];
                aggregate.stats.venue = venue;
                ++aggregate.stats.fill_count;
                aggregate.stats.signed_quantity += fill.quantity;
                aggregate.stats.absolute_quantity += std::abs(fill.quantity);
                const double notional = std::abs(fill.price * fill.quantity);
                aggregate.stats.notional += notional;
                aggregate.stats.commission += fill.commission;
                aggregate.stats.transaction_cost += fill.transaction_cost;
                aggregate.stats.total_cost += fill.commission + fill.transaction_cost;
                if (fill.order_id != 0) {
                    aggregate.orders.insert(fill.order_id);
                }
                if (fill.parent_order_id != 0) {
                    aggregate.parents.insert(fill.parent_order_id);
                    ++aggregate.stats.split_fill_count;
                }
                if (fill.is_maker) {
                    ++aggregate.maker_fills;
                }
                const double reference_price = fill.price - fill.slippage;
                if (notional > 0.0 && std::abs(reference_price) > 1e-9) {
                    aggregate.weighted_slippage_bps +=
                        (std::abs(fill.slippage) / std::abs(reference_price)) * 10000.0 * notional;
                }
            }

            std::vector<VenueFillAnalytics> results;
            results.reserve(by_venue.size());
            for (auto& [venue, aggregate] : by_venue) {
                aggregate.stats.order_count = aggregate.orders.size();
                aggregate.stats.parent_order_count = aggregate.parents.size();
                if (aggregate.stats.notional > 0.0) {
                    aggregate.stats.avg_slippage_bps =
                        aggregate.weighted_slippage_bps / aggregate.stats.notional;
                }
                if (aggregate.stats.fill_count > 0) {
                    aggregate.stats.maker_fill_ratio =
                        static_cast<double>(aggregate.maker_fills)
                        / static_cast<double>(aggregate.stats.fill_count);
                }
                results.emplace_back(std::move(aggregate.stats));
            }
            std::ranges::sort(results, {}, &VenueFillAnalytics::venue);
            return results;
        }
    };
}  // namespace regimeflow::engine
