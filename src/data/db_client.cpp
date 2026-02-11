#include "regimeflow/data/db_client.h"

#include <algorithm>

namespace regimeflow::data {

void InMemoryDbClient::add_bars(SymbolId symbol, std::vector<Bar> bars) {
    auto& bucket = bars_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(bars.begin()),
                  std::make_move_iterator(bars.end()));
    std::sort(bucket.begin(), bucket.end(), [](const Bar& a, const Bar& b) {
        return a.timestamp < b.timestamp;
    });
}

void InMemoryDbClient::add_ticks(SymbolId symbol, std::vector<Tick> ticks) {
    auto& bucket = ticks_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(ticks.begin()),
                  std::make_move_iterator(ticks.end()));
    std::sort(bucket.begin(), bucket.end(), [](const Tick& a, const Tick& b) {
        return a.timestamp < b.timestamp;
    });
}

void InMemoryDbClient::add_symbol_info(SymbolInfo info) {
    symbols_[info.id] = std::move(info);
}

void InMemoryDbClient::add_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions) {
    auto& bucket = actions_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(actions.begin()),
                  std::make_move_iterator(actions.end()));
    std::sort(bucket.begin(), bucket.end(), [](const CorporateAction& a, const CorporateAction& b) {
        return a.effective_date < b.effective_date;
    });
}

void InMemoryDbClient::add_order_books(SymbolId symbol, std::vector<OrderBook> books) {
    auto& bucket = books_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(books.begin()),
                  std::make_move_iterator(books.end()));
    std::sort(bucket.begin(), bucket.end(), [](const OrderBook& a, const OrderBook& b) {
        return a.timestamp < b.timestamp;
    });
}

std::vector<Bar> InMemoryDbClient::query_bars(SymbolId symbol, TimeRange range, BarType) {
    std::vector<Bar> result;
    auto it = bars_.find(symbol);
    if (it == bars_.end()) {
        return result;
    }
    for (const auto& bar : it->second) {
        if (range.contains(bar.timestamp)) {
            result.push_back(bar);
        }
    }
    return result;
}

std::vector<Tick> InMemoryDbClient::query_ticks(SymbolId symbol, TimeRange range) {
    std::vector<Tick> result;
    auto it = ticks_.find(symbol);
    if (it == ticks_.end()) {
        return result;
    }
    for (const auto& tick : it->second) {
        if (range.contains(tick.timestamp)) {
            result.push_back(tick);
        }
    }
    return result;
}

std::vector<SymbolInfo> InMemoryDbClient::list_symbols() {
    std::vector<SymbolInfo> out;
    out.reserve(symbols_.size());
    for (const auto& [id, info] : symbols_) {
        out.push_back(info);
    }
    return out;
}

TimeRange InMemoryDbClient::get_available_range(SymbolId symbol) {
    TimeRange range;
    auto bar_it = bars_.find(symbol);
    if (bar_it != bars_.end() && !bar_it->second.empty()) {
        range.start = bar_it->second.front().timestamp;
        range.end = bar_it->second.back().timestamp;
        return range;
    }
    auto tick_it = ticks_.find(symbol);
    if (tick_it != ticks_.end() && !tick_it->second.empty()) {
        range.start = tick_it->second.front().timestamp;
        range.end = tick_it->second.back().timestamp;
        return range;
    }
    return range;
}

std::vector<CorporateAction> InMemoryDbClient::query_corporate_actions(SymbolId symbol,
                                                                       TimeRange range) {
    std::vector<CorporateAction> result;
    auto it = actions_.find(symbol);
    if (it == actions_.end()) {
        return result;
    }
    for (const auto& action : it->second) {
        if (range.contains(action.effective_date)) {
            result.push_back(action);
        }
    }
    return result;
}

std::vector<OrderBook> InMemoryDbClient::query_order_books(SymbolId symbol, TimeRange range) {
    std::vector<OrderBook> result;
    auto it = books_.find(symbol);
    if (it == books_.end()) {
        return result;
    }
    for (const auto& book : it->second) {
        if (range.contains(book.timestamp)) {
            result.push_back(book);
        }
    }
    return result;
}

}  // namespace regimeflow::data
