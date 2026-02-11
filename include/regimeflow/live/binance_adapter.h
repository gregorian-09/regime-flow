#pragma once

#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/data/websocket_feed.h"

#include <atomic>
#include <mutex>

namespace regimeflow::live {

class BinanceAdapter final : public BrokerAdapter {
public:
    struct Config {
        std::string api_key;
        std::string secret_key;
        std::string base_url = "https://api.binance.com";
        std::string stream_url = "wss://stream.binance.com:9443/ws";
        std::string stream_subscribe_template =
            "{\"method\":\"SUBSCRIBE\",\"params\":{symbols},\"id\":1}";
        std::string stream_unsubscribe_template =
            "{\"method\":\"UNSUBSCRIBE\",\"params\":{symbols},\"id\":2}";
        std::string stream_ca_bundle_path;
        std::string stream_expected_hostname;
        int timeout_seconds = 10;
        bool enable_streaming = true;
        int64_t recv_window_ms = 5000;
    };

    explicit BinanceAdapter(Config config);

    Result<void> connect() override;
    Result<void> disconnect() override;
    bool is_connected() const override;

    void subscribe_market_data(const std::vector<std::string>& symbols) override;
    void unsubscribe_market_data(const std::vector<std::string>& symbols) override;

    Result<std::string> submit_order(const engine::Order& order) override;
    Result<void> cancel_order(const std::string& broker_order_id) override;
    Result<void> modify_order(const std::string& broker_order_id,
                              const engine::OrderModification& mod) override;

    AccountInfo get_account_info() override;
    std::vector<Position> get_positions() override;
    std::vector<ExecutionReport> get_open_orders() override;

    void on_market_data(std::function<void(const MarketDataUpdate&)> cb) override;
    void on_execution_report(std::function<void(const ExecutionReport&)> cb) override;
    void on_position_update(std::function<void(const Position&)> cb) override;

    int max_orders_per_second() const override;
    int max_messages_per_second() const override;

    void poll() override;

private:
    Result<std::string> rest_get(const std::string& path) const;
    Result<std::string> rest_post(const std::string& path, const std::string& body) const;
    Result<std::string> rest_delete(const std::string& path) const;
    Result<std::string> signed_get(const std::string& path, const std::string& query) const;
    Result<std::string> signed_post(const std::string& path, const std::string& query) const;
    Result<std::string> signed_delete(const std::string& path, const std::string& query) const;

    std::string build_trade_stream_symbol(const std::string& symbol) const;
    void handle_stream_message(const std::string& msg);

    Config config_;
    std::atomic<bool> connected_{false};
    std::mutex mutex_;
    std::vector<std::string> symbols_;
    std::vector<std::string> raw_symbols_;

    std::function<void(const MarketDataUpdate&)> market_cb_;
    std::function<void(const ExecutionReport&)> exec_cb_;
    std::function<void(const Position&)> position_cb_;
    std::unique_ptr<data::WebSocketFeed> stream_;
};

}  // namespace regimeflow::live
