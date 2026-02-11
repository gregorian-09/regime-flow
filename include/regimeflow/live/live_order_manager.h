#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/regime/types.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::live {

struct LiveOrder {
    engine::OrderId internal_id = 0;
    std::string broker_order_id;
    std::string broker_exec_id;

    std::string symbol;
    engine::OrderSide side = engine::OrderSide::Buy;
    engine::OrderType type = engine::OrderType::Market;
    double quantity = 0.0;
    double filled_quantity = 0.0;
    double limit_price = 0.0;
    double stop_price = 0.0;
    double avg_fill_price = 0.0;

    LiveOrderStatus status = LiveOrderStatus::PendingNew;
    std::string status_message;

    Timestamp created_at;
    Timestamp submitted_at;
    Timestamp acked_at;
    Timestamp filled_at;

    std::string strategy_id;
    regime::RegimeType regime_at_creation = regime::RegimeType::Neutral;
};

class LiveOrderManager {
public:
    explicit LiveOrderManager(BrokerAdapter* broker);

    Result<engine::OrderId> submit_order(const engine::Order& order);
    Result<void> cancel_order(engine::OrderId id);
    Result<void> cancel_all_orders();
    Result<void> cancel_orders(const std::string& symbol);
    Result<void> modify_order(engine::OrderId id, const engine::OrderModification& mod);

    std::optional<LiveOrder> get_order(engine::OrderId id) const;
    std::vector<LiveOrder> get_open_orders() const;
    std::vector<LiveOrder> get_orders_by_status(LiveOrderStatus status) const;

    void on_execution_report(std::function<void(const ExecutionReport&)> cb);
    void on_order_update(std::function<void(const LiveOrder&)> cb);

    void handle_execution_report(const ExecutionReport& report);
    Result<void> reconcile_with_broker();
    bool validate_order(const engine::Order& order) const;
    std::optional<engine::OrderId> find_order_id_by_broker_id(
        const std::string& broker_order_id) const;

private:
    void update_order_state(LiveOrder& order, const ExecutionReport& report);

    std::unordered_map<engine::OrderId, LiveOrder> orders_;
    BrokerAdapter* broker_ = nullptr;
    engine::OrderId next_order_id_ = 1;

    std::vector<std::function<void(const ExecutionReport&)>> exec_callbacks_;
    std::vector<std::function<void(const LiveOrder&)>> order_callbacks_;
};

}  // namespace regimeflow::live
