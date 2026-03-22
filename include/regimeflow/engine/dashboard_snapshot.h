/**
 * @file dashboard_snapshot.h
 * @brief RegimeFlow shared dashboard snapshot declarations.
 */

#pragma once

#include "regimeflow/engine/order.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/regime/types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace regimeflow::engine
{
    struct BacktestResults;

    /**
     * @brief Explicit strategy tester setup metadata.
     */
    struct DashboardSetup {
        std::string strategy_name;
        std::string data_source;
        std::string data_directory;
        std::string file_pattern;
        std::vector<std::string> symbols;
        std::string start_date;
        std::string end_date;
        std::string bar_type;
        std::string execution_model = "basic";
        std::string tick_mode = "synthetic_ticks";
        std::string synthetic_tick_profile = "bar_close";
        std::string bar_mode = "close_only";
        std::string fill_policy = "preserve";
        std::string price_drift_action = "ignore";
        bool routing_enabled = false;
        bool queue_enabled = false;
        bool session_enabled = false;
        bool margin_enabled = false;
        bool financing_enabled = false;
        bool optimization_enabled = false;
    };

    /**
     * @brief Venue-level row for dashboard summary tables.
     */
    struct DashboardVenueSummary {
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
     * @brief Generic order row for dashboard grids.
     */
    struct DashboardOrderSummary {
        OrderId id = 0;
        std::string symbol;
        OrderSide side = OrderSide::Buy;
        OrderType type = OrderType::Market;
        double quantity = 0.0;
        double filled_quantity = 0.0;
        double limit_price = 0.0;
        double stop_price = 0.0;
        double avg_fill_price = 0.0;
        std::string status;
        Timestamp updated_at;
    };

    /**
     * @brief Latest quote summary for dashboard overlays.
     */
    struct DashboardQuoteSummary {
        std::string symbol;
        Timestamp timestamp;
        double bid = 0.0;
        double ask = 0.0;
        double bid_size = 0.0;
        double ask_size = 0.0;
    };

    /**
     * @brief Shared snapshot shape for backtest and live dashboards.
     */
    struct DashboardSnapshot {
        Timestamp timestamp;
        DashboardSetup setup;
        double equity = 0.0;
        double cash = 0.0;
        double buying_power = 0.0;
        double daily_pnl = 0.0;
        double total_return = 0.0;
        double max_drawdown = 0.0;
        double sharpe_ratio = 0.0;
        double sortino_ratio = 0.0;
        double win_rate = 0.0;
        double profit_factor = 0.0;
        double initial_margin = 0.0;
        double maintenance_margin = 0.0;
        double available_funds = 0.0;
        double margin_excess = 0.0;
        double total_commission = 0.0;
        double total_transaction_cost = 0.0;
        double total_cost = 0.0;
        double maker_fill_ratio = 0.0;
        size_t fill_count = 0;
        size_t open_order_count = 0;
        size_t position_count = 0;
        bool margin_call = false;
        bool stop_out = false;
        regime::RegimeState current_regime;
        std::vector<PortfolioSnapshot> equity_curve;
        std::vector<Position> positions;
        std::vector<DashboardOrderSummary> open_orders;
        std::vector<DashboardQuoteSummary> quote_summary;
        std::vector<Fill> recent_fills;
        std::vector<DashboardVenueSummary> venue_summary;
        std::vector<std::string> alerts;
        double cpu_usage_pct = 0.0;
        double memory_mb = 0.0;
        double event_loop_latency_ms = 0.0;
    };

    /**
     * @brief Terminal dashboard tab selection.
     */
    enum class DashboardTab : uint8_t {
        All,
        Setup,
        Report,
        Graph,
        Trades,
        Orders,
        Venues,
        Optimization,
        Journal
    };

    /**
     * @brief Rendering preferences for terminal-style strategy tester views.
     */
    struct DashboardRenderOptions {
        bool ansi_colors = true;
        size_t max_rows = 5;
        DashboardTab active_tab = DashboardTab::All;
    };

    /**
     * @brief Build venue summary rows from fills.
     * @param fills Fills to aggregate.
     * @return Aggregated venue rows.
     */
    [[nodiscard]] std::vector<DashboardVenueSummary> summarize_dashboard_venues(
        const std::vector<Fill>& fills);

    /**
     * @brief Build a dashboard snapshot from backtest results.
     * @param results Backtest results.
     * @return Shared dashboard snapshot.
     */
    [[nodiscard]] DashboardSnapshot make_dashboard_snapshot(const BacktestResults& results);

    /**
     * @brief Serialize a dashboard snapshot as JSON for UI consumers.
     * @param snapshot Snapshot to serialize.
     * @return JSON string.
     */
    [[nodiscard]] std::string dashboard_snapshot_to_json(const DashboardSnapshot& snapshot);
    /**
     * @brief Render a terminal strategy tester view from the shared snapshot.
     * @param snapshot Snapshot to render.
     * @param options Rendering options.
     * @return ANSI-capable terminal string.
     */
    [[nodiscard]] std::string render_dashboard_terminal(
        const DashboardSnapshot& snapshot,
        const DashboardRenderOptions& options = {});
}  // namespace regimeflow::engine
