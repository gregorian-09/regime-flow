#include "regimeflow/common/config.h"
#include "regimeflow/common/time.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/events/event.h"
#include "regimeflow/strategy/strategy.h"

#include <iostream>
#include <memory>
#include <string>

using namespace regimeflow;

namespace {
    class ControlDemoStrategy final : public strategy::Strategy {
    public:
        void initialize(strategy::StrategyContext& ctx) override {
            ctx_ = &ctx;
        }

        void on_quote(const data::Quote& quote) override {
            if (submitted_) {
                return;
            }
            engine::Order order;
            order.symbol = quote.symbol;
            order.side = engine::OrderSide::Buy;
            order.type = engine::OrderType::Market;
            order.quantity = 1.0;
            order.tif = engine::TimeInForce::GTC;
            order.created_at = quote.timestamp;
            const auto result = ctx_->submit_order(order);
            if (result.is_ok()) {
                order_id_ = result.value();
                submitted_ = true;
            }
        }

        [[nodiscard]] engine::OrderId order_id() const {
            return order_id_;
        }

    private:
        strategy::StrategyContext* ctx_ = nullptr;
        engine::OrderId order_id_ = 0;
        bool submitted_ = false;
    };

    void print_status(engine::BacktestEngine& engine,
                      const engine::OrderId order_id,
                      const std::string& label) {
        const auto order = engine.order_manager().get_order(order_id);
        if (!order.has_value()) {
            std::cout << label << ": order not found\n";
            return;
        }
        std::string status = "unknown";
        switch (order->status) {
        case engine::OrderStatus::Created:
            status = "created";
            break;
        case engine::OrderStatus::Pending:
            status = "pending";
            break;
        case engine::OrderStatus::PartiallyFilled:
            status = "partially_filled";
            break;
        case engine::OrderStatus::Filled:
            status = "filled";
            break;
        case engine::OrderStatus::Cancelled:
            status = "cancelled";
            break;
        case engine::OrderStatus::Rejected:
            status = "rejected";
            break;
        case engine::OrderStatus::Invalid:
            status = "invalid";
            break;
        }
        std::cout << label << ": " << status;
        if (order->filled_quantity > 0.0) {
            std::cout << " qty=" << order->filled_quantity
                      << " avg_price=" << order->avg_fill_price;
        }
        std::cout << "\n";
    }
}  // namespace

int main() {
    engine::BacktestEngine engine(100000.0, "USD");

    Config exec_cfg;
    exec_cfg.set_path("session.enabled", true);
    exec_cfg.set_path("session.start_hhmm", "09:30");
    exec_cfg.set_path("session.end_hhmm", "16:00");
    exec_cfg.set_path("session.weekdays",
                      ConfigValue::Array{
                          ConfigValue("mon"),
                          ConfigValue("tue"),
                          ConfigValue("wed"),
                          ConfigValue("thu"),
                          ConfigValue("fri"),
                      });
    exec_cfg.set_path("session.closed_dates",
                      ConfigValue::Array{ConfigValue("2020-01-01")});
    engine.configure_execution(exec_cfg);

    auto strategy = std::make_unique<ControlDemoStrategy>();
    auto* strategy_ptr = strategy.get();
    engine.set_strategy(std::move(strategy));

    const auto symbol = SymbolRegistry::instance().intern("DEMO");

    data::Quote holiday_quote;
    holiday_quote.symbol = symbol;
    holiday_quote.bid = 99.0;
    holiday_quote.ask = 100.0;
    holiday_quote.timestamp = Timestamp::from_string("2020-01-01 10:00:00", "%Y-%m-%d %H:%M:%S");
    engine.enqueue(events::make_market_event(holiday_quote));
    (void) engine.step();
    (void) engine.step();

    const auto order_id = strategy_ptr->order_id();
    if (order_id == 0) {
        std::cerr << "error: demo strategy failed to submit an order\n";
        return 1;
    }
    print_status(engine, order_id, "After holiday quote");

    engine.enqueue(events::make_system_event(events::SystemEventKind::TradingHalt,
                                             Timestamp::from_string("2020-01-02 09:29:00",
                                                                    "%Y-%m-%d %H:%M:%S"),
                                             0,
                                             "DEMO"));
    (void) engine.step();

    data::Quote halted_quote = holiday_quote;
    halted_quote.timestamp = Timestamp::from_string("2020-01-02 10:00:00", "%Y-%m-%d %H:%M:%S");
    engine.enqueue(events::make_market_event(halted_quote));
    (void) engine.step();
    print_status(engine, order_id, "During symbol halt");

    engine.enqueue(events::make_system_event(events::SystemEventKind::TradingResume,
                                             Timestamp::from_string("2020-01-02 10:00:30",
                                                                    "%Y-%m-%d %H:%M:%S"),
                                             0,
                                             "DEMO"));
    (void) engine.step();

    data::Quote resumed_quote = holiday_quote;
    resumed_quote.timestamp = Timestamp::from_string("2020-01-02 10:01:00", "%Y-%m-%d %H:%M:%S");
    engine.enqueue(events::make_market_event(resumed_quote));
    (void) engine.step();
    (void) engine.step();
    print_status(engine, order_id, "After resume");

    const auto results = engine.results();
    std::cout << "fills=" << results.fills.size() << "\n";
    if (!results.fills.empty()) {
        const auto& fill = results.fills.back();
        std::cout << "last_fill price=" << fill.price
                  << " timestamp=" << fill.timestamp.to_string("%Y-%m-%d %H:%M:%S")
                  << "\n";
    }

    return 0;
}
