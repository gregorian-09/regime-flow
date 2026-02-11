#include "regimeflow/data/api_data_source.h"

#include "regimeflow/data/merged_iterator.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/validation_utils.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <mutex>
#include <sstream>
#include <stdexcept>

#ifdef REGIMEFLOW_USE_CURL
#include <curl/curl.h>
#endif

namespace regimeflow::data {

namespace {

std::vector<std::string> split_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string token;
    std::istringstream stream(line);
    while (std::getline(stream, token, delimiter)) {
        fields.push_back(token);
    }
    return fields;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string normalize_header(std::string value) {
    value = trim(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::map<std::string, std::string> default_aliases() {
    return {
        {"datetime", "timestamp"},
        {"date", "timestamp"},
        {"time", "timestamp"},
        {"open_price", "open"},
        {"high_price", "high"},
        {"low_price", "low"},
        {"close_price", "close"},
        {"adj_close", "close"},
        {"adjclose", "close"},
        {"volume_traded", "volume"},
        {"vol", "volume"},
        {"qty", "quantity"}
    };
}

std::string apply_alias(std::string name, const std::map<std::string, std::string>& aliases) {
    auto it = aliases.find(name);
    if (it != aliases.end()) {
        return it->second;
    }
    return name;
}

std::map<std::string, int> map_headers(const std::string& header_line) {
    std::map<std::string, int> mapping;
    auto aliases = default_aliases();
    auto fields = split_line(header_line, ',');
    for (size_t i = 0; i < fields.size(); ++i) {
        auto name = normalize_header(fields[i]);
        name = apply_alias(name, aliases);
        if (!name.empty()) {
            mapping[name] = static_cast<int>(i);
        }
    }
    return mapping;
}

Timestamp parse_timestamp(const std::string& value) {
    bool numeric = !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
    if (numeric) {
        auto num = std::stoll(value);
        if (value.size() > 13) {
            return Timestamp(num);
        }
        return Timestamp(num * 1000);
    }
    try {
        return Timestamp::from_string(value, "%Y-%m-%d %H:%M:%S");
    } catch (...) {
        try {
            return Timestamp::from_string(value, "%Y-%m-%d");
        } catch (...) {
            return Timestamp(0);
        }
    }
}

std::vector<Bar> parse_csv_bars(const std::string& data, SymbolId symbol) {
    std::vector<Bar> out;
    std::istringstream stream(data);
    std::string line;
    if (!std::getline(stream, line)) {
        return out;
    }
    auto mapping = map_headers(line);
    if (!mapping.count("timestamp") || !mapping.count("open") || !mapping.count("high") ||
        !mapping.count("low") || !mapping.count("close") || !mapping.count("volume")) {
        return out;
    }
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        auto fields = split_line(line, ',');
        if (fields.size() < mapping.size()) {
            continue;
        }
        Bar bar;
        bar.symbol = symbol;
        bar.timestamp = parse_timestamp(fields[mapping["timestamp"]]);
        bar.open = std::stod(fields[mapping["open"]]);
        bar.high = std::stod(fields[mapping["high"]]);
        bar.low = std::stod(fields[mapping["low"]]);
        bar.close = std::stod(fields[mapping["close"]]);
        bar.volume = static_cast<uint64_t>(std::stoull(fields[mapping["volume"]]));
        out.push_back(bar);
    }
    return out;
}

std::vector<Tick> parse_csv_ticks(const std::string& data, SymbolId symbol) {
    std::vector<Tick> out;
    std::istringstream stream(data);
    std::string line;
    if (!std::getline(stream, line)) {
        return out;
    }
    auto mapping = map_headers(line);
    if (!mapping.count("timestamp") || !mapping.count("price") || !mapping.count("quantity")) {
        return out;
    }
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        auto fields = split_line(line, ',');
        if (fields.size() < mapping.size()) {
            continue;
        }
        Tick tick;
        tick.symbol = symbol;
        tick.timestamp = parse_timestamp(fields[mapping["timestamp"]]);
        tick.price = std::stod(fields[mapping["price"]]);
        tick.quantity = std::stod(fields[mapping["quantity"]]);
        out.push_back(tick);
    }
    return out;
}

std::string url_encode(const std::string& value) {
    std::ostringstream encoded;
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase << std::hex
                    << static_cast<int>(c) << std::nouppercase << std::dec;
        }
    }
    return encoded.str();
}

#ifdef REGIMEFLOW_USE_CURL
size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}
#endif

Result<std::string> http_get(const std::string& url,
                             const std::vector<std::string>& headers,
                             int timeout_seconds) {
#ifdef REGIMEFLOW_USE_CURL
    static std::once_flag init_flag;
    std::call_once(init_flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Error(Error::Code::NetworkError, "Failed to init curl");
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* hdrs = nullptr;
    for (const auto& h : headers) {
        hdrs = curl_slist_append(hdrs, h.c_str());
    }
    if (hdrs) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    }

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    if (hdrs) {
        curl_slist_free_all(hdrs);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return Error(Error::Code::NetworkError, curl_easy_strerror(res));
    }
    if (status < 200 || status >= 300) {
        return Error(Error::Code::NetworkError, "HTTP error: " + std::to_string(status));
    }
    return Ok(response);
#else
    (void)url;
    (void)headers;
    (void)timeout_seconds;
    return Error(Error::Code::NetworkError, "libcurl not enabled");
#endif
}

}  // namespace

ApiDataSource::ApiDataSource(Config config) : config_(std::move(config)) {}

std::vector<SymbolInfo> ApiDataSource::get_available_symbols() const {
    std::vector<SymbolInfo> out;
    out.reserve(config_.symbols.size());
    for (const auto& s : config_.symbols) {
        SymbolInfo info;
        info.id = SymbolRegistry::instance().intern(s);
        info.ticker = s;
        out.push_back(info);
    }
    return out;
}

TimeRange ApiDataSource::get_available_range(SymbolId) const {
    return {};
}

std::vector<Bar> ApiDataSource::get_bars(SymbolId symbol, TimeRange range, BarType bar_type) {
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return {};
    }
    std::string url = build_url(config_.bars_endpoint, symbol_name, range, bar_type);
    std::vector<std::string> headers;
    if (!config_.api_key.empty()) {
        headers.push_back(config_.api_key_header + ": " + config_.api_key);
    }
    auto response = http_get(url, headers, config_.timeout_seconds);
    if (response.is_err()) {
        return {};
    }
    last_report_ = ValidationReport();
    auto bars = parse_csv_bars(response.value(), symbol);
    return validate_bars(std::move(bars), bar_type, config_.validation, config_.fill_missing_bars,
                         config_.collect_validation_report, &last_report_);
}

std::vector<Tick> ApiDataSource::get_ticks(SymbolId symbol, TimeRange range) {
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return {};
    }
    std::string url = build_url(config_.ticks_endpoint, symbol_name, range, BarType::Tick);
    std::vector<std::string> headers;
    if (!config_.api_key.empty()) {
        headers.push_back(config_.api_key_header + ": " + config_.api_key);
    }
    auto response = http_get(url, headers, config_.timeout_seconds);
    if (response.is_err()) {
        return {};
    }
    last_report_ = ValidationReport();
    auto ticks = parse_csv_ticks(response.value(), symbol);
    return validate_ticks(std::move(ticks), config_.validation, config_.collect_validation_report,
                          &last_report_);
}

std::unique_ptr<DataIterator> ApiDataSource::create_iterator(
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

std::vector<CorporateAction> ApiDataSource::get_corporate_actions(SymbolId, TimeRange) {
    return {};
}

std::string ApiDataSource::build_url(const std::string& endpoint,
                                     const std::string& symbol,
                                     TimeRange range,
                                     BarType bar_type) const {
    std::ostringstream out;
    out << config_.base_url << endpoint;
    out << "?symbol=" << url_encode(symbol);
    if (range.start.microseconds() > 0) {
        out << "&start=" << url_encode(format_timestamp(range.start));
    }
    if (range.end.microseconds() > 0) {
        out << "&end=" << url_encode(format_timestamp(range.end));
    }
    out << "&bar_type=" << static_cast<int>(bar_type);
    out << "&format=" << url_encode(config_.format);
    return out.str();
}

std::string ApiDataSource::format_timestamp(Timestamp ts) const {
    if (config_.time_format == "epoch_us") {
        return std::to_string(ts.microseconds());
    }
    if (config_.time_format == "epoch_ms") {
        return std::to_string(ts.milliseconds());
    }
    return ts.to_string("%Y-%m-%d %H:%M:%S");
}

}  // namespace regimeflow::data
