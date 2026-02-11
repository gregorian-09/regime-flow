#include "regimeflow/engine/market_data_cache.h"

namespace regimeflow::engine {

void MarketDataCache::update(const data::Bar& bar) {
    latest_bars_[bar.symbol] = bar;
    auto& history = bar_history_[bar.symbol];
    history.push_back(bar);
    if (history.size() > max_history_) {
        history.erase(history.begin(), history.begin() + (history.size() - max_history_));
    }
}

void MarketDataCache::update(const data::Tick& tick) {
    latest_ticks_[tick.symbol] = tick;
}

void MarketDataCache::update(const data::Quote& quote) {
    latest_quotes_[quote.symbol] = quote;
}

std::optional<data::Bar> MarketDataCache::latest_bar(SymbolId symbol) const {
    auto it = latest_bars_.find(symbol);
    if (it == latest_bars_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<data::Tick> MarketDataCache::latest_tick(SymbolId symbol) const {
    auto it = latest_ticks_.find(symbol);
    if (it == latest_ticks_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<data::Quote> MarketDataCache::latest_quote(SymbolId symbol) const {
    auto it = latest_quotes_.find(symbol);
    if (it == latest_quotes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<data::Bar> MarketDataCache::recent_bars(SymbolId symbol, size_t count) const {
    std::vector<data::Bar> result;
    auto it = bar_history_.find(symbol);
    if (it == bar_history_.end()) {
        return result;
    }
    const auto& history = it->second;
    if (count > history.size()) {
        count = history.size();
    }
    result.insert(result.end(), history.end() - static_cast<long>(count), history.end());
    return result;
}

}  // namespace regimeflow::engine
