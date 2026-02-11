#include "regimeflow/data/order_book_mmap_data_source.h"

#include "regimeflow/data/merged_iterator.h"

#include <filesystem>
#include <optional>
#include <unordered_set>

namespace regimeflow::data {

namespace {

std::optional<std::string> extract_symbol(const std::filesystem::path& path) {
    if (path.extension() != ".rfob") {
        return std::nullopt;
    }
    return path.stem().string();
}

std::string make_range_key(SymbolId symbol, TimeRange range) {
    const auto& name = SymbolRegistry::instance().lookup(symbol);
    std::string key = name;
    key.push_back('|');
    key += std::to_string(range.start.microseconds());
    key.push_back(':');
    key += std::to_string(range.end.microseconds());
    return key;
}

}  // namespace

OrderBookMmapDataSource::OrderBookMmapDataSource(const Config& config)
    : config_(config),
      file_cache_(config.max_cached_files),
      range_cache_(config.max_cached_ranges) {}

std::vector<SymbolInfo> OrderBookMmapDataSource::get_available_symbols() const {
    std::vector<SymbolInfo> symbols;
    if (config_.data_directory.empty()) {
        return symbols;
    }
    std::unordered_set<std::string> seen;
    std::unordered_set<SymbolId> seen_ids;
    for (const auto& entry : std::filesystem::directory_iterator(config_.data_directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto symbol = extract_symbol(entry.path());
        if (!symbol) {
            continue;
        }
        if (!seen.insert(*symbol).second) {
            continue;
        }
        SymbolInfo info;
        info.id = SymbolRegistry::instance().intern(*symbol);
        info.ticker = *symbol;
        for (auto alias : adjuster_.aliases_for(info.id)) {
            if (!seen_ids.insert(alias).second) {
                continue;
            }
            SymbolInfo entry_info = info;
            entry_info.id = alias;
            entry_info.ticker = SymbolRegistry::instance().lookup(alias);
            symbols.push_back(std::move(entry_info));
        }
    }
    return symbols;
}

TimeRange OrderBookMmapDataSource::get_available_range(SymbolId symbol) const {
    symbol = adjuster_.resolve_symbol(symbol);
    auto file = get_file(symbol);
    if (!file) {
        return {};
    }
    return file->time_range();
}

std::vector<Bar> OrderBookMmapDataSource::get_bars(SymbolId, TimeRange, BarType) {
    return {};
}

std::vector<Tick> OrderBookMmapDataSource::get_ticks(SymbolId, TimeRange) {
    return {};
}

std::vector<OrderBook> OrderBookMmapDataSource::get_order_books(SymbolId symbol, TimeRange range) {
    symbol = adjuster_.resolve_symbol(symbol, range.start);
    std::string cache_key;
    if (config_.max_cached_ranges > 0) {
        cache_key = make_range_key(symbol, range);
        if (auto cached = range_cache_.get(cache_key)) {
            return **cached;
        }
    }

    std::vector<OrderBook> result;
    auto file = get_file(symbol);
    if (!file) {
        return result;
    }
    auto [start, end] = file->find_range(range);
    result.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        result.push_back(file->at(i));
    }
    if (config_.max_cached_ranges > 0) {
        auto shared = std::make_shared<std::vector<OrderBook>>(result);
        range_cache_.put(cache_key, shared);
        return *shared;
    }
    return result;
}

std::unique_ptr<DataIterator> OrderBookMmapDataSource::create_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range,
    BarType bar_type) {
    std::vector<std::unique_ptr<DataIterator>> iterators;
    iterators.reserve(symbols.size());
    for (SymbolId symbol : symbols) {
        auto bars = get_bars(symbol, range, bar_type);
        iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars)));
    }
    return std::make_unique<MergedBarIterator>(std::move(iterators));
}

std::unique_ptr<TickIterator> OrderBookMmapDataSource::create_tick_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range) {
    std::vector<std::unique_ptr<TickIterator>> iterators;
    iterators.reserve(symbols.size());
    for (SymbolId symbol : symbols) {
        auto ticks = get_ticks(symbol, range);
        iterators.push_back(std::make_unique<VectorTickIterator>(std::move(ticks)));
    }
    return std::make_unique<MergedTickIterator>(std::move(iterators));
}

std::unique_ptr<OrderBookIterator> OrderBookMmapDataSource::create_book_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range) {
    std::vector<std::unique_ptr<OrderBookIterator>> iterators;
    iterators.reserve(symbols.size());
    for (SymbolId symbol : symbols) {
        auto books = get_order_books(symbol, range);
        iterators.push_back(std::make_unique<VectorOrderBookIterator>(std::move(books)));
    }
    return std::make_unique<MergedOrderBookIterator>(std::move(iterators));
}

std::vector<CorporateAction> OrderBookMmapDataSource::get_corporate_actions(SymbolId,
                                                                           TimeRange) {
    return {};
}

void OrderBookMmapDataSource::set_corporate_actions(SymbolId symbol,
                                                    std::vector<CorporateAction> actions) {
    adjuster_.add_actions(symbol, std::move(actions));
}

std::shared_ptr<OrderBookMmapFile> OrderBookMmapDataSource::get_file(SymbolId symbol) const {
    if (config_.data_directory.empty()) {
        return nullptr;
    }
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return nullptr;
    }
    std::filesystem::path path = config_.data_directory;
    path /= symbol_name + ".rfob";
    std::string key = path.string();

    if (auto cached = file_cache_.get(key)) {
        return *cached;
    }

    auto file = std::make_shared<OrderBookMmapFile>(key);
    file_cache_.put(key, file);
    return file;
}

}  // namespace regimeflow::data
