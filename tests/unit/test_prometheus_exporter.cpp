#include <gtest/gtest.h>

#include "regimeflow/live/prometheus_exporter.h"

namespace regimeflow::test
{
    TEST(PrometheusExporter, ExportsDashboardMetrics) {
        regimeflow::engine::DashboardSnapshot snapshot;
        snapshot.equity = 125000.0;
        snapshot.cash = 25000.0;
        snapshot.open_order_count = 3;
        snapshot.position_count = 2;
        snapshot.fill_count = 7;
        snapshot.event_loop_latency_ms = 4.5;
        snapshot.current_regime.regime = regimeflow::regime::RegimeType::Bear;

        const auto text = regimeflow::live::dashboard_snapshot_to_prometheus(snapshot);

        EXPECT_NE(text.find("# TYPE regimeflow_equity gauge"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_equity 125000"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_open_orders 3"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_fills_total 7"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_current_regime 2"), std::string::npos);
    }

    TEST(PrometheusExporter, ExportsExecutionQualityMetrics) {
        regimeflow::engine::DashboardSnapshot dashboard;
        dashboard.equity = 100000.0;

        regimeflow::live::ExecutionQualitySnapshot quality;
        quality.submitted = 10;
        quality.submit_rejected = 1;
        quality.broker_rejected = 2;
        quality.acknowledged = 7;
        quality.filled = 5;
        quality.rejection_rate = 0.3;
        quality.average_ack_latency_ms = 12.5;
        quality.average_signed_slippage_bps = -1.25;
        quality.queue_observations = 4;
        quality.average_queue_position = 2.5;
        quality.average_queue_delay_error_ms = 7.0;

        const auto text = regimeflow::live::live_metrics_to_prometheus(dashboard, quality);

        EXPECT_NE(text.find("regimeflow_live_orders_submitted_total 10"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_submit_rejected_total 1"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_broker_rejected_total 2"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_rejection_rate 0.3"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_average_ack_latency_ms 12.5"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_average_signed_slippage_bps -1.25"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_queue_observations_total 4"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_average_queue_position 2.5"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_live_average_queue_delay_error_ms 7"), std::string::npos);
    }
}  // namespace regimeflow::test
