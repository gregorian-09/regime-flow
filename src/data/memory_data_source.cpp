#include "regimeflow/data/memory_data_source.h"

#include "regimeflow/data/merged_iterator.h"

#include <stdexcept>
#include <unordered_set>

namespace regimeflow::data {

VectorBarIterator::VectorBarIterator(std::vector<Bar> bars) : bars_(std::move(bars)) {
    std::sort(bars_.begin(), bars_.end(), [](const Bar& a, const Bar& b) {
        if (a.timestamp == b.timestamp) {
            return a.symbol < b.symbol;
        }
        return a.timestamp < b.timestamp;
    });
}

bool VectorBarIterator::has_next() const {
    return index_ < bars_.size();
}

Bar VectorBarIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more bars");
    }
    return bars_[index_++];
}

void VectorBarIterator::reset() {
    index_ = 0;
}

VectorOrderBookIterator::VectorOrderBookIterator(std::vector<OrderBook> books)
    : books_(std::move(books)) {
    std::sort(books_.begin(), books_.end(), [](const OrderBook& a, const OrderBook& b) {
        if (a.timestamp == b.timestamp) {
            return a.symbol < b.symbol;
        }
        return a.timestamp < b.timestamp;
    });
}

bool VectorOrderBookIterator::has_next() const {
    return index_ < books_.size();
}

OrderBook VectorOrderBookIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more order books");
    }
    return books_[index_++];
}

void VectorOrderBookIterator::reset() {
    index_ = 0;
}

VectorTickIterator::VectorTickIterator(std::vector<Tick> ticks) : ticks_(std::move(ticks)) {
    std::sort(ticks_.begin(), ticks_.end(), [](const Tick& a, const Tick& b) {
        if (a.timestamp == b.timestamp) {
            return a.symbol < b.symbol;
        }
        return a.timestamp < b.timestamp;
    });
}

bool VectorTickIterator::has_next() const {
    return index_ < ticks_.size();
}

Tick VectorTickIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more ticks");
    }
    return ticks_[index_++];
}

void VectorTickIterator::reset() {
    index_ = 0;
}

void MemoryDataSource::add_bars(SymbolId symbol, std::vector<Bar> bars) {
    auto& bucket = bars_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(bars.begin()),
                  std::make_move_iterator(bars.end()));
    std::sort(bucket.begin(), bucket.end(), [](const Bar& a, const Bar& b) {
        return a.timestamp < b.timestamp;
    });
}

void MemoryDataSource::add_ticks(SymbolId symbol, std::vector<Tick> ticks) {
    auto& bucket = ticks_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(ticks.begin()),
                  std::make_move_iterator(ticks.end()));
    std::sort(bucket.begin(), bucket.end(), [](const Tick& a, const Tick& b) {
        return a.timestamp < b.timestamp;
    });
}

void MemoryDataSource::add_order_books(SymbolId symbol, std::vector<OrderBook> books) {
    auto& bucket = books_[symbol];
    bucket.insert(bucket.end(), std::make_move_iterator(books.begin()),
                  std::make_move_iterator(books.end()));
    std::sort(bucket.begin(), bucket.end(), [](const OrderBook& a, const OrderBook& b) {
        return a.timestamp < b.timestamp;
    });
}

void MemoryDataSource::add_symbol_info(SymbolInfo info) {
    symbols_[info.id] = std::move(info);
}

void MemoryDataSource::set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions) {
    adjuster_.add_actions(symbol, std::move(actions));
}

std::vector<SymbolInfo> MemoryDataSource::get_available_symbols() const {
    std::vector<SymbolInfo> info;
    info.reserve(symbols_.size());
    std::unordered_set<SymbolId> seen;
    for (const auto& [id, entry] : symbols_) {
        for (auto alias : adjuster_.aliases_for(id)) {
            if (!seen.insert(alias).second) {
                continue;
            }
            SymbolInfo symbol = entry;
            symbol.id = alias;
            symbol.ticker = SymbolRegistry::instance().lookup(alias);
            info.push_back(std::move(symbol));
        }
    }
    return info;
}

TimeRange MemoryDataSource::get_available_range(SymbolId symbol) const {
    TimeRange range;
    symbol = adjuster_.resolve_symbol(symbol);
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
    auto book_it = books_.find(symbol);
    if (book_it != books_.end() && !book_it->second.empty()) {
        range.start = book_it->second.front().timestamp;
        range.end = book_it->second.back().timestamp;
        return range;
    }
    return range;
}

std::vector<Bar> MemoryDataSource::get_bars(SymbolId symbol, TimeRange range,
                                           BarType) {
    std::vector<Bar> result;
    symbol = adjuster_.resolve_symbol(symbol, range.start);
    auto it = bars_.find(symbol);
    if (it == bars_.end()) {
        return result;
    }
    for (const auto& bar : it->second) {
        if (in_range(bar.timestamp, range)) {
            result.push_back(bar);
        }
    }
    return result;
}

std::vector<Tick> MemoryDataSource::get_ticks(SymbolId symbol, TimeRange range) {
    std::vector<Tick> result;
    symbol = adjuster_.resolve_symbol(symbol, range.start);
    auto it = ticks_.find(symbol);
    if (it == ticks_.end()) {
        return result;
    }
    for (const auto& tick : it->second) {
        if (in_range(tick.timestamp, range)) {
            result.push_back(tick);
        }
    }
    return result;
}

std::vector<OrderBook> MemoryDataSource::get_order_books(SymbolId symbol, TimeRange range) {
    std::vector<OrderBook> result;
    symbol = adjuster_.resolve_symbol(symbol, range.start);
    auto it = books_.find(symbol);
    if (it == books_.end()) {
        return result;
    }
    for (const auto& book : it->second) {
        if (in_range(book.timestamp, range)) {
            result.push_back(book);
        }
    }
    return result;
}

std::unique_ptr<DataIterator> MemoryDataSource::create_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) {
    std::vector<std::unique_ptr<DataIterator>> iterators;
    iterators.reserve(symbols.size());
    for (SymbolId symbol : symbols) {
        auto bars = get_bars(symbol, range, bar_type);
        iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars)));
    }
    return std::make_unique<MergedBarIterator>(std::move(iterators));
}

std::unique_ptr<TickIterator> MemoryDataSource::create_tick_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range) {
    std::vector<std::unique_ptr<TickIterator>> iterators;
    iterators.reserve(symbols.size());
    for (SymbolId symbol : symbols) {
        auto ticks = get_ticks(symbol, range);
        iterators.push_back(std::make_unique<VectorTickIterator>(std::move(ticks)));
    }
    return std::make_unique<MergedTickIterator>(std::move(iterators));
}

std::unique_ptr<OrderBookIterator> MemoryDataSource::create_book_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range) {
    std::vector<std::unique_ptr<OrderBookIterator>> iterators;
    iterators.reserve(symbols.size());
    for (SymbolId symbol : symbols) {
        auto books = get_order_books(symbol, range);
        iterators.push_back(std::make_unique<VectorOrderBookIterator>(std::move(books)));
    }
    return std::make_unique<MergedOrderBookIterator>(std::move(iterators));
}

std::vector<CorporateAction> MemoryDataSource::get_corporate_actions(SymbolId,
                                                                    TimeRange) {
    return {};
}

bool MemoryDataSource::in_range(const Timestamp& ts, TimeRange range) {
    if (range.start.microseconds() == 0 && range.end.microseconds() == 0) {
        return true;
    }
    return range.contains(ts);
}

}  // namespace regimeflow::data
