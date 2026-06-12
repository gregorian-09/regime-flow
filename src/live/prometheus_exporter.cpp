#include "regimeflow/live/prometheus_exporter.h"

#include <sstream>
#include <string_view>

namespace regimeflow::live
{
    namespace {
        void metric(std::ostringstream& out,
                    const std::string_view name,
                    const std::string_view help,
                    const std::string_view type,
                    const double value) {
            out << "# HELP " << name << ' ' << help << '\n';
            out << "# TYPE " << name << ' ' << type << '\n';
            out << name << ' ' << value << "\n\n";
        }

        void counter(std::ostringstream& out,
                     const std::string_view name,
                     const std::string_view help,
                     const uint64_t value) {
            metric(out, name, help, "counter", static_cast<double>(value));
        }

        void gauge(std::ostringstream& out,
                   const std::string_view name,
                   const std::string_view help,
                   const double value) {
            metric(out, name, help, "gauge", value);
        }
    }  // namespace

    std::string dashboard_snapshot_to_prometheus(const engine::DashboardSnapshot& snapshot) {
        std::ostringstream out;
        gauge(out, "regimeflow_equity", "Current portfolio equity.", snapshot.equity);
        gauge(out, "regimeflow_cash", "Current portfolio cash.", snapshot.cash);
        gauge(out, "regimeflow_buying_power", "Current account buying power.", snapshot.buying_power);
        gauge(out, "regimeflow_daily_pnl", "Current live daily PnL.", snapshot.daily_pnl);
        gauge(out, "regimeflow_total_return", "Current total return.", snapshot.total_return);
        gauge(out, "regimeflow_max_drawdown", "Current max drawdown.", snapshot.max_drawdown);
        gauge(out, "regimeflow_open_orders", "Current open order count.",
              static_cast<double>(snapshot.open_order_count));
        gauge(out, "regimeflow_positions", "Current position count.",
              static_cast<double>(snapshot.position_count));
        counter(out, "regimeflow_fills_total", "Total observed fills.",
                static_cast<uint64_t>(snapshot.fill_count));
        gauge(out, "regimeflow_cpu_usage_pct", "Current process CPU usage percentage.",
              snapshot.cpu_usage_pct);
        gauge(out, "regimeflow_memory_mb", "Current process memory usage in megabytes.",
              snapshot.memory_mb);
        gauge(out, "regimeflow_event_loop_latency_ms", "Current event-loop latency in milliseconds.",
              snapshot.event_loop_latency_ms);
        gauge(out, "regimeflow_current_regime", "Current numeric regime identifier.",
              static_cast<double>(snapshot.current_regime.regime));
        return out.str();
    }

    std::string live_metrics_to_prometheus(const engine::DashboardSnapshot& dashboard,
                                           const ExecutionQualitySnapshot& quality) {
        std::ostringstream out;
        out << dashboard_snapshot_to_prometheus(dashboard);
        counter(out, "regimeflow_live_orders_submitted_total", "Total live orders submitted.",
                quality.submitted);
        counter(out, "regimeflow_live_submit_rejected_total", "Total local or broker submit rejections.",
                quality.submit_rejected);
        counter(out, "regimeflow_live_broker_rejected_total", "Total broker order rejections.",
                quality.broker_rejected);
        counter(out, "regimeflow_live_orders_acknowledged_total", "Total live orders acknowledged.",
                quality.acknowledged);
        counter(out, "regimeflow_live_orders_partially_filled_total", "Total partial-fill reports.",
                quality.partially_filled);
        counter(out, "regimeflow_live_orders_filled_total", "Total filled live orders.",
                quality.filled);
        counter(out, "regimeflow_live_orders_cancelled_total", "Total cancelled live orders.",
                quality.cancelled);
        counter(out, "regimeflow_live_order_errors_total", "Total errored live orders.",
                quality.errored);
        gauge(out, "regimeflow_live_rejection_rate", "Live order rejection rate.",
              quality.rejection_rate);
        gauge(out, "regimeflow_live_average_ack_latency_ms", "Average broker acknowledgement latency.",
              quality.average_ack_latency_ms);
        gauge(out, "regimeflow_live_average_fill_latency_ms", "Average broker fill latency.",
              quality.average_fill_latency_ms);
        gauge(out, "regimeflow_live_average_signed_slippage_bps", "Average signed reference-price slippage.",
              quality.average_signed_slippage_bps);
        gauge(out, "regimeflow_live_average_absolute_slippage_bps", "Average absolute reference-price slippage.",
              quality.average_absolute_slippage_bps);
        gauge(out, "regimeflow_live_average_effective_spread_bps", "Average fill cost versus submit-time quote midpoint.",
              quality.average_effective_spread_bps);
        return out.str();
    }
}  // namespace regimeflow::live
