#include <gtest/gtest.h>

#include "regimeflow/live/prometheus_exporter.h"

#if defined(REGIMEFLOW_USE_BOOST_BEAST)
#include <boost/asio.hpp>
#endif

#include <sstream>

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

    TEST(PrometheusExporter, ServesScrapeEndpoint) {
#if !defined(REGIMEFLOW_USE_BOOST_BEAST)
        GTEST_SKIP() << "Boost.Asio not enabled";
#else
        regimeflow::live::PrometheusScrapeEndpoint endpoint({.host = "127.0.0.1", .port = 0, .path = "/metrics"});
        auto start = endpoint.start([] { return std::string("regimeflow_test_metric 1\n"); });
        if (start.is_err() && start.error().message.find("Operation not permitted") != std::string::npos) {
            GTEST_SKIP() << "Network bind is not permitted in this sandbox";
        }
        ASSERT_TRUE(start.is_ok()) << start.error().to_string();
        ASSERT_TRUE(endpoint.is_running());
        ASSERT_NE(endpoint.port(), 0u);

        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket socket(ioc);
        socket.connect({boost::asio::ip::make_address("127.0.0.1"), endpoint.port()});
        const std::string request = "GET /metrics HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf response;
        boost::system::error_code ec;
        boost::asio::read(socket, response, ec);
        EXPECT_TRUE(ec == boost::asio::error::eof || !ec) << ec.message();
        std::ostringstream out;
        out << &response;
        const auto text = out.str();
        EXPECT_NE(text.find("HTTP/1.1 200 OK"), std::string::npos);
        EXPECT_NE(text.find("regimeflow_test_metric 1"), std::string::npos);
        endpoint.stop();
        EXPECT_FALSE(endpoint.is_running());
#endif
    }
}  // namespace regimeflow::test
