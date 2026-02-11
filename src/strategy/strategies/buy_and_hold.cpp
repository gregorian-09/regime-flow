#include "regimeflow/strategy/strategies/buy_and_hold.h"

#include "regimeflow/common/types.h"

namespace regimeflow::strategy {

void BuyAndHoldStrategy::initialize(StrategyContext& ctx) {
    ctx_ = &ctx;
    auto symbol_opt = ctx.get_as<std::string>("symbol");
    if (symbol_opt) {
        symbol_ = SymbolRegistry::instance().intern(*symbol_opt);
    }
    if (auto qty = ctx.get_as<double>("quantity")) {
        quantity_ = *qty;
    }
}

void BuyAndHoldStrategy::on_bar(const data::Bar& bar) {
    if (entered_) {
        return;
    }
    if (symbol_ != 0 && bar.symbol != symbol_) {
        return;
    }
    if (symbol_ == 0) {
        symbol_ = bar.symbol;
    }

    double qty = quantity_;
    if (qty <= 0) {
        auto pos = ctx_->portfolio().get_position(symbol_);
        if (!pos || pos->quantity == 0) {
            qty = 1.0;
        }
    }

    if (qty > 0) {
        engine::Order order = engine::Order::market(symbol_, engine::OrderSide::Buy, qty);
        ctx_->submit_order(std::move(order));
        entered_ = true;
    }
}

}  // namespace regimeflow::strategy
