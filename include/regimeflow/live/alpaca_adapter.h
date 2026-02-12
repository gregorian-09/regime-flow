/**
 * @file alpaca_adapter.h
 * @brief RegimeFlow regimeflow alpaca adapter declarations.
 */

#pragma once

#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/data/websocket_feed.h"

#include <atomic>
#include <mutex>

namespace regimeflow::live {

/**
 * @brief Broker adapter for Alpaca.
 */
class AlpacaAdapter final : public BrokerAdapter {
public:
    /**
     * @brief Alpaca adapter configuration.
     */
    struct Config {
        /**
         * @brief API key.
         */
        std::string api_key;
        /**
         * @brief API secret key.
         */
        std::string secret_key;
        /**
         * @brief Base REST URL.
         */
        std::string base_url;
        /**
         * @brief Data REST URL.
         */
        std::string data_url;
        /**
         * @brief Paper trading flag.
         */
        bool paper = true;
        /**
         * @brief REST timeout in seconds.
         */
        int timeout_seconds = 10;
        /**
         * @brief Enable streaming over WebSocket.
         */
        bool enable_streaming = false;
        /**
         * @brief Streaming WebSocket URL.
         */
        std::string stream_url;
        /**
         * @brief Streaming auth message template.
         */
        std::string stream_auth_template;
        /**
         * @brief Streaming subscribe template.
         */
        std::string stream_subscribe_template;
        /**
         * @brief Streaming unsubscribe template.
         */
        std::string stream_unsubscribe_template;
        /**
         * @brief CA bundle path for TLS.
         */
        std::string stream_ca_bundle_path;
        /**
         * @brief Expected TLS hostname.
         */
        std::string stream_expected_hostname;
    };

    /**
     * @brief Construct an Alpaca adapter.
     * @param config Configuration.
     */
    explicit AlpacaAdapter(Config config);

    /**
     * @brief Connect to Alpaca REST/streaming.
     */
    Result<void> connect() override;
    /**
     * @brief Disconnect from Alpaca.
     */
    Result<void> disconnect() override;
    /**
     * @brief Check connection status.
     */
    bool is_connected() const override;

    void subscribe_market_data(const std::vector<std::string>& symbols) override;
    void unsubscribe_market_data(const std::vector<std::string>& symbols) override;

    /**
     * @brief Submit an order.
     */
    Result<std::string> submit_order(const engine::Order& order) override;
    /**
     * @brief Cancel an order.
     */
    Result<void> cancel_order(const std::string& broker_order_id) override;
    /**
     * @brief Modify an order.
     */
    Result<void> modify_order(const std::string& broker_order_id,
                              const engine::OrderModification& mod) override;

    /**
     * @brief Fetch account info.
     */
    AccountInfo get_account_info() override;
    /**
     * @brief Fetch current positions.
     */
    std::vector<Position> get_positions() override;
    /**
     * @brief Fetch open orders.
     */
    std::vector<ExecutionReport> get_open_orders() override;

    void on_market_data(std::function<void(const MarketDataUpdate&)> cb) override;
    void on_execution_report(std::function<void(const ExecutionReport&)> cb) override;
    void on_position_update(std::function<void(const Position&)> cb) override;

    /**
     * @brief Broker order rate limit.
     */
    int max_orders_per_second() const override;
    /**
     * @brief Broker message rate limit.
     */
    int max_messages_per_second() const override;

    /**
     * @brief Poll the adapter (for REST updates).
     */
    void poll() override;

private:
    Result<std::string> rest_get(const std::string& path) const;
    Result<std::string> rest_post(const std::string& path, const std::string& body) const;
    Result<std::string> rest_delete(const std::string& path) const;

    void handle_stream_message(const std::string& msg);
    static LiveOrderStatus parse_trade_update_status(const std::string& value);

    Config config_;
    std::atomic<bool> connected_{false};
    std::mutex mutex_;
    std::vector<std::string> symbols_;

    std::function<void(const MarketDataUpdate&)> market_cb_;
    std::function<void(const ExecutionReport&)> exec_cb_;
    std::function<void(const Position&)> position_cb_;
    std::unique_ptr<data::WebSocketFeed> stream_;
};

}  // namespace regimeflow::live
