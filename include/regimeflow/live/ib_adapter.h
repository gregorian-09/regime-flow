/**
 * @file ib_adapter.h
 * @brief RegimeFlow regimeflow ib adapter declarations.
 */

#pragma once

#include "regimeflow/live/broker_adapter.h"

#include "DefaultEWrapper.h"
#include "EClientSocket.h"
#include "EReader.h"
#include "EReaderOSSignal.h"
#include "Order.h"
#include "OrderState.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace regimeflow::live {

/**
 * @brief Broker adapter for Interactive Brokers (IB).
 */
class IBAdapter final : public BrokerAdapter, public DefaultEWrapper {
public:
    /**
     * @brief IB adapter configuration.
     */
    struct Config {
        /**
         * @brief TWS/Gateway host.
         */
        std::string host = "127.0.0.1";
        /**
         * @brief TWS/Gateway port.
         */
        int port = 7497;
        /**
         * @brief Client ID.
         */
        int client_id = 1;
    };

    /**
     * @brief Construct an IB adapter.
     * @param config Configuration.
     */
    explicit IBAdapter(Config config);

    /**
     * @brief Connect to TWS/Gateway.
     */
    Result<void> connect() override;
    /**
     * @brief Disconnect from TWS/Gateway.
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
     * @brief Poll the adapter (for IB message pump).
     */
    void poll() override;

private:
    ::Contract build_stock_contract(const std::string& symbol) const;
    ::Order build_order(const engine::Order& order, int64_t order_id) const;

    void nextValidId(OrderId orderId) override;
    void tickPrice(TickerId tickerId, TickType field, double price,
                   const TickAttrib& attrib) override;
    void tickSize(TickerId tickerId, TickType field, Decimal size) override;
    void orderStatus(OrderId orderId, const std::string& status, Decimal filled,
                     Decimal remaining, double avgFillPrice, long long permId, int parentId,
                     double lastFillPrice, int clientId, const std::string& whyHeld,
                     double mktCapPrice) override;
    void openOrder(OrderId orderId, const ::Contract& contract, const ::Order& order,
                   const ::OrderState& orderState) override;
    void openOrderEnd() override;
    void execDetails(int reqId, const ::Contract& contract,
                     const ::Execution& execution) override;
    void position(const std::string& account, const ::Contract& contract,
                  Decimal position, double avgCost) override;
    void positionEnd() override;
    void accountSummary(int reqId, const std::string& account,
                        const std::string& tag, const std::string& value,
                        const std::string& currency) override;
    void accountSummaryEnd(int reqId) override;
    void error(int id, time_t errorTime, int errorCode, const std::string& errorString,
               const std::string& advancedOrderRejectJson) override;

    Config config_;
    std::atomic<bool> connected_{false};
    std::unique_ptr<EReaderOSSignal> reader_signal_;
    std::unique_ptr<EClientSocket> client_;
    std::unique_ptr<EReader> reader_;
    std::thread reader_thread_;

    std::mutex state_mutex_;
    int64_t next_order_id_ = -1;
    std::unordered_map<TickerId, SymbolId> ticker_to_symbol_;
    std::unordered_map<SymbolId, double> last_prices_;
    std::unordered_map<SymbolId, double> last_sizes_;
    std::unordered_map<int64_t, ExecutionReport> open_orders_;
    std::unordered_map<int64_t, ::Contract> order_contracts_;
    std::unordered_map<SymbolId, Position> positions_;
    AccountInfo account_info_{};

    std::function<void(const MarketDataUpdate&)> market_cb_;
    std::function<void(const ExecutionReport&)> exec_cb_;
    std::function<void(const Position&)> position_cb_;
};

}  // namespace regimeflow::live
