#include "regimeflow/data/mmap_data_source.h"

#include "regimeflow/data/merged_iterator.h"

#include <filesystem>
#include <optional>
#include <unordered_set>

namespace regimeflow::data {

namespace {

std::string build_filename(const std::string& symbol, const std::string& suffix) {
    return symbol + "_" + suffix + ".rfb";
}

std::optional<std::string> extract_symbol(const std::filesystem::path& path) {
    if (path.extension() != ".rfb") {
        return std::nullopt;
    }
    auto stem = path.stem().string();
    auto pos = stem.find_last_of('_');
    if (pos == std::string::npos || pos == 0) {
        return std::nullopt;
    }
    return stem.substr(0, pos);
}

std::string make_range_key(SymbolId symbol, BarType bar_type, TimeRange range) {
    const auto& name = SymbolRegistry::instance().lookup(symbol);
    std::string key = name;
    key.push_back('|');
    key += std::to_string(static_cast<int>(bar_type));
    key.push_back('|');
    key += std::to_string(range.start.microseconds());
    key.push_back(':');
    key += std::to_string(range.end.microseconds());
    return key;
}

}  // namespace

MemoryMappedDataSource::MemoryMappedDataSource(const Config& config)
    : config_(config),
      file_cache_(config.max_cached_files),
      range_cache_(config.max_cached_ranges) {}

std::vector<SymbolInfo> MemoryMappedDataSource::get_available_symbols() const {
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

TimeRange MemoryMappedDataSource::get_available_range(SymbolId symbol) const {
    symbol = adjuster_.resolve_symbol(symbol);
    auto file = get_file(symbol, BarType::Time_1Day);
    if (!file) {
        return {};
    }
    return file->time_range();
}

std::vector<Bar> MemoryMappedDataSource::get_bars(SymbolId symbol, TimeRange range,
                                                  BarType bar_type) {
    symbol = adjuster_.resolve_symbol(symbol, range.start);
    std::string cache_key;
    if (config_.max_cached_ranges > 0) {
        cache_key = make_range_key(symbol, bar_type, range);
        if (auto cached = range_cache_.get(cache_key)) {
            return **cached;
        }
    }

    std::vector<Bar> result;
    auto file = get_file(symbol, bar_type);
    if (!file) {
        return result;
    }
    auto [start, end] = file->find_range(range);
    result.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        result.push_back((*file)[i].to_bar());
    }

    if (config_.max_cached_ranges > 0) {
        auto shared = std::make_shared<std::vector<Bar>>(result);
        range_cache_.put(cache_key, shared);
        return *shared;
    }
    return result;
}

std::vector<Tick> MemoryMappedDataSource::get_ticks(SymbolId, TimeRange) {
    return {};
}

std::unique_ptr<DataIterator> MemoryMappedDataSource::create_iterator(
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

std::vector<CorporateAction> MemoryMappedDataSource::get_corporate_actions(SymbolId,
                                                                          TimeRange) {
    return {};
}

void MemoryMappedDataSource::set_corporate_actions(SymbolId symbol,
                                                   std::vector<CorporateAction> actions) {
    adjuster_.add_actions(symbol, std::move(actions));
}

std::shared_ptr<MemoryMappedDataFile> MemoryMappedDataSource::get_file(SymbolId symbol,
                                                                        BarType bar_type) const {
    if (config_.data_directory.empty()) {
        return nullptr;
    }
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return nullptr;
    }
    std::filesystem::path path = config_.data_directory;
    path /= build_filename(symbol_name, bar_type_suffix(bar_type));
    std::string key = path.string();

    if (auto cached = file_cache_.get(key)) {
        return *cached;
    }

    auto file = std::make_shared<MemoryMappedDataFile>(key);
    if (config_.preload_index) {
        file->preload_index();
    }
    file_cache_.put(key, file);
    return file;
}

std::string MemoryMappedDataSource::bar_type_suffix(BarType type) {
    switch (type) {
        case BarType::Time_1Min: return "1m";
        case BarType::Time_5Min: return "5m";
        case BarType::Time_15Min: return "15m";
        case BarType::Time_30Min: return "30m";
        case BarType::Time_1Hour: return "1h";
        case BarType::Time_4Hour: return "4h";
        case BarType::Time_1Day: return "1d";
        case BarType::Volume: return "volume";
        case BarType::Tick: return "tick";
        case BarType::Dollar: return "dollar";
    }
    return "1d";
}

}  // namespace regimeflow::data
