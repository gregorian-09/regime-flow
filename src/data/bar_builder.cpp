#include "regimeflow/data/bar_builder.h"

namespace regimeflow::data {

BarBuilder::BarBuilder(const Config& config) : config_(config) {}

std::optional<Bar> BarBuilder::process(const Tick& tick) {
    if (!has_state_) {
        reset_state(tick);
        return std::nullopt;
    }

    if (tick.symbol != symbol_) {
        Bar completed = build_bar();
        reset_state(tick);
        return completed;
    }

    switch (config_.type) {
        case BarType::Time_1Min:
        case BarType::Time_5Min:
        case BarType::Time_15Min:
        case BarType::Time_30Min:
        case BarType::Time_1Hour:
        case BarType::Time_4Hour:
        case BarType::Time_1Day: {
            if (is_bar_complete(tick)) {
                Bar completed = build_bar();
                reset_state(tick);
                return completed;
            }
            update_state(tick);
            return std::nullopt;
        }
        case BarType::Volume:
        case BarType::Tick:
        case BarType::Dollar: {
            update_state(tick);
            if (is_bar_complete(tick)) {
                Bar completed = build_bar();
                reset();
                return completed;
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<Bar> BarBuilder::flush() {
    if (!has_state_) {
        return std::nullopt;
    }
    Bar completed = build_bar();
    reset();
    return completed;
}

void BarBuilder::reset() {
    symbol_ = 0;
    bar_start_ = Timestamp();
    open_ = std::numeric_limits<double>::quiet_NaN();
    high_ = std::numeric_limits<double>::lowest();
    low_ = std::numeric_limits<double>::max();
    close_ = 0;
    volume_ = 0;
    tick_count_ = 0;
    dollar_volume_ = 0.0;
    has_state_ = false;
}

bool BarBuilder::is_bar_complete(const Tick& tick) const {
    switch (config_.type) {
        case BarType::Time_1Min:
        case BarType::Time_5Min:
        case BarType::Time_15Min:
        case BarType::Time_30Min:
        case BarType::Time_1Hour:
        case BarType::Time_4Hour:
        case BarType::Time_1Day: {
            int64_t elapsed_ms = (tick.timestamp - bar_start_).total_milliseconds();
            return elapsed_ms >= config_.time_interval_ms;
        }
        case BarType::Volume:
            return volume_ >= config_.volume_threshold;
        case BarType::Tick:
            return tick_count_ >= config_.tick_threshold;
        case BarType::Dollar:
            return dollar_volume_ >= config_.dollar_threshold;
    }
    return false;
}

Bar BarBuilder::build_bar() const {
    Bar bar;
    bar.timestamp = bar_start_;
    bar.symbol = symbol_;
    bar.open = open_;
    bar.high = high_;
    bar.low = low_;
    bar.close = close_;
    bar.volume = volume_;
    if (volume_ > 0) {
        bar.vwap = dollar_volume_ / static_cast<double>(volume_);
    }
    bar.trade_count = tick_count_;
    return bar;
}

void BarBuilder::update_state(const Tick& tick) {
    if (!has_state_) {
        reset_state(tick);
        return;
    }
    close_ = tick.price;
    if (tick.price > high_) high_ = tick.price;
    if (tick.price < low_) low_ = tick.price;
    volume_ += static_cast<Volume>(tick.quantity);
    tick_count_ += 1;
    dollar_volume_ += tick.price * tick.quantity;
}

void BarBuilder::reset_state(const Tick& tick) {
    symbol_ = tick.symbol;
    bar_start_ = tick.timestamp;
    open_ = tick.price;
    high_ = tick.price;
    low_ = tick.price;
    close_ = tick.price;
    volume_ = static_cast<Volume>(tick.quantity);
    tick_count_ = 1;
    dollar_volume_ = tick.price * tick.quantity;
    has_state_ = true;
}

MultiSymbolBarBuilder::MultiSymbolBarBuilder(const BarBuilder::Config& config) : config_(config) {}

std::optional<Bar> MultiSymbolBarBuilder::process(const Tick& tick) {
    auto it = builders_.find(tick.symbol);
    if (it == builders_.end()) {
        it = builders_.emplace(tick.symbol, BarBuilder(config_)).first;
    }
    return it->second.process(tick);
}

std::vector<Bar> MultiSymbolBarBuilder::flush_all() {
    std::vector<Bar> bars;
    for (auto& [symbol, builder] : builders_) {
        if (auto bar = builder.flush()) {
            bars.push_back(*bar);
        }
    }
    return bars;
}

}  // namespace regimeflow::data
