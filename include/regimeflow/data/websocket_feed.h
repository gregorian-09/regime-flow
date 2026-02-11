#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/live_feed.h"
#include "regimeflow/data/validation_config.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#ifdef REGIMEFLOW_USE_BOOST_BEAST
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#ifdef REGIMEFLOW_USE_OPENSSL
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl.hpp>
#endif
#endif

namespace regimeflow::data {

class WebSocketFeed final : public LiveFeedAdapter {
public:
    struct ReconnectState {
        bool connected = false;
        int attempts = 0;
        int64_t backoff_ms = 0;
        Timestamp last_attempt;
        Timestamp next_attempt;
        std::string last_error;
    };

    struct Config {
        std::string url;
        std::string subscribe_template;
        std::string unsubscribe_template;
        int64_t read_timeout_ms = 50;
        bool auto_reconnect = true;
        int64_t reconnect_initial_ms = 500;
        int64_t reconnect_max_ms = 10'000;
        bool verify_tls = true;
        std::string ca_bundle_path;
        std::string expected_hostname;
        std::function<Result<void>()> connect_override;
        ValidationConfig validation;
        bool validate_messages = false;
        bool strict_schema = true;
    };

    explicit WebSocketFeed(Config config);

    Result<void> connect() override;
    void disconnect() override;
    bool is_connected() const override;

    void subscribe(const std::vector<std::string>& symbols) override;
    void unsubscribe(const std::vector<std::string>& symbols) override;

    void on_bar(std::function<void(const Bar&)> cb) override;
    void on_tick(std::function<void(const Tick&)> cb) override;
    void on_book(std::function<void(const OrderBook&)> cb) override;
    void on_raw(std::function<void(const std::string&)> cb);
    void on_reconnect(std::function<void(const ReconnectState&)> cb);

    Result<void> validate_tls_config() const;
    void handle_message(const std::string& message);
    Result<void> send_raw(const std::string& message);
    void poll() override;

private:
    struct RunningStats {
        size_t count = 0;
        double mean = 0.0;
        double m2 = 0.0;

        void push(double value) {
            ++count;
            double delta = value - mean;
            mean += delta / static_cast<double>(count);
            double delta2 = value - mean;
            m2 += delta * delta2;
        }

        double stddev() const {
            return count > 1 ? std::sqrt(m2 / static_cast<double>(count - 1)) : 0.0;
        }
    };

    struct StreamState {
        Timestamp last_ts;
        bool has_last_ts = false;
        double last_price = 0.0;
        bool has_last_price = false;
        RunningStats price_stats;
        RunningStats volume_stats;
    };

    Config config_;
    bool connected_ = false;
    std::vector<std::string> subscriptions_;
    std::function<void(const Bar&)> bar_cb_;
    std::function<void(const Tick&)> tick_cb_;
    std::function<void(const OrderBook&)> book_cb_;
    std::function<void(const std::string&)> raw_cb_;
    std::function<void(const ReconnectState&)> reconnect_cb_;
    int reconnect_attempts_ = 0;
    Timestamp last_reconnect_attempt_;
    Timestamp next_reconnect_attempt_;
    std::string last_reconnect_error_;
    std::unordered_map<SymbolId, StreamState> bar_state_;
    std::unordered_map<SymbolId, StreamState> tick_state_;
    std::unordered_map<SymbolId, Timestamp> book_last_ts_;

#ifdef REGIMEFLOW_USE_BOOST_BEAST
    boost::asio::io_context ioc_;
    boost::asio::ip::tcp::resolver resolver_{ioc_};
    std::optional<boost::beast::websocket::stream<boost::beast::tcp_stream>> ws_;
#ifdef REGIMEFLOW_USE_OPENSSL
    boost::asio::ssl::context ssl_ctx_{boost::asio::ssl::context::tls_client};
    std::optional<boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>>> ws_tls_;
#endif
    boost::beast::flat_buffer buffer_;
    std::chrono::steady_clock::time_point next_reconnect_{};
    int64_t backoff_ms_ = 0;
    bool use_tls_ = false;
#endif
};

}  // namespace regimeflow::data
