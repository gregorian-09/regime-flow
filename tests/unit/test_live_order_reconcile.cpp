#include <gtest/gtest.h>

#include "regimeflow/live/live_order_manager.h"

namespace regimeflow::test {

class TestBrokerAdapter final : public regimeflow::live::BrokerAdapter {
public:
    Result<void> connect() override { return Ok(); }
    Result<void> disconnect() override { return Ok(); }
    bool is_connected() const override { return true; }

    void subscribe_market_data(const std::vector<std::string>&) override {}
    void unsubscribe_market_data(const std::vector<std::string>&) override {}

    Result<std::string> submit_order(const regimeflow::engine::Order&) override {
        return std::string("BRK-1");
    }
    Result<void> cancel_order(const std::string&) override { return Ok(); }
    Result<void> modify_order(const std::string&, const regimeflow::engine::OrderModification&) override {
        return Ok();
    }

    regimeflow::live::AccountInfo get_account_info() override { return {}; }
    std::vector<regimeflow::live::Position> get_positions() override { return {}; }
    std::vector<regimeflow::live::ExecutionReport> get_open_orders() override { return open_orders_; }

    void on_market_data(std::function<void(const regimeflow::live::MarketDataUpdate&)>) override {}
    void on_execution_report(std::function<void(const regimeflow::live::ExecutionReport&)>) override {}
    void on_position_update(std::function<void(const regimeflow::live::Position&)>) override {}

    int max_orders_per_second() const override { return 1; }
    int max_messages_per_second() const override { return 1; }

    void poll() override {}

    std::vector<regimeflow::live::ExecutionReport> open_orders_;
};

TEST(LiveOrderManager, ReconcileAddsMissingBrokerOrders) {
    TestBrokerAdapter broker;
    regimeflow::live::ExecutionReport report;
    report.broker_order_id = "BRK-42";
    report.symbol = "AAA";
    report.quantity = 10;
    report.price = 100;
    report.status = regimeflow::live::LiveOrderStatus::New;
    report.timestamp = regimeflow::Timestamp::now();
    broker.open_orders_.push_back(report);

    regimeflow::live::LiveOrderManager manager(&broker);
    auto res = manager.reconcile_with_broker();
    ASSERT_TRUE(res.is_ok());

    auto orders = manager.get_open_orders();
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0].broker_order_id, "BRK-42");
}

TEST(LiveOrderManager, InvalidTransitionSetsErrorStatus) {
    TestBrokerAdapter broker;
    regimeflow::live::LiveOrderManager manager(&broker);

    regimeflow::engine::Order order;
    order.symbol = SymbolRegistry::instance().intern("TRANSITION");
    order.quantity = 1;
    order.side = regimeflow::engine::OrderSide::Buy;
    auto submit = manager.submit_order(order);
    ASSERT_TRUE(submit.is_ok());

    auto live_order = manager.get_order(submit.value());
    ASSERT_TRUE(live_order.has_value());

    regimeflow::live::ExecutionReport cancelled;
    cancelled.broker_order_id = live_order->broker_order_id;
    cancelled.symbol = "TRANSITION";
    cancelled.quantity = 1;
    cancelled.price = 100;
    cancelled.status = regimeflow::live::LiveOrderStatus::Cancelled;
    cancelled.timestamp = regimeflow::Timestamp::now();
    manager.handle_execution_report(cancelled);

    regimeflow::live::ExecutionReport new_report = cancelled;
    new_report.status = regimeflow::live::LiveOrderStatus::New;
    manager.handle_execution_report(new_report);

    auto updated = manager.get_order(submit.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, regimeflow::live::LiveOrderStatus::Error);
}

}  // namespace regimeflow::test
