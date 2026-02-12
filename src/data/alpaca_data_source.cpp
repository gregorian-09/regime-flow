#include "regimeflow/data/alpaca_data_source.h"

#include "regimeflow/common/json.h"
#include "regimeflow/common/time.h"
#include "regimeflow/common/types.h"

#include <cctype>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

namespace regimeflow::data {

namespace {

std::string bar_timeframe(BarType bar_type) {
    switch (bar_type) {
        case BarType::Time_1Min: return "1Min";
        case BarType::Time_5Min: return "5Min";
        case BarType::Time_15Min: return "15Min";
        case BarType::Time_30Min: return "30Min";
        case BarType::Time_1Hour: return "1Hour";
        case BarType::Time_4Hour: return "4Hour";
        case BarType::Time_1Day: return "1Day";
        default: return "1Day";
    }
}

std::optional<double> get_number(const common::JsonValue::Object& obj,
                                 const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        auto it = obj.find(key);
        if (it != obj.end()) {
            if (const auto* num = it->second.as_number()) {
                return *num;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string> get_string(const common::JsonValue::Object& obj,
                                      const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        auto it = obj.find(key);
        if (it != obj.end()) {
            if (const auto* str = it->second.as_string()) {
                return *str;
            }
        }
    }
    return std::nullopt;
}

Timestamp parse_alpaca_time(const common::JsonValue& value) {
    if (const auto* num = value.as_number()) {
        double ts = *num;
        if (ts > 1e15) {
            return Timestamp(static_cast<int64_t>(ts / 1000.0));
        }
        if (ts > 1e12) {
            return Timestamp(static_cast<int64_t>(ts * 1000.0));
        }
        if (ts > 1e9) {
            return Timestamp(static_cast<int64_t>(ts * 1'000'000.0));
        }
        return Timestamp(static_cast<int64_t>(ts));
    }
    if (const auto* str = value.as_string()) {
        std::string raw = *str;
        if (!raw.empty() && raw.back() == 'Z') {
            raw.pop_back();
        }
        auto dot = raw.find('.');
        if (dot != std::string::npos) {
            raw = raw.substr(0, dot);
        }
        if (raw.size() == 10) {
            return Timestamp::from_string(raw, "%Y-%m-%d");
        }
        return Timestamp::from_string(raw, "%Y-%m-%dT%H:%M:%S");
    }
    return {};
}

struct BarsPage {
    std::unordered_map<SymbolId, std::vector<Bar>> bars;
    std::string next_page_token;
};

struct TradesPage {
    std::unordered_map<SymbolId, std::vector<Tick>> ticks;
    std::string next_page_token;
};

std::string get_next_page_token(const common::JsonValue::Object& root) {
    auto it = root.find("next_page_token");
    if (it == root.end()) {
        return {};
    }
    if (const auto* str = it->second.as_string()) {
        return *str;
    }
    return {};
}

BarsPage parse_bars_page_json(const std::string& payload) {
    BarsPage out;
    auto parsed = common::parse_json(payload);
    if (parsed.is_err()) {
        return out;
    }
    const auto* root = parsed.value().as_object();
    if (!root) {
        return out;
    }
    out.next_page_token = get_next_page_token(*root);
    auto it = root->find("bars");
    if (it == root->end()) {
        return out;
    }
    const auto* bars_obj = it->second.as_object();
    if (bars_obj) {
        for (const auto& [sym, arr_value] : *bars_obj) {
            const auto* arr = arr_value.as_array();
            if (!arr) {
                continue;
            }
            SymbolId symbol_id = SymbolRegistry::instance().intern(sym);
            auto& bars = out.bars[symbol_id];
            for (const auto& entry : *arr) {
                const auto* obj = entry.as_object();
                if (!obj) {
                    continue;
                }
                Bar bar{};
                bar.symbol = symbol_id;
                auto ts = obj->find("t");
                if (ts == obj->end()) {
                    ts = obj->find("timestamp");
                }
                if (ts != obj->end()) {
                    bar.timestamp = parse_alpaca_time(ts->second);
                }
                bar.open = get_number(*obj, {"o", "open"}).value_or(0.0);
                bar.high = get_number(*obj, {"h", "high"}).value_or(0.0);
                bar.low = get_number(*obj, {"l", "low"}).value_or(0.0);
                bar.close = get_number(*obj, {"c", "close"}).value_or(0.0);
                bar.volume = static_cast<Volume>(
                    get_number(*obj, {"v", "volume"}).value_or(0.0));
                bar.trade_count = static_cast<Volume>(
                    get_number(*obj, {"n", "trade_count"}).value_or(0.0));
                bar.vwap = get_number(*obj, {"vw", "vwap"}).value_or(0.0);
                if (bar.timestamp.microseconds() != 0) {
                    bars.push_back(bar);
                }
            }
        }
    }
    return out;
}

TradesPage parse_trades_page_json(const std::string& payload) {
    TradesPage out;
    auto parsed = common::parse_json(payload);
    if (parsed.is_err()) {
        return out;
    }
    const auto* root = parsed.value().as_object();
    if (!root) {
        return out;
    }
    out.next_page_token = get_next_page_token(*root);
    auto it = root->find("trades");
    if (it == root->end()) {
        return out;
    }
    const auto* trades_obj = it->second.as_object();
    if (!trades_obj) {
        return out;
    }
    for (const auto& [sym, arr_value] : *trades_obj) {
        const auto* arr = arr_value.as_array();
        if (!arr) {
            continue;
        }
        SymbolId symbol_id = SymbolRegistry::instance().intern(sym);
        auto& ticks = out.ticks[symbol_id];
        for (const auto& entry : *arr) {
            const auto* obj = entry.as_object();
            if (!obj) {
                continue;
            }
            Tick tick{};
            tick.symbol = symbol_id;
            auto ts = obj->find("t");
            if (ts == obj->end()) {
                ts = obj->find("timestamp");
            }
            if (ts != obj->end()) {
                tick.timestamp = parse_alpaca_time(ts->second);
            }
            tick.price = get_number(*obj, {"p", "price"}).value_or(0.0);
            tick.quantity = get_number(*obj, {"s", "size", "quantity"}).value_or(0.0);
            if (tick.timestamp.microseconds() != 0) {
                ticks.push_back(tick);
            }
        }
    }
    return out;
}

void merge_bars(std::unordered_map<SymbolId, std::vector<Bar>>& out,
                std::unordered_map<SymbolId, std::vector<Bar>>&& page) {
    for (auto& [symbol, bars] : page) {
        auto& dest = out[symbol];
        dest.insert(dest.end(), bars.begin(), bars.end());
    }
}

void merge_ticks(std::unordered_map<SymbolId, std::vector<Tick>>& out,
                 std::unordered_map<SymbolId, std::vector<Tick>>&& page) {
    for (auto& [symbol, ticks] : page) {
        auto& dest = out[symbol];
        dest.insert(dest.end(), ticks.begin(), ticks.end());
    }
}

}  // namespace

AlpacaDataSource::AlpacaDataSource(Config config)
    : client_(AlpacaDataClient::Config{
          config.api_key,
          config.secret_key,
          config.trading_base_url,
          config.data_base_url,
          config.timeout_seconds,
      }),
      symbols_(std::move(config.symbols)) {
    for (const auto& symbol : symbols_) {
        if (!symbol.empty()) {
            allowed_symbols_.insert(SymbolRegistry::instance().intern(symbol));
        }
    }
}

std::vector<SymbolInfo> AlpacaDataSource::get_available_symbols() const {
    std::vector<SymbolInfo> out;
    auto res = client_.list_assets("active", "us_equity");
    if (res.is_err()) {
        if (symbols_.empty()) {
            return out;
        }
        out.reserve(symbols_.size());
        for (const auto& symbol : symbols_) {
            if (symbol.empty()) {
                continue;
            }
            SymbolInfo info{};
            info.id = SymbolRegistry::instance().intern(symbol);
            info.ticker = symbol;
            out.push_back(info);
        }
        return out;
    }
    auto parsed = common::parse_json(res.value());
    if (parsed.is_err()) {
        if (symbols_.empty()) {
            return out;
        }
        out.reserve(symbols_.size());
        for (const auto& symbol : symbols_) {
            if (symbol.empty()) {
                continue;
            }
            SymbolInfo info{};
            info.id = SymbolRegistry::instance().intern(symbol);
            info.ticker = symbol;
            out.push_back(info);
        }
        return out;
    }
    const auto* arr = parsed.value().as_array();
    if (!arr) {
        return out;
    }
    std::unordered_map<std::string, SymbolInfo> assets;
    assets.reserve(arr->size());
    for (const auto& entry : *arr) {
        const auto* obj = entry.as_object();
        if (!obj) {
            continue;
        }
        auto symbol = get_string(*obj, {"symbol"});
        if (!symbol || symbol->empty()) {
            continue;
        }
        SymbolInfo info{};
        info.id = SymbolRegistry::instance().intern(*symbol);
        info.ticker = *symbol;
        if (auto exch = get_string(*obj, {"exchange"})) {
            info.exchange = *exch;
        }
        if (auto currency = get_string(*obj, {"currency"})) {
            info.currency = *currency;
        }
        assets[*symbol] = info;
    }
    if (symbols_.empty()) {
        out.reserve(assets.size());
        for (const auto& [_, info] : assets) {
            out.push_back(info);
        }
        return out;
    }
    out.reserve(symbols_.size());
    for (const auto& symbol : symbols_) {
        auto it = assets.find(symbol);
        if (it != assets.end()) {
            out.push_back(it->second);
        }
    }
    return out;
}

TimeRange AlpacaDataSource::get_available_range(SymbolId) const {
    return {};
}

std::vector<Bar> AlpacaDataSource::get_bars(SymbolId symbol, TimeRange range,
                                            BarType bar_type) {
    if (!allowed_symbols_.empty() && allowed_symbols_.find(symbol) == allowed_symbols_.end()) {
        return {};
    }
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return {};
    }
    std::string start;
    std::string end;
    if (range.start.microseconds() != 0) {
        start = range.start.to_string("%Y-%m-%dT%H:%M:%S");
    }
    if (range.end.microseconds() != 0) {
        end = range.end.to_string("%Y-%m-%dT%H:%M:%S");
    }
    std::unordered_map<SymbolId, std::vector<Bar>> out;
    std::string page_token;
    std::string last_token;
    constexpr int kMaxPages = 1000;
    for (int page = 0; page < kMaxPages; ++page) {
        auto res = client_.get_bars({symbol_name}, bar_timeframe(bar_type), start, end, 0,
                                    page_token);
        if (res.is_err()) {
            break;
        }
        auto parsed = parse_bars_page_json(res.value());
        merge_bars(out, std::move(parsed.bars));
        if (parsed.next_page_token.empty() || parsed.next_page_token == last_token) {
            break;
        }
        last_token = parsed.next_page_token;
        page_token = parsed.next_page_token;
    }
    auto it = out.find(symbol);
    if (it == out.end()) {
        return {};
    }
    return std::move(it->second);
}

std::vector<Tick> AlpacaDataSource::get_ticks(SymbolId symbol, TimeRange range) {
    if (!allowed_symbols_.empty() && allowed_symbols_.find(symbol) == allowed_symbols_.end()) {
        return {};
    }
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return {};
    }
    std::string start;
    std::string end;
    if (range.start.microseconds() != 0) {
        start = range.start.to_string("%Y-%m-%dT%H:%M:%S");
    }
    if (range.end.microseconds() != 0) {
        end = range.end.to_string("%Y-%m-%dT%H:%M:%S");
    }
    std::unordered_map<SymbolId, std::vector<Tick>> out;
    std::string page_token;
    std::string last_token;
    constexpr int kMaxPages = 1000;
    for (int page = 0; page < kMaxPages; ++page) {
        auto res = client_.get_trades({symbol_name}, start, end, 0, page_token);
        if (res.is_err()) {
            break;
        }
        auto parsed = parse_trades_page_json(res.value());
        merge_ticks(out, std::move(parsed.ticks));
        if (parsed.next_page_token.empty() || parsed.next_page_token == last_token) {
            break;
        }
        last_token = parsed.next_page_token;
        page_token = parsed.next_page_token;
    }
    auto it = out.find(symbol);
    if (it == out.end()) {
        return {};
    }
    return std::move(it->second);
}

std::unique_ptr<DataIterator> AlpacaDataSource::create_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range,
    BarType bar_type) {
    std::vector<std::unique_ptr<DataIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto bars = get_bars(symbol, range, bar_type);
        iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars)));
    }
    return std::make_unique<MergedBarIterator>(std::move(iterators));
}

std::unique_ptr<TickIterator> AlpacaDataSource::create_tick_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range) {
    std::vector<std::unique_ptr<TickIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto ticks = get_ticks(symbol, range);
        iterators.push_back(std::make_unique<VectorTickIterator>(std::move(ticks)));
    }
    return std::make_unique<MergedTickIterator>(std::move(iterators));
}

std::vector<CorporateAction> AlpacaDataSource::get_corporate_actions(SymbolId, TimeRange) {
    return {};
}

}  // namespace regimeflow::data
