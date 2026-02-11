#pragma once

#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/data/websocket_feed.h"

#include <atomic>
#include <mutex>

namespace regimeflow::live {

class AlpacaAdapter final : public BrokerAdapter {
public:
    struct Config {
        std::string api_key;
        std::string secret_key;
        std::string base_url;
        std::string data_url;
        bool paper = true;
        int timeout_seconds = 10;
        bool enable_streaming = false;
        std::string stream_url;
        std::string stream_auth_template;
        std::string stream_subscribe_template;
        std::string stream_unsubscribe_template;
        std::string stream_ca_bundle_path;
        std::string stream_expected_hostname;
    };

    explicit AlpacaAdapter(Config config);

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
