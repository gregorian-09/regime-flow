/**
 * @file binance_adapter.h
 * @brief RegimeFlow regimeflow binance adapter declarations.
 */

#pragma once

#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/data/websocket_feed.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace regimeflow::live
{
    /**
     * @brief Broker adapter for Binance.
     */
    class BinanceAdapter final : public BrokerAdapter {
    public:
        /**
         * @brief Binance adapter configuration.
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
            std::string base_url = "https://api.binance.com";
            /**
             * @brief Streaming WebSocket URL.
             */
            std::string stream_url = "wss://stream.binance.com:9443/ws";
            /**
             * @brief Streaming subscribe template.
             */
            std::string stream_subscribe_template =
                R"({"method":"SUBSCRIBE","params":{symbols},"id":1})";
            /**
             * @brief Streaming unsubscribe template.
             */
            std::string stream_unsubscribe_template =
                R"({"method":"UNSUBSCRIBE","params":{symbols},"id":2})";
            /**
             * @brief CA bundle path for TLS.
             */
            std::string stream_ca_bundle_path;
            /**
             * @brief Expected TLS hostname.
             */
            std::string stream_expected_hostname;
            /**
             * @brief REST timeout in seconds.
             */
            int timeout_seconds = 10;
            /**
             * @brief Enable streaming feed.
             */
            bool enable_streaming = true;
            /**
             * @brief Receive window in milliseconds.
             */
            int64_t recv_window_ms = 5000;
        };

        /**
         * @brief Construct a Binance adapter.
         * @param config Configuration.
         */
        explicit BinanceAdapter(Config config);

        /**
         * @brief Connect to Binance REST/streaming.
         */
        Result<void> connect() override;
        /**
         * @brief Disconnect from Binance.
         */
        Result<void> disconnect() override;
        /**
         * @brief Check connection status.
         */
        [[nodiscard]] bool is_connected() const override;

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
        [[nodiscard]] int max_orders_per_second() const override;
        /**
         * @brief Broker message rate limit.
         */
        [[nodiscard]] int max_messages_per_second() const override;
        [[nodiscard]] bool supports_tif(engine::OrderType type,
                                        engine::TimeInForce tif) const override;

        /**
         * @brief Poll the adapter (for REST updates).
         */
        void poll() override;

    private:
        [[nodiscard]] Result<std::string> rest_get(const std::string& path) const;
        [[nodiscard]] Result<std::string> rest_post(const std::string& path, const std::string& body) const;
        [[nodiscard]] Result<std::string> rest_delete(const std::string& path) const;
        [[nodiscard]] Result<std::string> signed_get(const std::string& path, const std::string& query) const;
        [[nodiscard]] Result<std::string> signed_post(const std::string& path, const std::string& query) const;
        [[nodiscard]] Result<std::string> signed_delete(const std::string& path, const std::string& query) const;

        [[nodiscard]] std::string build_trade_stream_symbol(const std::string& symbol) const;
        [[nodiscard]] std::string resolve_balance_symbol(const std::string& asset) const;
        [[nodiscard]] std::optional<double> fetch_public_price(const std::string& symbol) const;
        void handle_stream_message(const std::string& msg) const;

        Config config_;
        std::atomic<bool> connected_{false};
        mutable std::mutex mutex_;
        std::vector<std::string> symbols_;
        std::vector<std::string> raw_symbols_;
        std::unordered_map<std::string, std::string> order_symbols_;

        std::function<void(const MarketDataUpdate&)> market_cb_;
        std::function<void(const ExecutionReport&)> exec_cb_;
        std::function<void(const Position&)> position_cb_;
        std::unique_ptr<data::WebSocketFeed> stream_;
    };
}  // namespace regimeflow::live
