#include "regimeflow/engine/dashboard_snapshot.h"

#include "regimeflow/common/types.h"
#include "regimeflow/engine/backtest_results.h"
#include "regimeflow/metrics/report.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace regimeflow::engine
{
    namespace {
        constexpr double kQuantityEpsilon = 1e-9;

        std::string json_escape(const std::string& value) {
            std::string escaped;
            escaped.reserve(value.size());
            for (const char ch : value) {
                switch (ch) {
                case '\\':
                    escaped += "\\\\";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    escaped.push_back(ch);
                    break;
                }
            }
            return escaped;
        }

        std::string regime_name(const regime::RegimeType regime_type) {
            switch (regime_type) {
            case regime::RegimeType::Bull:
                return "bull";
            case regime::RegimeType::Bear:
                return "bear";
            case regime::RegimeType::Crisis:
                return "crisis";
            case regime::RegimeType::Custom:
                return "custom";
            case regime::RegimeType::Neutral:
            default:
                return "neutral";
            }
        }

        std::string colorize(const std::string& value,
                             const char* code,
                             const bool ansi_colors) {
            if (!ansi_colors || value.empty()) {
                return value;
            }
            return std::string("\033[") + code + "m" + value + "\033[0m";
        }

        std::string format_number(const double value, const int precision = 2) {
            std::ostringstream out;
            out << std::fixed << std::setprecision(precision) << value;
            return out.str();
        }

        std::string format_ratio_pct(const double value) {
            return format_number(value * 100.0, 2) + "%";
        }

        std::string pad_label(const std::string& value, const size_t width = 20) {
            if (value.size() >= width) {
                return value;
            }
            return value + std::string(width - value.size(), ' ');
        }

        void append_metric(std::ostringstream& out,
                           const std::string& label,
                           const std::string& value,
                           const bool ansi_colors,
                           const char* value_color = "37") {
            out << "  "
                << colorize(pad_label(label), "90", ansi_colors)
                << colorize(value, value_color, ansi_colors)
                << "\n";
        }

        const char* tab_name(const DashboardTab tab) {
            switch (tab) {
            case DashboardTab::All:
                return "All";
            case DashboardTab::Setup:
                return "Setup";
            case DashboardTab::Report:
                return "Report";
            case DashboardTab::Graph:
                return "Graph";
            case DashboardTab::Trades:
                return "Trades";
            case DashboardTab::Orders:
                return "Orders";
            case DashboardTab::Venues:
                return "Venues";
            case DashboardTab::Optimization:
                return "Optimization";
            case DashboardTab::Journal:
                return "Journal";
            }
            return "All";
        }

        std::string render_tab_chip(const DashboardTab current,
                                    const DashboardTab active,
                                    const bool ansi_colors) {
            std::string label = tab_name(current);
            if (current == active || (active == DashboardTab::All && current == DashboardTab::All)) {
                label = "[" + label + "]";
                return colorize(label, "1;96", ansi_colors);
            }
            return colorize(label, "90", ansi_colors);
        }
    }  // namespace

    std::vector<DashboardVenueSummary> summarize_dashboard_venues(const std::vector<Fill>& fills) {
        struct Aggregate {
            DashboardVenueSummary stats;
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
            if (notional > 0.0 && std::abs(reference_price) > kQuantityEpsilon) {
                aggregate.weighted_slippage_bps +=
                    (std::abs(fill.slippage) / std::abs(reference_price)) * 10000.0 * notional;
            }
        }

        std::vector<DashboardVenueSummary> results;
        results.reserve(by_venue.size());
        for (auto& [venue, aggregate] : by_venue) {
            (void) venue;
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
        std::ranges::sort(results, {}, &DashboardVenueSummary::venue);
        return results;
    }

    DashboardSnapshot make_dashboard_snapshot(const BacktestResults& results) {
        DashboardSnapshot snapshot;
        snapshot.setup = results.setup;
        snapshot.total_return = results.total_return;
        snapshot.max_drawdown = results.max_drawdown;
        snapshot.fill_count = results.fills.size();
        snapshot.equity_curve = results.metrics.portfolio_snapshots();
        snapshot.venue_summary = summarize_dashboard_venues(results.fills);
        snapshot.recent_fills = results.fills;
        if (snapshot.recent_fills.size() > 10) {
            snapshot.recent_fills.erase(snapshot.recent_fills.begin(),
                                        snapshot.recent_fills.end() - 10);
        }

        if (const auto snapshot_opt = results.latest_account_snapshot()) {
            snapshot.timestamp = snapshot_opt->timestamp;
            snapshot.equity = snapshot_opt->equity;
            snapshot.cash = snapshot_opt->cash;
            snapshot.buying_power = snapshot_opt->buying_power;
            snapshot.initial_margin = snapshot_opt->initial_margin;
            snapshot.maintenance_margin = snapshot_opt->maintenance_margin;
            snapshot.available_funds = snapshot_opt->available_funds;
            snapshot.margin_excess = snapshot_opt->margin_excess;
            snapshot.margin_call = snapshot_opt->margin_call;
            snapshot.stop_out = snapshot_opt->stop_out;
            snapshot.positions.reserve(snapshot_opt->positions.size());
            for (const auto& [symbol, position] : snapshot_opt->positions) {
                (void) symbol;
                snapshot.positions.emplace_back(position);
            }
            snapshot.position_count = snapshot.positions.size();
        } else if (!results.fills.empty()) {
            snapshot.timestamp = results.fills.back().timestamp;
        }

        double total_commission = 0.0;
        double total_transaction_cost = 0.0;
        size_t maker_fills = 0;
        for (const auto& fill : results.fills) {
            total_commission += fill.commission;
            total_transaction_cost += fill.transaction_cost;
            if (fill.is_maker) {
                ++maker_fills;
            }
        }
        snapshot.total_commission = total_commission;
        snapshot.total_transaction_cost = total_transaction_cost;
        snapshot.total_cost = total_commission + total_transaction_cost;
        if (!results.fills.empty()) {
            snapshot.maker_fill_ratio =
                static_cast<double>(maker_fills) / static_cast<double>(results.fills.size());
        }

        const auto report = metrics::build_report(results.metrics, results.fills);
        snapshot.sharpe_ratio = report.performance_summary.sharpe_ratio;
        snapshot.sortino_ratio = report.performance_summary.sortino_ratio;
        snapshot.win_rate = report.performance_summary.win_rate;
        snapshot.profit_factor = report.performance_summary.profit_factor;
        snapshot.current_regime = results.regime_history.empty()
            ? regime::RegimeState{}
            : results.regime_history.back();
        return snapshot;
    }

    std::string dashboard_snapshot_to_json(const DashboardSnapshot& snapshot) {
        std::ostringstream out;
        out << "{"
            << "\"timestamp\":\"" << snapshot.timestamp.to_string() << "\","
            << "\"setup\":{"
            << "\"strategy_name\":\"" << json_escape(snapshot.setup.strategy_name) << "\","
            << "\"data_source\":\"" << json_escape(snapshot.setup.data_source) << "\","
            << "\"symbols\":[";
        for (size_t i = 0; i < snapshot.setup.symbols.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << "\"" << json_escape(snapshot.setup.symbols[i]) << "\"";
        }
        out << "],"
            << "\"start_date\":\"" << json_escape(snapshot.setup.start_date) << "\","
            << "\"end_date\":\"" << json_escape(snapshot.setup.end_date) << "\","
            << "\"bar_type\":\"" << json_escape(snapshot.setup.bar_type) << "\","
            << "\"execution_model\":\"" << json_escape(snapshot.setup.execution_model) << "\","
            << "\"tick_mode\":\"" << json_escape(snapshot.setup.tick_mode) << "\","
            << "\"synthetic_tick_profile\":\"" << json_escape(snapshot.setup.synthetic_tick_profile) << "\","
            << "\"bar_mode\":\"" << json_escape(snapshot.setup.bar_mode) << "\","
            << "\"fill_policy\":\"" << json_escape(snapshot.setup.fill_policy) << "\","
            << "\"price_drift_action\":\"" << json_escape(snapshot.setup.price_drift_action) << "\","
            << "\"routing_enabled\":" << (snapshot.setup.routing_enabled ? "true" : "false") << ","
            << "\"queue_enabled\":" << (snapshot.setup.queue_enabled ? "true" : "false") << ","
            << "\"session_enabled\":" << (snapshot.setup.session_enabled ? "true" : "false") << ","
            << "\"margin_enabled\":" << (snapshot.setup.margin_enabled ? "true" : "false") << ","
            << "\"financing_enabled\":" << (snapshot.setup.financing_enabled ? "true" : "false") << ","
            << "\"optimization_enabled\":" << (snapshot.setup.optimization_enabled ? "true" : "false")
            << "},"
            << "\"headline\":{"
            << "\"equity\":" << snapshot.equity << ","
            << "\"cash\":" << snapshot.cash << ","
            << "\"buying_power\":" << snapshot.buying_power << ","
            << "\"daily_pnl\":" << snapshot.daily_pnl << ","
            << "\"total_return\":" << snapshot.total_return << ","
            << "\"max_drawdown\":" << snapshot.max_drawdown << ","
            << "\"sharpe_ratio\":" << snapshot.sharpe_ratio << ","
            << "\"sortino_ratio\":" << snapshot.sortino_ratio << ","
            << "\"win_rate\":" << snapshot.win_rate << ","
            << "\"profit_factor\":" << snapshot.profit_factor
            << "},"
            << "\"account\":{"
            << "\"initial_margin\":" << snapshot.initial_margin << ","
            << "\"maintenance_margin\":" << snapshot.maintenance_margin << ","
            << "\"available_funds\":" << snapshot.available_funds << ","
            << "\"margin_excess\":" << snapshot.margin_excess << ","
            << "\"margin_call\":" << (snapshot.margin_call ? "true" : "false") << ","
            << "\"stop_out\":" << (snapshot.stop_out ? "true" : "false")
            << "},"
            << "\"execution\":{"
            << "\"fill_count\":" << snapshot.fill_count << ","
            << "\"open_order_count\":" << snapshot.open_order_count << ","
            << "\"position_count\":" << snapshot.position_count << ","
            << "\"total_commission\":" << snapshot.total_commission << ","
            << "\"total_transaction_cost\":" << snapshot.total_transaction_cost << ","
            << "\"total_cost\":" << snapshot.total_cost << ","
            << "\"maker_fill_ratio\":" << snapshot.maker_fill_ratio
            << "},"
            << "\"regime\":{"
            << "\"name\":\"" << regime_name(snapshot.current_regime.regime) << "\","
            << "\"confidence\":" << snapshot.current_regime.confidence
            << "},"
            << "\"telemetry\":{"
            << "\"cpu_usage_pct\":" << snapshot.cpu_usage_pct << ","
            << "\"memory_mb\":" << snapshot.memory_mb << ","
            << "\"event_loop_latency_ms\":" << snapshot.event_loop_latency_ms
            << "},"
            << "\"venue_summary\":[";
        for (size_t i = 0; i < snapshot.venue_summary.size(); ++i) {
            const auto& venue = snapshot.venue_summary[i];
            if (i > 0) {
                out << ",";
            }
            out << "{"
                << "\"venue\":\"" << json_escape(venue.venue) << "\","
                << "\"fill_count\":" << venue.fill_count << ","
                << "\"order_count\":" << venue.order_count << ","
                << "\"parent_order_count\":" << venue.parent_order_count << ","
                << "\"split_fill_count\":" << venue.split_fill_count << ","
                << "\"notional\":" << venue.notional << ","
                << "\"commission\":" << venue.commission << ","
                << "\"transaction_cost\":" << venue.transaction_cost << ","
                << "\"total_cost\":" << venue.total_cost << ","
                << "\"avg_slippage_bps\":" << venue.avg_slippage_bps << ","
                << "\"maker_fill_ratio\":" << venue.maker_fill_ratio
                << "}";
        }
        out << "],"
            << "\"alerts\":[";
        for (size_t i = 0; i < snapshot.alerts.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << "\"" << json_escape(snapshot.alerts[i]) << "\"";
        }
        out << "]}";
        return out.str();
    }

    std::string render_dashboard_terminal(const DashboardSnapshot& snapshot,
                                          const DashboardRenderOptions& options) {
        const auto positive_color = "32";
        const auto caution_color = "33";
        const auto accent_color = "96";
        std::string symbol_summary = "n/a";
        if (!snapshot.setup.symbols.empty()) {
            symbol_summary.clear();
            const auto count = std::min<size_t>(3, snapshot.setup.symbols.size());
            for (size_t i = 0; i < count; ++i) {
                if (i > 0) {
                    symbol_summary += ", ";
                }
                symbol_summary += snapshot.setup.symbols[i];
            }
            if (snapshot.setup.symbols.size() > count) {
                symbol_summary += " +" + std::to_string(snapshot.setup.symbols.size() - count);
            }
        }
        std::ostringstream out;
        const auto render_all = options.active_tab == DashboardTab::All;
        const auto wants = [&](const DashboardTab tab) {
            return render_all || options.active_tab == tab;
        };
        out << colorize("REGIMEFLOW STRATEGY TESTER", "1;96", options.ansi_colors) << "\n";
        out << render_tab_chip(DashboardTab::All, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Setup, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Report, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Graph, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Trades, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Orders, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Venues, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Optimization, options.active_tab, options.ansi_colors)
            << " | " << render_tab_chip(DashboardTab::Journal, options.active_tab, options.ansi_colors)
            << "\n";
        out << colorize(std::string(78, '='), "90", options.ansi_colors) << "\n";
        append_metric(out, "Timestamp", snapshot.timestamp.to_string(), options.ansi_colors);
        append_metric(out,
                      "Regime",
                      regime_name(snapshot.current_regime.regime) + " ("
                          + format_number(snapshot.current_regime.confidence, 2) + ")",
                      options.ansi_colors,
                      accent_color);

        if (wants(DashboardTab::Setup)) {
            out << colorize("SETUP", "1;97", options.ansi_colors) << "\n";
            append_metric(out,
                          "Strategy",
                          snapshot.setup.strategy_name.empty() ? "n/a" : snapshot.setup.strategy_name,
                          options.ansi_colors);
            append_metric(out, "Symbols", symbol_summary, options.ansi_colors);
            append_metric(out,
                          "Date Range",
                          (snapshot.setup.start_date.empty() ? "n/a" : snapshot.setup.start_date) + " -> "
                              + (snapshot.setup.end_date.empty() ? "n/a" : snapshot.setup.end_date),
                          options.ansi_colors);
            append_metric(out,
                          "Bar Type",
                          snapshot.setup.bar_type.empty() ? "n/a" : snapshot.setup.bar_type,
                          options.ansi_colors);
            append_metric(out,
                          "Execution",
                          snapshot.setup.execution_model + " / " + snapshot.setup.tick_mode,
                          options.ansi_colors);
            append_metric(out,
                          "Simulation",
                          snapshot.setup.synthetic_tick_profile + " / " + snapshot.setup.bar_mode,
                          options.ansi_colors);
            append_metric(out,
                          "Policy",
                          snapshot.setup.fill_policy + " / " + snapshot.setup.price_drift_action,
                          options.ansi_colors);
        }

        if (wants(DashboardTab::Report)) {
            out << colorize("SUMMARY", "1;97", options.ansi_colors) << "\n";
            append_metric(out, "Equity", format_number(snapshot.equity), options.ansi_colors);
            append_metric(out, "Cash", format_number(snapshot.cash), options.ansi_colors);
            append_metric(out, "Buying Power", format_number(snapshot.buying_power), options.ansi_colors);
            append_metric(out,
                          "Total Return",
                          format_ratio_pct(snapshot.total_return),
                          options.ansi_colors,
                          snapshot.total_return >= 0.0 ? positive_color : caution_color);
            append_metric(out, "Max Drawdown", format_ratio_pct(snapshot.max_drawdown), options.ansi_colors);
            append_metric(out,
                          "Sharpe / Sortino",
                          format_number(snapshot.sharpe_ratio, 2) + " / "
                              + format_number(snapshot.sortino_ratio, 2),
                          options.ansi_colors);
            append_metric(out,
                          "Win Rate / PF",
                          format_ratio_pct(snapshot.win_rate) + " / "
                              + format_number(snapshot.profit_factor, 2),
                          options.ansi_colors);

            out << colorize("ACCOUNT", "1;97", options.ansi_colors) << "\n";
            append_metric(out, "Initial Margin", format_number(snapshot.initial_margin), options.ansi_colors);
            append_metric(out,
                          "Maintenance Margin",
                          format_number(snapshot.maintenance_margin),
                          options.ansi_colors);
            append_metric(out,
                          "Available Funds",
                          format_number(snapshot.available_funds),
                          options.ansi_colors);
            append_metric(out, "Margin Excess", format_number(snapshot.margin_excess), options.ansi_colors);
            append_metric(out,
                          "Margin Flags",
                          std::string(snapshot.margin_call ? "margin_call " : "")
                              + (snapshot.stop_out ? "stop_out" : (!snapshot.margin_call ? "clear" : "")),
                          options.ansi_colors,
                          snapshot.margin_call || snapshot.stop_out ? caution_color : positive_color);

            out << colorize("EXECUTION", "1;97", options.ansi_colors) << "\n";
            append_metric(out,
                          "Fills / Orders",
                          std::to_string(snapshot.fill_count) + " / "
                              + std::to_string(snapshot.open_order_count),
                          options.ansi_colors);
            append_metric(out,
                          "Positions",
                          std::to_string(snapshot.position_count),
                          options.ansi_colors);
            append_metric(out,
                          "Commission / Tx Cost",
                          format_number(snapshot.total_commission) + " / "
                              + format_number(snapshot.total_transaction_cost),
                          options.ansi_colors);
            append_metric(out, "Total Cost", format_number(snapshot.total_cost), options.ansi_colors);
            append_metric(out,
                          "Maker Fill Ratio",
                          format_ratio_pct(snapshot.maker_fill_ratio),
                          options.ansi_colors);
        }

        if (wants(DashboardTab::Graph)) {
            out << colorize("GRAPH", "1;97", options.ansi_colors) << "\n";
            if (snapshot.equity_curve.empty()) {
                out << "  " << colorize("No equity history", "90", options.ansi_colors) << "\n";
            } else {
                const auto count = std::min(options.max_rows, snapshot.equity_curve.size());
                const auto start = snapshot.equity_curve.size() - count;
                for (size_t i = start; i < snapshot.equity_curve.size(); ++i) {
                    const auto& point = snapshot.equity_curve[i];
                    out << "  "
                        << colorize(point.timestamp.to_string("%H:%M:%S"), "90", options.ansi_colors)
                        << " equity=" << format_number(point.equity)
                        << " cash=" << format_number(point.cash)
                        << "\n";
                }
            }
        }

        if (wants(DashboardTab::Trades)) {
            out << colorize("TRADES", "1;97", options.ansi_colors) << "\n";
            if (snapshot.recent_fills.empty()) {
                out << "  " << colorize("No fills", "90", options.ansi_colors) << "\n";
            } else {
                const auto count = std::min(options.max_rows, snapshot.recent_fills.size());
                for (size_t i = 0; i < count; ++i) {
                    const auto& fill = snapshot.recent_fills[snapshot.recent_fills.size() - count + i];
                    std::string symbol = "unknown";
                    try {
                        symbol = SymbolRegistry::instance().lookup(fill.symbol);
                    } catch (...) {
                    }
                    out << "  "
                        << colorize(symbol, accent_color, options.ansi_colors)
                        << " qty=" << format_number(fill.quantity)
                        << " px=" << format_number(fill.price)
                        << " venue=" << (fill.venue.empty() ? "unassigned" : fill.venue)
                        << "\n";
                }
            }
        }

        if (wants(DashboardTab::Orders)) {
            out << colorize("OPEN ORDERS", "1;97", options.ansi_colors) << "\n";
            if (snapshot.open_orders.empty()) {
                out << "  " << colorize("No open orders", "90", options.ansi_colors) << "\n";
            } else {
                const auto count = std::min(options.max_rows, snapshot.open_orders.size());
                for (size_t i = 0; i < count; ++i) {
                    const auto& order = snapshot.open_orders[i];
                    out << "  "
                        << colorize("#" + std::to_string(order.id), accent_color, options.ansi_colors)
                        << " " << order.symbol
                        << " qty=" << format_number(order.quantity)
                        << " filled=" << format_number(order.filled_quantity)
                        << " status=" << order.status << "\n";
                }
                if (snapshot.open_orders.size() > count) {
                    out << "  " << colorize("...", "90", options.ansi_colors) << "\n";
                }
            }
        }

        if (wants(DashboardTab::Venues)) {
            out << colorize("VENUES", "1;97", options.ansi_colors) << "\n";
            if (snapshot.venue_summary.empty()) {
                out << "  " << colorize("No venue fills", "90", options.ansi_colors) << "\n";
            } else {
                const auto count = std::min(options.max_rows, snapshot.venue_summary.size());
                for (size_t i = 0; i < count; ++i) {
                    const auto& venue = snapshot.venue_summary[i];
                    out << "  "
                        << colorize(venue.venue, accent_color, options.ansi_colors)
                        << " fills=" << venue.fill_count
                        << " maker=" << format_ratio_pct(venue.maker_fill_ratio)
                        << " cost=" << format_number(venue.total_cost)
                        << " slip=" << format_number(venue.avg_slippage_bps, 2) << "bps"
                        << "\n";
                }
                if (snapshot.venue_summary.size() > count) {
                    out << "  " << colorize("...", "90", options.ansi_colors) << "\n";
                }
            }
        }

        if (wants(DashboardTab::Optimization)) {
            out << colorize("OPTIMIZATION", "1;97", options.ansi_colors) << "\n";
            out << "  "
                << colorize(snapshot.setup.optimization_enabled
                                ? "Optimization metadata available in Python strategy tester views"
                                : "No optimization payload for current run",
                            snapshot.setup.optimization_enabled ? accent_color : "90",
                            options.ansi_colors)
                << "\n";
        }

        if (wants(DashboardTab::Journal)) {
            out << colorize("JOURNAL", "1;97", options.ansi_colors) << "\n";
            if (snapshot.alerts.empty()) {
                out << "  " << colorize("No alerts", "90", options.ansi_colors) << "\n";
            } else {
                const auto count = std::min(options.max_rows, snapshot.alerts.size());
                for (size_t i = 0; i < count; ++i) {
                    out << "  " << colorize(snapshot.alerts[i], caution_color, options.ansi_colors) << "\n";
                }
                if (snapshot.alerts.size() > count) {
                    out << "  " << colorize("...", "90", options.ansi_colors) << "\n";
                }
            }
        }
        return out.str();
    }

    DashboardSnapshot BacktestResults::dashboard_snapshot() const {
        return make_dashboard_snapshot(*this);
    }

    std::string BacktestResults::dashboard_snapshot_json() const {
        return dashboard_snapshot_to_json(dashboard_snapshot());
    }

    std::string BacktestResults::dashboard_terminal(const DashboardRenderOptions& options) const {
        return render_dashboard_terminal(dashboard_snapshot(), options);
    }
}  // namespace regimeflow::engine
