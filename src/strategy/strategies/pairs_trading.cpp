#include "regimeflow/strategy/strategies/pairs_trading.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::strategy {

namespace {

double mean(const std::vector<double>& v) {
    if (v.empty()) {
        return 0.0;
    }
    double s = 0.0;
    for (double x : v) {
        s += x;
    }
    return s / static_cast<double>(v.size());
}

double stddev(const std::vector<double>& v, double m) {
    if (v.size() < 2) {
        return 0.0;
    }
    double s = 0.0;
    for (double x : v) {
        double d = x - m;
        s += d * d;
    }
    return std::sqrt(s / static_cast<double>(v.size() - 1));
}

}  // namespace

void PairsTradingStrategy::initialize(StrategyContext& ctx) {
    ctx_ = &ctx;
    load_symbols_from_config();
    if (auto v = ctx.config().get_as<int64_t>("lookback")) {
        lookback_ = static_cast<size_t>(std::max<int64_t>(30, *v));
    }
    if (auto v = ctx.config().get_as<double>("entry_z")) {
        entry_z_ = std::max(0.5, *v);
    }
    if (auto v = ctx.config().get_as<double>("exit_z")) {
        exit_z_ = std::max(0.1, *v);
    }
    if (auto v = ctx.config().get_as<double>("max_z")) {
        max_z_ = std::max(entry_z_, *v);
    }
    if (auto v = ctx.config().get_as<bool>("allow_short")) {
        allow_short_ = *v;
    }
    if (auto v = ctx.config().get_as<int64_t>("base_qty")) {
        base_qty_ = static_cast<Quantity>(std::max<int64_t>(1, *v));
    }
    if (auto v = ctx.config().get_as<double>("min_qty_scale")) {
        min_qty_scale_ = std::max(0.1, *v);
    }
    if (auto v = ctx.config().get_as<double>("max_qty_scale")) {
        max_qty_scale_ = std::max(min_qty_scale_, *v);
    }
    if (auto v = ctx.config().get_as<int64_t>("cooldown_bars")) {
        cooldown_bars_ = static_cast<size_t>(std::max<int64_t>(0, *v));
    }
}

bool PairsTradingStrategy::load_symbols_from_config() {
    if (!ctx_) {
        return false;
    }
    if (auto v = ctx_->config().get_as<std::string>("symbol_a")) {
        symbol_a_ = *v;
    }
    if (auto v = ctx_->config().get_as<std::string>("symbol_b")) {
        symbol_b_ = *v;
    }
    if (!symbol_a_.empty()) {
        symbol_a_id_ = SymbolRegistry::instance().intern(symbol_a_);
    }
    if (!symbol_b_.empty()) {
        symbol_b_id_ = SymbolRegistry::instance().intern(symbol_b_);
    }
    return symbol_a_id_ != 0 && symbol_b_id_ != 0;
}

bool PairsTradingStrategy::compute_spread(double& spread, double& hedge_ratio) const {
    if (!ctx_ || symbol_a_id_ == 0 || symbol_b_id_ == 0) {
        return false;
    }
    auto bars_a = ctx_->recent_bars(symbol_a_id_, lookback_);
    auto bars_b = ctx_->recent_bars(symbol_b_id_, lookback_);
    if (bars_a.size() < lookback_ || bars_b.size() < lookback_) {
        return false;
    }
    size_t n = std::min(bars_a.size(), bars_b.size());
    std::vector<double> a_prices;
    std::vector<double> b_prices;
    a_prices.reserve(n);
    b_prices.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        a_prices.push_back(bars_a[bars_a.size() - n + i].close);
        b_prices.push_back(bars_b[bars_b.size() - n + i].close);
    }
    double mean_a = mean(a_prices);
    double mean_b = mean(b_prices);
    double cov = 0.0;
    double var_b = 0.0;
    for (size_t i = 0; i < n; ++i) {
        cov += (a_prices[i] - mean_a) * (b_prices[i] - mean_b);
        var_b += (b_prices[i] - mean_b) * (b_prices[i] - mean_b);
    }
    hedge_ratio = var_b > 0.0 ? cov / var_b : 1.0;
    spread = a_prices.back() - hedge_ratio * b_prices.back();
    return true;
}

Quantity PairsTradingStrategy::scaled_qty(double zscore) const {
    double abs_z = std::min(std::abs(zscore), max_z_);
    double scale = min_qty_scale_;
    if (max_z_ > entry_z_) {
        double t = (abs_z - entry_z_) / (max_z_ - entry_z_);
        scale = min_qty_scale_ + (max_qty_scale_ - min_qty_scale_) * std::clamp(t, 0.0, 1.0);
    }
    return static_cast<Quantity>(std::max(1.0, std::round(base_qty_ * scale)));
}

void PairsTradingStrategy::submit_spread_trade(double hedge_ratio,
                                               double zscore,
                                               double price_a,
                                               double price_b) {
    if (!ctx_) {
        return;
    }
    Quantity qty = scaled_qty(zscore);
    Quantity qty_b = static_cast<Quantity>(std::max(1.0, std::round(qty * std::abs(hedge_ratio))));

    engine::OrderSide side_a = zscore > 0 ? engine::OrderSide::Sell : engine::OrderSide::Buy;
    engine::OrderSide side_b = zscore > 0 ? engine::OrderSide::Buy : engine::OrderSide::Sell;

    auto order_a = engine::Order::market(symbol_a_id_, side_a, qty);
    auto order_b = engine::Order::market(symbol_b_id_, side_b, qty_b);
    order_a.metadata["strategy"] = "pairs_trading";
    order_b.metadata["strategy"] = "pairs_trading";
    order_a.metadata["zscore"] = std::to_string(zscore);
    order_b.metadata["zscore"] = std::to_string(zscore);
    order_a.metadata["hedge_ratio"] = std::to_string(hedge_ratio);
    order_b.metadata["hedge_ratio"] = std::to_string(hedge_ratio);
    order_a.metadata["price"] = std::to_string(price_a);
    order_b.metadata["price"] = std::to_string(price_b);

    ctx_->submit_order(order_a);
    ctx_->submit_order(order_b);
}

void PairsTradingStrategy::on_bar(const data::Bar& bar) {
    if (!ctx_) {
        return;
    }
    if (symbol_a_id_ == 0 || symbol_b_id_ == 0) {
        load_symbols_from_config();
    }
    if (symbol_a_id_ == symbol_b_id_) {
        return;
    }
    if (bar.symbol != symbol_a_id_ && bar.symbol != symbol_b_id_) {
        return;
    }
    if (bar.symbol == symbol_a_id_) {
        ++bar_index_;
    }
    double spread = 0.0;
    double hedge_ratio = 1.0;
    if (!compute_spread(spread, hedge_ratio)) {
        return;
    }

    auto bars_a = ctx_->recent_bars(symbol_a_id_, lookback_);
    auto bars_b = ctx_->recent_bars(symbol_b_id_, lookback_);
    if (last_signal_index_ + cooldown_bars_ > bar_index_) {
        return;
    }
    size_t n = std::min(bars_a.size(), bars_b.size());
    std::vector<double> spreads;
    spreads.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double s = bars_a[bars_a.size() - n + i].close -
            hedge_ratio * bars_b[bars_b.size() - n + i].close;
        spreads.push_back(s);
    }
    double m = mean(spreads);
    double sd = stddev(spreads, m);
    if (sd <= 0.0) {
        return;
    }
    double zscore = (spread - m) / sd;
    if (!allow_short_ && zscore > 0.0) {
        return;
    }

    auto pos_a = ctx_->portfolio().get_position(symbol_a_id_);
    auto pos_b = ctx_->portfolio().get_position(symbol_b_id_);
    Quantity qty_a = pos_a ? pos_a->quantity : 0;
    Quantity qty_b = pos_b ? pos_b->quantity : 0;

    if (std::abs(zscore) <= exit_z_) {
        if (qty_a != 0 || qty_b != 0) {
            if (qty_a != 0) {
                auto side = qty_a > 0 ? engine::OrderSide::Sell : engine::OrderSide::Buy;
                auto order = engine::Order::market(symbol_a_id_, side, std::abs(qty_a));
                order.metadata["strategy"] = "pairs_trading";
                order.metadata["action"] = "exit";
                ctx_->submit_order(order);
            }
            if (qty_b != 0) {
                auto side = qty_b > 0 ? engine::OrderSide::Sell : engine::OrderSide::Buy;
                auto order = engine::Order::market(symbol_b_id_, side, std::abs(qty_b));
                order.metadata["strategy"] = "pairs_trading";
                order.metadata["action"] = "exit";
                ctx_->submit_order(order);
            }
            last_signal_index_ = bar_index_;
        }
        return;
    }

    if (std::abs(zscore) >= entry_z_) {
        submit_spread_trade(hedge_ratio, zscore, bars_a.back().close, bars_b.back().close);
        last_signal_index_ = bar_index_;
    }
}

}  // namespace regimeflow::strategy
