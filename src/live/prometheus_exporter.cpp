#include "regimeflow/live/prometheus_exporter.h"

#if defined(REGIMEFLOW_USE_BOOST_BEAST)
#include <boost/asio.hpp>
#endif

#include <atomic>
#include <sstream>
#include <string_view>
#include <thread>
#include <utility>

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

#if defined(REGIMEFLOW_USE_BOOST_BEAST)
        std::string http_response(const unsigned status,
                                  const std::string_view reason,
                                  const std::string& body,
                                  const std::string_view content_type) {
            std::ostringstream out;
            out << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
                << "Content-Type: " << content_type << "\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << body;
            return out.str();
        }
#endif
    }  // namespace

    struct PrometheusScrapeEndpoint::Impl {
        explicit Impl(PrometheusScrapeEndpointConfig cfg) : config(std::move(cfg)) {}

        PrometheusScrapeEndpointConfig config;
        MetricsProvider provider;
        std::atomic<bool> running{false};
        std::thread thread;
        uint16_t bound_port = 0;

#if defined(REGIMEFLOW_USE_BOOST_BEAST)
        boost::asio::io_context ioc;
        std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;

        void serve() {
            using boost::asio::ip::tcp;
            while (running.load()) {
                tcp::socket socket(ioc);
                boost::system::error_code ec;
                acceptor->accept(socket, ec);
                if (ec) {
                    continue;
                }
                boost::asio::streambuf buffer;
                boost::asio::read_until(socket, buffer, "\r\n\r\n", ec);
                std::string request_line;
                std::istream input(&buffer);
                std::getline(input, request_line);
                if (!request_line.empty() && request_line.back() == '\r') {
                    request_line.pop_back();
                }
                const std::string expected = "GET " + config.path + " ";
                const bool ok = request_line.rfind(expected, 0) == 0;
                const std::string body = ok ? provider() : "not found\n";
                const std::string response = ok
                    ? http_response(200, "OK", body, "text/plain; version=0.0.4")
                    : http_response(404, "Not Found", body, "text/plain");
                boost::asio::write(socket, boost::asio::buffer(response), ec);
            }
        }
#endif
    };

    PrometheusScrapeEndpoint::PrometheusScrapeEndpoint(PrometheusScrapeEndpointConfig config)
        : impl_(std::make_unique<Impl>(std::move(config))) {}

    PrometheusScrapeEndpoint::~PrometheusScrapeEndpoint() {
        stop();
    }

    PrometheusScrapeEndpoint::PrometheusScrapeEndpoint(PrometheusScrapeEndpoint&&) noexcept = default;

    PrometheusScrapeEndpoint& PrometheusScrapeEndpoint::operator=(PrometheusScrapeEndpoint&&) noexcept = default;

    Result<void> PrometheusScrapeEndpoint::start(MetricsProvider provider) {
        if (!provider) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Prometheus metrics provider is empty"));
        }
        if (impl_->running.exchange(true)) {
            return Ok();
        }
        impl_->provider = std::move(provider);
#if defined(REGIMEFLOW_USE_BOOST_BEAST)
        using boost::asio::ip::tcp;
        boost::system::error_code ec;
        const auto address = boost::asio::ip::make_address(impl_->config.host, ec);
        if (ec) {
            impl_->running.store(false);
            return Result<void>(Error(Error::Code::ConfigError, "Invalid Prometheus endpoint host"));
        }
        impl_->acceptor = std::make_unique<tcp::acceptor>(impl_->ioc);
        impl_->acceptor->open(address.is_v6() ? tcp::v6() : tcp::v4(), ec);
        if (!ec) {
            impl_->acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
        }
        if (!ec) {
            impl_->acceptor->bind(tcp::endpoint(address, impl_->config.port), ec);
        }
        if (!ec) {
            impl_->bound_port = impl_->acceptor->local_endpoint().port();
            impl_->acceptor->listen(16, ec);
        }
        if (ec) {
            impl_->running.store(false);
            impl_->acceptor.reset();
            Error error(Error::Code::NetworkError,
                        "Failed to start Prometheus scrape endpoint: " + ec.message());
            error.details = ec.message();
            return Result<void>(std::move(error));
        }
        impl_->thread = std::thread([this] { impl_->serve(); });
        return Ok();
#else
        impl_->running.store(false);
        return Result<void>(Error(Error::Code::InvalidState,
                                  "Prometheus scrape endpoint requires Boost.Asio support"));
#endif
    }

    void PrometheusScrapeEndpoint::stop() {
        if (!impl_ || !impl_->running.exchange(false)) {
            return;
        }
#if defined(REGIMEFLOW_USE_BOOST_BEAST)
        if (impl_->acceptor) {
            boost::system::error_code ec;
            impl_->acceptor->close(ec);
        }
        if (impl_->thread.joinable()) {
            impl_->thread.join();
        }
        impl_->acceptor.reset();
        impl_->bound_port = 0;
#endif
    }

    bool PrometheusScrapeEndpoint::is_running() const {
        return impl_ && impl_->running.load();
    }

    uint16_t PrometheusScrapeEndpoint::port() const {
        return impl_ ? impl_->bound_port : 0;
    }

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
        counter(out, "regimeflow_live_queue_observations_total", "Total fills with queue-model attribution.",
                quality.queue_observations);
        gauge(out, "regimeflow_live_average_queue_position", "Average model-estimated queue position.",
              quality.average_queue_position);
        gauge(out, "regimeflow_live_average_queue_delay_error_ms",
              "Average realized fill latency minus model-estimated queue delay.",
              quality.average_queue_delay_error_ms);
        return out.str();
    }
}  // namespace regimeflow::live
