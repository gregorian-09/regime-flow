#include "regimeflow/strategy/strategies/moving_average_cross.h"

#include <algorithm>

namespace regimeflow::strategy
{
    void MovingAverageCrossStrategy::initialize(StrategyContext& ctx) {
        ctx_ = &ctx;
        if (const auto fast = ctx.get_as<int64_t>("fast_period")) {
            fast_period_ = static_cast<int>(*fast);
        }
        if (const auto slow = ctx.get_as<int64_t>("slow_period")) {
            slow_period_ = static_cast<int>(*slow);
        }
        if (const auto qty = ctx.get_as<double>("quantity")) {
            quantity_ = *qty;
        }
        if (fast_period_ <= 0) fast_period_ = 10;
        if (slow_period_ <= 0) slow_period_ = 30;
        if (fast_period_ >= slow_period_) {
            slow_period_ = fast_period_ + 1;
        }
    }

    void MovingAverageCrossStrategy::on_bar(const data::Bar& bar) {
        auto& history = price_history_[bar.symbol];
        history.push_back(bar.close);
        if (history.size() > static_cast<size_t>(slow_period_)) {
            history.erase(history.begin(), history.begin() + (history.size() - slow_period_));
        }

        if (history.size() < static_cast<size_t>(slow_period_)) {
            return;
        }

        const double fast = compute_sma(bar.symbol, fast_period_);
        const double slow = compute_sma(bar.symbol, slow_period_);

        const auto pos = ctx_->portfolio().get_position(bar.symbol);

        if (const double current = pos ? pos->quantity : 0.0; fast > slow && current <= 0) {
            const double qty = quantity_ > 0 ? quantity_ : 1.0;
            engine::Order order = engine::Order::market(bar.symbol, engine::OrderSide::Buy, qty);
            ctx_->submit_order(std::move(order));
        } else if (fast < slow && current > 0) {
            engine::Order order = engine::Order::market(bar.symbol, engine::OrderSide::Sell, current);
            ctx_->submit_order(std::move(order));
        }
    }

    double MovingAverageCrossStrategy::compute_sma(const SymbolId symbol, const int period) const {
        const auto it = price_history_.find(symbol);
        if (it == price_history_.end() || it->second.size() < static_cast<size_t>(period)) {
            return 0.0;
        }
        const auto& values = it->second;
        double sum = 0.0;
        for (size_t i = values.size() - period; i < values.size(); ++i) {
            sum += values[i];
        }
        return sum / static_cast<double>(period);
    }
}  // namespace regimeflow::strategy
