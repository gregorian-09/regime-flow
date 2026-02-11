#include "regimeflow/data/symbol_metadata.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace regimeflow::data {

namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string normalize(std::string value) {
    value = trim(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<std::string> split_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string token;
    for (char ch : line) {
        if (ch == delimiter) {
            fields.push_back(token);
            token.clear();
        } else {
            token.push_back(ch);
        }
    }
    fields.push_back(token);
    return fields;
}

std::optional<double> parse_double(const std::string& raw) {
    std::string value = trim(raw);
    if (value.empty()) {
        return std::nullopt;
    }
    try {
        return std::stod(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<AssetClass> parse_asset_class(const std::string& raw) {
    std::string value = normalize(raw);
    if (value.empty()) {
        return std::nullopt;
    }
    if (value == "equity" || value == "stock") {
        return AssetClass::Equity;
    }
    if (value == "futures" || value == "future") {
        return AssetClass::Futures;
    }
    if (value == "forex" || value == "fx") {
        return AssetClass::Forex;
    }
    if (value == "crypto" || value == "cryptocurrency") {
        return AssetClass::Crypto;
    }
    if (value == "options" || value == "option") {
        return AssetClass::Options;
    }
    return AssetClass::Other;
}

bool has_any_metadata(const SymbolInfo& info) {
    return !info.exchange.empty() || !info.currency.empty() || info.tick_size != 0.0 ||
           info.lot_size != 0.0 || info.multiplier != 0.0 || !info.sector.empty() ||
           !info.industry.empty();
}

void merge_metadata(SymbolInfo& info, const SymbolMetadata& metadata, bool overwrite) {
    if (metadata.exchange && !metadata.exchange->empty() &&
        (overwrite || info.exchange.empty())) {
        info.exchange = *metadata.exchange;
    }
    if (metadata.asset_class &&
        (overwrite || (!has_any_metadata(info) && info.asset_class == AssetClass::Equity))) {
        info.asset_class = *metadata.asset_class;
    }
    if (metadata.currency && !metadata.currency->empty() &&
        (overwrite || info.currency.empty())) {
        info.currency = *metadata.currency;
    }
    if (metadata.tick_size && (overwrite || info.tick_size == 0.0)) {
        info.tick_size = *metadata.tick_size;
    }
    if (metadata.lot_size && (overwrite || info.lot_size == 0.0)) {
        info.lot_size = *metadata.lot_size;
    }
    if (metadata.multiplier && (overwrite || info.multiplier == 0.0)) {
        info.multiplier = *metadata.multiplier;
    }
    if (metadata.sector && !metadata.sector->empty() &&
        (overwrite || info.sector.empty())) {
        info.sector = *metadata.sector;
    }
    if (metadata.industry && !metadata.industry->empty() &&
        (overwrite || info.industry.empty())) {
        info.industry = *metadata.industry;
    }
}

}  // namespace

SymbolMetadataMap load_symbol_metadata_csv(const std::string& path,
                                           char delimiter,
                                           bool has_header) {
    SymbolMetadataMap output;
    if (path.empty()) {
        return output;
    }
    std::ifstream file(path);
    if (!file) {
        return output;
    }
    std::string header_line;
    std::vector<std::string> headers;
    if (has_header && std::getline(file, header_line)) {
        auto raw_headers = split_line(header_line, delimiter);
        headers.reserve(raw_headers.size());
        for (auto& h : raw_headers) {
            headers.push_back(normalize(h));
        }
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        auto fields = split_line(line, delimiter);
        SymbolMetadata meta;
        for (size_t i = 0; i < fields.size(); ++i) {
            std::string key;
            if (!headers.empty() && i < headers.size()) {
                key = headers[i];
            } else {
                continue;
            }
            std::string value = trim(fields[i]);
            if (key == "symbol" || key == "ticker") {
                meta.ticker = value;
            } else if (key == "exchange") {
                meta.exchange = value;
            } else if (key == "asset_class" || key == "assetclass") {
                meta.asset_class = parse_asset_class(value);
            } else if (key == "currency") {
                meta.currency = value;
            } else if (key == "tick_size") {
                meta.tick_size = parse_double(value);
            } else if (key == "lot_size") {
                meta.lot_size = parse_double(value);
            } else if (key == "multiplier") {
                meta.multiplier = parse_double(value);
            } else if (key == "sector") {
                meta.sector = value;
            } else if (key == "industry") {
                meta.industry = value;
            }
        }
        if (!meta.ticker.empty()) {
            output[meta.ticker] = std::move(meta);
        }
    }
    return output;
}

SymbolMetadataMap load_symbol_metadata_config(const Config& config,
                                              const std::string& key) {
    SymbolMetadataMap output;
    auto obj = config.get_as<ConfigValue::Object>(key);
    if (!obj) {
        return output;
    }
    for (const auto& [ticker, raw] : *obj) {
        auto entry = raw.get_if<ConfigValue::Object>();
        if (!entry) {
            continue;
        }
        SymbolMetadata meta;
        meta.ticker = ticker;
        auto find_string = [&](const char* field) -> std::optional<std::string> {
            auto it = entry->find(field);
            if (it == entry->end()) {
                return std::nullopt;
            }
            if (const auto* str = it->second.get_if<std::string>()) {
                return *str;
            }
            return std::nullopt;
        };
        auto find_double = [&](const char* field) -> std::optional<double> {
            auto it = entry->find(field);
            if (it == entry->end()) {
                return std::nullopt;
            }
            if (const auto* dbl = it->second.get_if<double>()) {
                return *dbl;
            }
            if (const auto* integer = it->second.get_if<int64_t>()) {
                return static_cast<double>(*integer);
            }
            if (const auto* str = it->second.get_if<std::string>()) {
                return parse_double(*str);
            }
            return std::nullopt;
        };
        if (auto value = find_string("exchange")) meta.exchange = *value;
        if (auto value = find_string("asset_class")) meta.asset_class = parse_asset_class(*value);
        if (auto value = find_string("currency")) meta.currency = *value;
        if (auto value = find_double("tick_size")) meta.tick_size = *value;
        if (auto value = find_double("lot_size")) meta.lot_size = *value;
        if (auto value = find_double("multiplier")) meta.multiplier = *value;
        if (auto value = find_string("sector")) meta.sector = *value;
        if (auto value = find_string("industry")) meta.industry = *value;
        output[meta.ticker] = std::move(meta);
    }
    return output;
}

SymbolMetadataMap metadata_from_symbols(const std::vector<SymbolInfo>& symbols) {
    SymbolMetadataMap output;
    for (const auto& symbol : symbols) {
        SymbolMetadata meta;
        meta.ticker = symbol.ticker;
        if (!symbol.exchange.empty()) meta.exchange = symbol.exchange;
        meta.asset_class = symbol.asset_class;
        if (!symbol.currency.empty()) meta.currency = symbol.currency;
        if (symbol.tick_size != 0.0) meta.tick_size = symbol.tick_size;
        if (symbol.lot_size != 0.0) meta.lot_size = symbol.lot_size;
        if (symbol.multiplier != 0.0) meta.multiplier = symbol.multiplier;
        if (!symbol.sector.empty()) meta.sector = symbol.sector;
        if (!symbol.industry.empty()) meta.industry = symbol.industry;
        output[meta.ticker] = std::move(meta);
    }
    return output;
}

void apply_symbol_metadata(std::vector<SymbolInfo>& symbols,
                           const SymbolMetadataMap& metadata,
                           bool overwrite) {
    if (metadata.empty()) {
        return;
    }
    for (auto& symbol : symbols) {
        auto it = metadata.find(symbol.ticker);
        if (it == metadata.end()) {
            continue;
        }
        merge_metadata(symbol, it->second, overwrite);
    }
}

}  // namespace regimeflow::data
