#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/live/types.h"

#include <functional>
#include <string>
#include <vector>

namespace regimeflow::live {

enum class LiveOrderStatus {
    PendingNew,
    New,
    PartiallyFilled,
    Filled,
    PendingCancel,
    Cancelled,
    Rejected,
    Expired,
    Error
};

struct ExecutionReport {
    std::string broker_order_id;
    std::string broker_exec_id;
    std::string symbol;
    engine::OrderSide side = engine::OrderSide::Buy;
    double quantity = 0.0;
    double price = 0.0;
    double commission = 0.0;
    LiveOrderStatus status = LiveOrderStatus::New;
    std::string text;
    Timestamp timestamp;
};

class BrokerAdapter {
public:
    virtual ~BrokerAdapter() = default;

    virtual Result<void> connect() = 0;
    virtual Result<void> disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual void subscribe_market_data(const std::vector<std::string>& symbols) = 0;
    virtual void unsubscribe_market_data(const std::vector<std::string>& symbols) = 0;

    virtual Result<std::string> submit_order(const engine::Order& order) = 0;
    virtual Result<void> cancel_order(const std::string& broker_order_id) = 0;
    virtual Result<void> modify_order(const std::string& broker_order_id,
                                      const engine::OrderModification& mod) = 0;

    virtual AccountInfo get_account_info() = 0;
    virtual std::vector<Position> get_positions() = 0;
    virtual std::vector<ExecutionReport> get_open_orders() = 0;

    virtual void on_market_data(std::function<void(const MarketDataUpdate&)>) = 0;
    virtual void on_execution_report(std::function<void(const ExecutionReport&)>) = 0;
    virtual void on_position_update(std::function<void(const Position&)>) = 0;

    virtual int max_orders_per_second() const = 0;
    virtual int max_messages_per_second() const = 0;

    virtual void poll() = 0;
};

}  // namespace regimeflow::live
