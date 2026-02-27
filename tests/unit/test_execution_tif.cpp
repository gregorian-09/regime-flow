#include "regimeflow/engine/execution_pipeline.h"
#include "regimeflow/events/event_queue.h"
#include "regimeflow/execution/execution_model.h"

#include <gtest/gtest.h>

using regimeflow::Timestamp;
using regimeflow::engine::Order;
using regimeflow::engine::OrderSide;
using regimeflow::engine::OrderType;
using regimeflow::engine::TimeInForce;

namespace {

class PartialExecutionModel final : public regimeflow::execution::ExecutionModel {
public:
    std::vector<regimeflow::engine::Fill> execute(const Order& order,
                                                  regimeflow::Price reference_price,
                                                  Timestamp timestamp) override {
        regimeflow::engine::Fill fill;
        fill.order_id = order.id;
        fill.symbol = order.symbol;
        fill.timestamp = timestamp;
        fill.quantity = (order.quantity / 2.0) * (order.side == OrderSide::Buy ? 1.0 : -1.0);
        fill.price = reference_price;
        fill.id = 1;
        return {fill};
    }
};

}  // namespace

TEST(ExecutionTifTest, IocCancelsRemainder) {
    regimeflow::events::EventQueue queue;
    regimeflow::engine::MarketDataCache market_data;
    regimeflow::engine::OrderBookCache order_books;
    regimeflow::engine::ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_execution_model(std::make_unique<PartialExecutionModel>());

    Order order = Order::market(regimeflow::SymbolRegistry::instance().intern("AAA"),
                                OrderSide::Buy,
                                10.0);
    order.tif = TimeInForce::IOC;
    order.created_at = Timestamp::now();

    pipeline.on_order_submitted(order);

    int fills = 0;
    int cancels = 0;
    while (auto event = queue.pop()) {
        const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
        if (!payload) {
            continue;
        }
        if (payload->kind == regimeflow::events::OrderEventKind::Fill) {
            fills++;
        } else if (payload->kind == regimeflow::events::OrderEventKind::Cancel) {
            cancels++;
        }
    }
    EXPECT_EQ(fills, 1);
    EXPECT_EQ(cancels, 1);
}

TEST(ExecutionTifTest, FokRejectsPartial) {
    regimeflow::events::EventQueue queue;
    regimeflow::engine::MarketDataCache market_data;
    regimeflow::engine::OrderBookCache order_books;
    regimeflow::engine::ExecutionPipeline pipeline(&market_data, &order_books, &queue);
    pipeline.set_execution_model(std::make_unique<PartialExecutionModel>());

    Order order = Order::market(regimeflow::SymbolRegistry::instance().intern("AAA"),
                                OrderSide::Buy,
                                10.0);
    order.tif = TimeInForce::FOK;
    order.created_at = Timestamp::now();

    pipeline.on_order_submitted(order);

    int fills = 0;
    int rejects = 0;
    while (auto event = queue.pop()) {
        const auto* payload = std::get_if<regimeflow::events::OrderEventPayload>(&event->payload);
        if (!payload) {
            continue;
        }
        if (payload->kind == regimeflow::events::OrderEventKind::Fill) {
            fills++;
        } else if (payload->kind == regimeflow::events::OrderEventKind::Reject) {
            rejects++;
        }
    }
    EXPECT_EQ(fills, 0);
    EXPECT_EQ(rejects, 1);
}
