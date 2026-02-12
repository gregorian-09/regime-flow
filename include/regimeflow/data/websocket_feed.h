/**
 * @file websocket_feed.h
 * @brief RegimeFlow regimeflow websocket feed declarations.
 */

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

/**
 * @brief WebSocket-based live feed with validation and reconnect logic.
 */
class WebSocketFeed final : public LiveFeedAdapter {
public:
    /**
     * @brief Reconnect state snapshot for callbacks.
     */
    struct ReconnectState {
        bool connected = false;
        int attempts = 0;
        int64_t backoff_ms = 0;
        Timestamp last_attempt;
        Timestamp next_attempt;
        std::string last_error;
    };

    /**
     * @brief WebSocket feed configuration.
     */
    struct Config {
        /**
         * @brief WebSocket URL.
         */
        std::string url;
        /**
         * @brief Subscription message template.
         */
        std::string subscribe_template;
        /**
         * @brief Unsubscribe message template.
         */
        std::string unsubscribe_template;
        /**
         * @brief Read timeout in milliseconds.
         */
        int64_t read_timeout_ms = 50;
        /**
         * @brief Enable automatic reconnects.
         */
        bool auto_reconnect = true;
        /**
         * @brief Initial reconnect backoff in milliseconds.
         */
        int64_t reconnect_initial_ms = 500;
        /**
         * @brief Maximum reconnect backoff in milliseconds.
         */
        int64_t reconnect_max_ms = 10'000;
        /**
         * @brief Verify TLS certificates if using wss.
         */
        bool verify_tls = true;
        /**
         * @brief CA bundle path for TLS.
         */
        std::string ca_bundle_path;
        /**
         * @brief Expected server hostname for TLS validation.
         */
        std::string expected_hostname;
        /**
         * @brief Optional override for connection setup.
         */
        std::function<Result<void>()> connect_override;
        /**
         * @brief Validation configuration for incoming data.
         */
        ValidationConfig validation;
        /**
         * @brief Enable schema/value validation on messages.
         */
        bool validate_messages = false;
        /**
         * @brief Enforce strict schema validation when enabled.
         */
        bool strict_schema = true;
    };

    /**
     * @brief Construct a WebSocket feed.
     * @param config Feed configuration.
     */
    explicit WebSocketFeed(Config config);

    /**
     * @brief Connect to the WebSocket endpoint.
     */
    Result<void> connect() override;
    /**
     * @brief Disconnect the WebSocket.
     */
    void disconnect() override;
    /**
     * @brief Check connection status.
     */
    bool is_connected() const override;

    /**
     * @brief Subscribe to symbols.
     */
    void subscribe(const std::vector<std::string>& symbols) override;
    /**
     * @brief Unsubscribe from symbols.
     */
    void unsubscribe(const std::vector<std::string>& symbols) override;

    /**
     * @brief Register a bar callback.
     */
    void on_bar(std::function<void(const Bar&)> cb) override;
    /**
     * @brief Register a tick callback.
     */
    void on_tick(std::function<void(const Tick&)> cb) override;
    /**
     * @brief Register an order book callback.
     */
    void on_book(std::function<void(const OrderBook&)> cb) override;
    /**
     * @brief Register a raw message callback.
     */
    void on_raw(std::function<void(const std::string&)> cb);
    /**
     * @brief Register a reconnect state callback.
     */
    void on_reconnect(std::function<void(const ReconnectState&)> cb);

    /**
     * @brief Validate TLS configuration for secure connections.
     */
    Result<void> validate_tls_config() const;
    /**
     * @brief Process a raw message from the socket.
     * @param message Raw message string.
     */
    void handle_message(const std::string& message);
    /**
     * @brief Send a raw message to the socket.
     * @param message Raw message string.
     * @return Ok on success, error otherwise.
     */
    Result<void> send_raw(const std::string& message);
    /**
     * @brief Poll the socket for new messages.
     */
    void poll() override;

private:
    /**
     * @brief Online running statistics for validation.
     */
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

    /**
     * @brief Per-symbol stream state for validation and checks.
     */
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
