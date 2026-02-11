#include "regimeflow/data/websocket_feed.h"

#include "regimeflow/common/json.h"

#include "regimeflow/data/data_validation.h"

#include "regimeflow/common/types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <sstream>
#include <vector>

#ifdef REGIMEFLOW_USE_BOOST_BEAST
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#ifdef REGIMEFLOW_USE_OPENSSL
#include <boost/asio/ssl/stream.hpp>
#include <openssl/ssl.h>
#endif
#endif

namespace regimeflow::data {

namespace {

struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
};

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl out;
    auto scheme_pos = url.find("://");
    if (scheme_pos == std::string::npos) {
        return out;
    }
    out.scheme = url.substr(0, scheme_pos);
    auto host_start = scheme_pos + 3;
    auto path_pos = url.find('/', host_start);
    std::string host_port = path_pos == std::string::npos ? url.substr(host_start)
                                                          : url.substr(host_start, path_pos - host_start);
    auto colon_pos = host_port.find(':');
    if (colon_pos == std::string::npos) {
        out.host = host_port;
    } else {
        out.host = host_port.substr(0, colon_pos);
        out.port = host_port.substr(colon_pos + 1);
    }
    if (path_pos == std::string::npos) {
        out.target = "/";
    } else {
        out.target = url.substr(path_pos);
    }
    return out;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) {
        return {};
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        return {};
    }
    pos++;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos >= json.size()) {
        return {};
    }
    if (json[pos] == '"') {
        ++pos;
        auto end = json.find('"', pos);
        if (end == std::string::npos) {
            return {};
        }
        return json.substr(pos, end - pos);
    }
    auto end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' &&
           !std::isspace(static_cast<unsigned char>(json[end]))) {
        ++end;
    }
    return trim(json.substr(pos, end - pos));
}

std::string extract_json_string_any(const std::string& json,
                                    const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        auto value = extract_json_string(json, key);
        if (!value.empty()) {
            return value;
        }
    }
    return {};
}

bool extract_json_number(const std::string& json, const std::string& key, double& out) {
    auto value = extract_json_string(json, key);
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    out = std::strtod(value.c_str(), &end);
    return end != value.c_str();
}

bool extract_json_number_any(const std::string& json,
                             const std::vector<std::string>& keys,
                             double& out) {
    for (const auto& key : keys) {
        if (extract_json_number(json, key, out)) {
            return true;
        }
    }
    return false;
}

bool extract_json_int64(const std::string& json, const std::string& key, int64_t& out) {
    auto value = extract_json_string(json, key);
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    out = std::strtoll(value.c_str(), &end, 10);
    return end != value.c_str();
}

bool extract_json_int64_any(const std::string& json,
                            const std::vector<std::string>& keys,
                            int64_t& out) {
    for (const auto& key : keys) {
        if (extract_json_int64(json, key, out)) {
            return true;
        }
    }
    return false;
}

std::vector<std::array<double, 3>> extract_levels(const std::string& json,
                                                  const std::string& key,
                                                  size_t max_levels) {
    std::vector<std::array<double, 3>> levels;
    std::string pattern = "\"" + key + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) {
        return levels;
    }
    pos = json.find('[', pos);
    if (pos == std::string::npos) {
        return levels;
    }
    ++pos;

    while (pos < json.size() && levels.size() < max_levels) {
        while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
            ++pos;
        }
        if (pos >= json.size() || json[pos] == ']') {
            break;
        }
        if (json[pos] != '[') {
            ++pos;
            continue;
        }
        ++pos;
        std::array<double, 3> level{0.0, 0.0, 0.0};
        for (int i = 0; i < 3; ++i) {
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
                ++pos;
            }
            char* end = nullptr;
            level[i] = std::strtod(json.c_str() + pos, &end);
            if (end == json.c_str() + pos) {
                break;
            }
            pos = static_cast<size_t>(end - json.c_str());
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
                ++pos;
            }
            if (i < 2 && pos < json.size() && json[pos] == ',') {
                ++pos;
            }
        }
        levels.push_back(level);
        while (pos < json.size() && json[pos] != ']' && json[pos] != ',') {
            ++pos;
        }
        if (pos < json.size() && json[pos] == ']') {
            ++pos;
        }
        if (pos < json.size() && json[pos] == ',') {
            ++pos;
        }
    }
    return levels;
}

std::string apply_template(const std::string& tpl, const std::vector<std::string>& symbols) {
    if (tpl.empty()) {
        return {};
    }
    std::ostringstream joined;
    joined << "[";
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0) {
            joined << ",";
        }
        joined << "\"" << symbols[i] << "\"";
    }
    joined << "]";
    std::string result = tpl;
    auto pos = result.find("{symbols}");
    if (pos != std::string::npos) {
        result.replace(pos, std::string("{symbols}").size(), joined.str());
    }
    return result;
}

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool json_has_string(const common::JsonValue::Object& obj,
                     std::initializer_list<const char*> keys) {
    for (const auto* key : keys) {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.as_string()) {
            return true;
        }
    }
    return false;
}

bool json_has_number(const common::JsonValue::Object& obj,
                     std::initializer_list<const char*> keys) {
    for (const auto* key : keys) {
        auto it = obj.find(key);
        if (it != obj.end()) {
            if (it->second.as_number()) {
                return true;
            }
            if (auto str = it->second.as_string()) {
                if (!str->empty()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool json_has_array(const common::JsonValue::Object& obj,
                    std::initializer_list<const char*> keys) {
    for (const auto* key : keys) {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.as_array()) {
            return true;
        }
    }
    return false;
}

bool within_trading_hours(const Timestamp& ts, int start_seconds, int end_seconds) {
    std::time_t seconds = static_cast<std::time_t>(ts.microseconds() / 1'000'000);
    std::tm tm = {};
#if defined(_WIN32)
    gmtime_s(&tm, &seconds);
#else
    gmtime_r(&seconds, &tm);
#endif
    int secs = tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
    if (start_seconds <= end_seconds) {
        return secs >= start_seconds && secs <= end_seconds;
    }
    return secs >= start_seconds || secs <= end_seconds;
}

ValidationAction normalize_error_action(ValidationAction action) {
    if (action == ValidationAction::Continue || action == ValidationAction::Fill) {
        return ValidationAction::Skip;
    }
    return action;
}

}  // namespace

WebSocketFeed::WebSocketFeed(Config config) : config_(std::move(config)) {}

Result<void> WebSocketFeed::validate_tls_config() const {
#ifdef REGIMEFLOW_USE_BOOST_BEAST
    ParsedUrl url = parse_url(config_.url);
    if (url.scheme != "wss") {
        return Ok();
    }
#ifdef REGIMEFLOW_USE_OPENSSL
    if (config_.verify_tls && !config_.ca_bundle_path.empty()) {
        boost::asio::ssl::context ctx{boost::asio::ssl::context::tls_client};
        try {
            ctx.load_verify_file(config_.ca_bundle_path);
        } catch (const std::exception& ex) {
            return Error(Error::Code::InvalidArgument, ex.what());
        }
    }
    return Ok();
#else
    return Error(Error::Code::InvalidState, "OpenSSL not enabled for wss://");
#endif
#else
    return Error(Error::Code::InvalidState, "Boost.Beast not enabled");
#endif
}

Result<void> WebSocketFeed::connect() {
    if (config_.url.empty()) {
        return Error(Error::Code::InvalidArgument, "WebSocket URL not configured");
    }

#ifdef REGIMEFLOW_USE_BOOST_BEAST
    if (config_.connect_override) {
        auto res = config_.connect_override();
        connected_ = res.is_ok();
        if (connected_) {
            backoff_ms_ = 0;
        }
        return res;
    }
    ParsedUrl url = parse_url(config_.url);
    if (url.scheme != "ws" && url.scheme != "wss") {
        return Error(Error::Code::InvalidArgument, "Only ws:// or wss:// URLs are supported");
    }
    use_tls_ = (url.scheme == "wss");
    if (url.port.empty()) {
        url.port = use_tls_ ? "443" : "80";
    }
    if (url.host.empty()) {
        return Error(Error::Code::InvalidArgument, "WebSocket URL missing host");
    }

    try {
        auto results = resolver_.resolve(url.host, url.port);
        if (use_tls_) {
#ifdef REGIMEFLOW_USE_OPENSSL
            if (config_.verify_tls) {
                if (!config_.ca_bundle_path.empty()) {
                    ssl_ctx_.load_verify_file(config_.ca_bundle_path);
                } else {
                    ssl_ctx_.set_default_verify_paths();
                }
                ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
            } else {
                ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_none);
            }
            ws_tls_.emplace(ioc_, ssl_ctx_);
            boost::beast::get_lowest_layer(*ws_tls_).connect(results);
            std::string host_for_sni = config_.expected_hostname.empty()
                ? url.host
                : config_.expected_hostname;
            if (!SSL_set_tlsext_host_name(ws_tls_->next_layer().native_handle(),
                                          host_for_sni.c_str())) {
                throw std::runtime_error("Failed to set SNI hostname");
            }
            ws_tls_->next_layer().handshake(boost::asio::ssl::stream_base::client);
            ws_tls_->set_option(boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::client));
            ws_tls_->handshake(host_for_sni, url.target);
#else
            return Error(Error::Code::InvalidState, "OpenSSL not enabled for wss://");
#endif
        } else {
            ws_.emplace(ioc_);
            boost::beast::get_lowest_layer(*ws_).connect(results);
            ws_->set_option(boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::client));
            ws_->handshake(url.host, url.target);
        }
        connected_ = true;
        backoff_ms_ = 0;
    } catch (const std::exception& ex) {
        connected_ = false;
        return Error(Error::Code::NetworkError, ex.what());
    }
#else
    return Error(Error::Code::InvalidState, "Boost.Beast not enabled");
#endif

    if (!subscriptions_.empty()) {
        subscribe(subscriptions_);
    }
    return Ok();
}

void WebSocketFeed::disconnect() {
#ifdef REGIMEFLOW_USE_BOOST_BEAST
    if (ws_) {
        boost::beast::error_code ec;
        ws_->close(boost::beast::websocket::close_code::normal, ec);
    }
#ifdef REGIMEFLOW_USE_OPENSSL
    if (ws_tls_) {
        boost::beast::error_code ec;
        ws_tls_->close(boost::beast::websocket::close_code::normal, ec);
    }
    ws_tls_.reset();
#endif
    ws_.reset();
#endif
    connected_ = false;
}

bool WebSocketFeed::is_connected() const {
    return connected_;
}

void WebSocketFeed::subscribe(const std::vector<std::string>& symbols) {
    for (const auto& sym : symbols) {
        if (std::find(subscriptions_.begin(), subscriptions_.end(), sym) == subscriptions_.end()) {
            subscriptions_.push_back(sym);
        }
    }
#ifdef REGIMEFLOW_USE_BOOST_BEAST
    if (connected_) {
        auto msg = apply_template(config_.subscribe_template, symbols);
        if (!msg.empty()) {
            boost::beast::error_code ec;
            if (ws_) {
                ws_->write(boost::asio::buffer(msg), ec);
#ifdef REGIMEFLOW_USE_OPENSSL
            } else if (ws_tls_) {
                ws_tls_->write(boost::asio::buffer(msg), ec);
#endif
            }
        }
    }
#endif
}

void WebSocketFeed::unsubscribe(const std::vector<std::string>& symbols) {
    for (const auto& sym : symbols) {
        subscriptions_.erase(std::remove(subscriptions_.begin(), subscriptions_.end(), sym),
                             subscriptions_.end());
    }
#ifdef REGIMEFLOW_USE_BOOST_BEAST
    if (connected_) {
        auto msg = apply_template(config_.unsubscribe_template, symbols);
        if (!msg.empty()) {
            boost::beast::error_code ec;
            if (ws_) {
                ws_->write(boost::asio::buffer(msg), ec);
#ifdef REGIMEFLOW_USE_OPENSSL
            } else if (ws_tls_) {
                ws_tls_->write(boost::asio::buffer(msg), ec);
#endif
            }
        }
    }
#endif
}

void WebSocketFeed::on_bar(std::function<void(const Bar&)> cb) {
    bar_cb_ = std::move(cb);
}

void WebSocketFeed::on_tick(std::function<void(const Tick&)> cb) {
    tick_cb_ = std::move(cb);
}

void WebSocketFeed::on_book(std::function<void(const OrderBook&)> cb) {
    book_cb_ = std::move(cb);
}

void WebSocketFeed::on_raw(std::function<void(const std::string&)> cb) {
    raw_cb_ = std::move(cb);
}

void WebSocketFeed::on_reconnect(std::function<void(const ReconnectState&)> cb) {
    reconnect_cb_ = std::move(cb);
}

void WebSocketFeed::handle_message(const std::string& msg) {
    if (msg.empty()) {
        return;
    }
    if (raw_cb_) {
        raw_cb_(msg);
    }

    auto handle_issue = [&](ValidationSeverity severity,
                            ValidationAction action,
                            const std::string& message) -> bool {
        if (!config_.validate_messages) {
            return true;
        }
        if (severity == ValidationSeverity::Error) {
            action = normalize_error_action(action);
        }
        if (action == ValidationAction::Fail) {
            last_reconnect_error_ = "Validation error: " + message;
            disconnect();
            return false;
        }
        if (action == ValidationAction::Skip) {
            return false;
        }
        return true;
    };

    if (config_.validate_messages && config_.strict_schema) {
        auto parsed = common::parse_json(msg);
        if (parsed.is_err()) {
            if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                              "JSON parse error")) {
                return;
            }
        } else {
            const auto* root = parsed.value().as_object();
            const auto* payload = root;
            if (root) {
                auto data_it = root->find("data");
                if (data_it != root->end()) {
                    if (auto* obj = data_it->second.as_object()) {
                        payload = obj;
                    }
                }
            }
            if (!payload) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "JSON payload is not an object")) {
                    return;
                }
            } else {
                std::string type = extract_json_string_any(msg, {"type", "T", "event"});
                type = to_lower(type);
                if (type.empty()) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Missing type field")) {
                        return;
                    }
                }
                if (!json_has_string(*payload, {"symbol", "S", "sym"})) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Missing symbol field")) {
                        return;
                    }
                }
                if (type == "tick" || type == "trade" || type == "t") {
                    if (!json_has_number(*payload, {"price", "p"}) ||
                        !json_has_number(*payload, {"timestamp", "t", "ts"})) {
                        if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                          "Tick schema missing price/timestamp")) {
                            return;
                        }
                    }
                } else if (type == "bar" || type == "b" || type == "ohlc") {
                    if (!json_has_number(*payload, {"open", "o"}) ||
                        !json_has_number(*payload, {"high", "h"}) ||
                        !json_has_number(*payload, {"low", "l"}) ||
                        !json_has_number(*payload, {"close", "c"}) ||
                        !json_has_number(*payload, {"volume", "v"}) ||
                        !json_has_number(*payload, {"timestamp", "t", "ts"})) {
                        if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                          "Bar schema missing required fields")) {
                            return;
                        }
                    }
                } else if (type == "book" || type == "depth" || type == "orderbook") {
                    if (!json_has_array(*payload, {"bids", "b"}) ||
                        !json_has_array(*payload, {"asks", "a"})) {
                        if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                          "Book schema missing bids/asks")) {
                            return;
                        }
                    }
                }
            }
        }
    }

    auto type = extract_json_string_any(msg, {"type", "T", "event"});
    type = to_lower(type);
    auto symbol_str = extract_json_string_any(msg, {"symbol", "S", "sym"});
    if (symbol_str.empty()) {
        return;
    }
    auto symbol_id = SymbolRegistry::instance().intern(symbol_str);
    int64_t ts = 0;
    if (!extract_json_int64_any(msg, {"timestamp", "t", "ts"}, ts)) {
        ts = Timestamp::now().microseconds();
    }

    if ((type == "tick" || type == "trade" || type == "t") && tick_cb_) {
        Tick tick;
        tick.symbol = symbol_id;
        tick.timestamp = Timestamp(ts);
        double price = 0.0;
        double qty = 0.0;
        if (!extract_json_number_any(msg, {"price", "p"}, price)) {
            return;
        }
        extract_json_number_any(msg, {"quantity", "size", "s"}, qty);
        tick.price = price;
        tick.quantity = qty;
        if (config_.validate_messages) {
            auto& state = tick_state_[symbol_id];
            if (config_.validation.check_price_bounds) {
                if (!std::isfinite(tick.price) || tick.price <= 0.0 ||
                    (config_.validation.max_price > 0.0 &&
                     tick.price > config_.validation.max_price)) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Invalid tick price")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_volume_bounds && config_.validation.max_volume > 0 &&
                tick.quantity > static_cast<double>(config_.validation.max_volume)) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "Tick quantity exceeds max_volume")) {
                    return;
                }
            }
            if (config_.validation.check_future_timestamps) {
                auto now = Timestamp::now();
                if (tick.timestamp > now + config_.validation.max_future_skew) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Tick timestamp is in the future")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_trading_hours &&
                !within_trading_hours(tick.timestamp, config_.validation.trading_start_seconds,
                                      config_.validation.trading_end_seconds)) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "Tick timestamp outside trading hours")) {
                    return;
                }
            }
            if (config_.validation.require_monotonic_timestamps && state.has_last_ts &&
                tick.timestamp < state.last_ts) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "Tick timestamp not monotonic")) {
                    return;
                }
            }
            if (config_.validation.check_gap && state.has_last_ts) {
                Duration gap = tick.timestamp - state.last_ts;
                if (gap.total_microseconds() >
                    config_.validation.max_gap.total_microseconds()) {
                    if (!handle_issue(ValidationSeverity::Warning, config_.validation.on_gap,
                                      "Tick timestamp gap exceeds max_gap")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_price_jump && state.has_last_price &&
                state.last_price != 0.0) {
                double jump = std::abs(tick.price - state.last_price) /
                              std::abs(state.last_price);
                if (jump > config_.validation.max_jump_pct) {
                    if (!handle_issue(ValidationSeverity::Warning, config_.validation.on_warning,
                                      "Tick price jump exceeds max_jump_pct")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_outliers &&
                state.price_stats.count >= config_.validation.outlier_warmup) {
                double stddev = state.price_stats.stddev();
                if (stddev > 0.0) {
                    double zscore = std::abs(tick.price - state.price_stats.mean) / stddev;
                    if (zscore > config_.validation.outlier_zscore) {
                        if (!handle_issue(ValidationSeverity::Warning,
                                          config_.validation.on_warning,
                                          "Tick price outlier")) {
                            return;
                        }
                    }
                }
            }
            if (config_.validation.check_outliers &&
                state.volume_stats.count >= config_.validation.outlier_warmup) {
                double stddev = state.volume_stats.stddev();
                if (stddev > 0.0) {
                    double zscore = std::abs(tick.quantity - state.volume_stats.mean) / stddev;
                    if (zscore > config_.validation.outlier_zscore) {
                        if (!handle_issue(ValidationSeverity::Warning,
                                          config_.validation.on_warning,
                                          "Tick quantity outlier")) {
                            return;
                        }
                    }
                }
            }

            state.last_ts = tick.timestamp;
            state.has_last_ts = true;
            state.last_price = tick.price;
            state.has_last_price = true;
            if (config_.validation.check_outliers) {
                state.price_stats.push(tick.price);
                state.volume_stats.push(tick.quantity);
            }
        }
        tick_cb_(tick);
        return;
    }

    if ((type == "bar" || type == "b" || type == "candlestick") && bar_cb_) {
        Bar bar;
        bar.symbol = symbol_id;
        bar.timestamp = Timestamp(ts);
        double open = 0.0;
        double high = 0.0;
        double low = 0.0;
        double close = 0.0;
        double volume = 0.0;
        if (!extract_json_number_any(msg, {"open", "o"}, open)) {
            return;
        }
        extract_json_number_any(msg, {"high", "h"}, high);
        extract_json_number_any(msg, {"low", "l"}, low);
        extract_json_number_any(msg, {"close", "c"}, close);
        extract_json_number_any(msg, {"volume", "v"}, volume);
        bar.open = open;
        bar.high = high;
        bar.low = low;
        bar.close = close;
        bar.volume = static_cast<uint64_t>(volume);
        if (config_.validate_messages) {
            auto& state = bar_state_[symbol_id];
            if (config_.validation.check_price_bounds) {
                if (!std::isfinite(bar.open) || !std::isfinite(bar.high) ||
                    !std::isfinite(bar.low) || !std::isfinite(bar.close) ||
                    bar.open <= 0 || bar.high <= 0 || bar.low <= 0 || bar.close <= 0 ||
                    (config_.validation.max_price > 0.0 &&
                     (bar.open > config_.validation.max_price ||
                      bar.high > config_.validation.max_price ||
                      bar.low > config_.validation.max_price ||
                      bar.close > config_.validation.max_price)) ||
                    (bar.high < bar.low || bar.open < bar.low || bar.open > bar.high ||
                     bar.close < bar.low || bar.close > bar.high)) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Bar OHLC out of range")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_volume_bounds && config_.validation.max_volume > 0 &&
                bar.volume > config_.validation.max_volume) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "Bar volume exceeds max_volume")) {
                    return;
                }
            }
            if (config_.validation.check_future_timestamps) {
                auto now = Timestamp::now();
                if (bar.timestamp > now + config_.validation.max_future_skew) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Bar timestamp is in the future")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_trading_hours &&
                !within_trading_hours(bar.timestamp, config_.validation.trading_start_seconds,
                                      config_.validation.trading_end_seconds)) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "Bar timestamp outside trading hours")) {
                    return;
                }
            }
            if (config_.validation.require_monotonic_timestamps && state.has_last_ts &&
                bar.timestamp < state.last_ts) {
                if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                  "Bar timestamp not monotonic")) {
                    return;
                }
            }
            if (config_.validation.check_gap && state.has_last_ts) {
                Duration gap = bar.timestamp - state.last_ts;
                if (gap.total_microseconds() >
                    config_.validation.max_gap.total_microseconds()) {
                    if (!handle_issue(ValidationSeverity::Warning, config_.validation.on_gap,
                                      "Bar timestamp gap exceeds max_gap")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_price_jump && state.has_last_price &&
                state.last_price != 0.0) {
                double jump = std::abs(bar.close - state.last_price) /
                              std::abs(state.last_price);
                if (jump > config_.validation.max_jump_pct) {
                    if (!handle_issue(ValidationSeverity::Warning, config_.validation.on_warning,
                                      "Bar price jump exceeds max_jump_pct")) {
                        return;
                    }
                }
            }
            if (config_.validation.check_outliers &&
                state.price_stats.count >= config_.validation.outlier_warmup) {
                double stddev = state.price_stats.stddev();
                if (stddev > 0.0) {
                    double zscore = std::abs(bar.close - state.price_stats.mean) / stddev;
                    if (zscore > config_.validation.outlier_zscore) {
                        if (!handle_issue(ValidationSeverity::Warning,
                                          config_.validation.on_warning,
                                          "Bar price outlier")) {
                            return;
                        }
                    }
                }
            }
            if (config_.validation.check_outliers &&
                state.volume_stats.count >= config_.validation.outlier_warmup) {
                double stddev = state.volume_stats.stddev();
                if (stddev > 0.0) {
                    double zscore =
                        std::abs(static_cast<double>(bar.volume) - state.volume_stats.mean) /
                        stddev;
                    if (zscore > config_.validation.outlier_zscore) {
                        if (!handle_issue(ValidationSeverity::Warning,
                                          config_.validation.on_warning,
                                          "Bar volume outlier")) {
                            return;
                        }
                    }
                }
            }

            state.last_ts = bar.timestamp;
            state.has_last_ts = true;
            state.last_price = bar.close;
            state.has_last_price = true;
            if (config_.validation.check_outliers) {
                state.price_stats.push(bar.close);
                state.volume_stats.push(static_cast<double>(bar.volume));
            }
        }
        bar_cb_(bar);
        return;
    }

    if ((type == "book" || type == "depth" || type == "orderbook" || type == "l2") && book_cb_) {
        OrderBook book;
        book.symbol = symbol_id;
        book.timestamp = Timestamp(ts);
        auto bids = extract_levels(msg, "bids", book.bids.size());
        auto asks = extract_levels(msg, "asks", book.asks.size());
        for (size_t i = 0; i < bids.size(); ++i) {
            book.bids[i].price = bids[i][0];
            book.bids[i].quantity = bids[i][1];
            book.bids[i].num_orders = static_cast<int>(bids[i][2]);
        }
        for (size_t i = 0; i < asks.size(); ++i) {
            book.asks[i].price = asks[i][0];
            book.asks[i].quantity = asks[i][1];
            book.asks[i].num_orders = static_cast<int>(asks[i][2]);
        }
        if (config_.validate_messages) {
            if (config_.validation.check_future_timestamps) {
                auto now = Timestamp::now();
                if (book.timestamp > now + config_.validation.max_future_skew) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Book timestamp is in the future")) {
                        return;
                    }
                }
            }
            if (config_.validation.require_monotonic_timestamps) {
                auto& last = book_last_ts_[symbol_id];
                if (last.microseconds() != 0 && book.timestamp < last) {
                    if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                      "Book timestamp not monotonic")) {
                        return;
                    }
                }
                last = book.timestamp;
            }
            if (config_.validation.check_price_bounds) {
                for (const auto& level : book.bids) {
                    if (level.price < 0 || level.quantity < 0) {
                        if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                          "Book level has negative price/quantity")) {
                            return;
                        }
                    }
                }
                for (const auto& level : book.asks) {
                    if (level.price < 0 || level.quantity < 0) {
                        if (!handle_issue(ValidationSeverity::Error, config_.validation.on_error,
                                          "Book level has negative price/quantity")) {
                            return;
                        }
                    }
                }
            }
        }
        book_cb_(book);
        return;
    }
}

Result<void> WebSocketFeed::send_raw(const std::string& message) {
#ifdef REGIMEFLOW_USE_BOOST_BEAST
    if (!connected_) {
        return Error(Error::Code::InvalidState, "WebSocket not connected");
    }
    if (message.empty()) {
        return Error(Error::Code::InvalidArgument, "Message is empty");
    }
    boost::beast::error_code ec;
    if (ws_) {
        ws_->write(boost::asio::buffer(message), ec);
#ifdef REGIMEFLOW_USE_OPENSSL
    } else if (ws_tls_) {
        ws_tls_->write(boost::asio::buffer(message), ec);
#endif
    } else {
        return Error(Error::Code::InvalidState, "WebSocket stream not initialized");
    }
    if (ec) {
        return Error(Error::Code::NetworkError, ec.message());
    }
    return Ok();
#else
    (void)message;
    return Error(Error::Code::InvalidState, "Boost.Beast not enabled");
#endif
}

void WebSocketFeed::poll() {
#ifdef REGIMEFLOW_USE_BOOST_BEAST
    if (!connected_) {
        if (config_.auto_reconnect) {
            auto now = std::chrono::steady_clock::now();
            if (now >= next_reconnect_) {
                auto result = connect();
                if (result.is_err()) {
                    reconnect_attempts_ += 1;
                    last_reconnect_attempt_ = Timestamp::now();
                    last_reconnect_error_ = result.error().message;
                    if (backoff_ms_ <= 0) {
                        backoff_ms_ = config_.reconnect_initial_ms;
                    } else {
                        backoff_ms_ = std::min(backoff_ms_ * 2, config_.reconnect_max_ms);
                    }
                    if (backoff_ms_ <= 0) {
                        backoff_ms_ = 500;
                    }
                    next_reconnect_ = now + std::chrono::milliseconds(backoff_ms_);
                    next_reconnect_attempt_ = Timestamp::now() + Duration::milliseconds(backoff_ms_);
                    if (reconnect_cb_) {
                        ReconnectState state;
                        state.connected = false;
                        state.attempts = reconnect_attempts_;
                        state.backoff_ms = backoff_ms_;
                        state.last_attempt = last_reconnect_attempt_;
                        state.next_attempt = next_reconnect_attempt_;
                        state.last_error = last_reconnect_error_;
                        reconnect_cb_(state);
                    }
                } else if (reconnect_attempts_ > 0 && reconnect_cb_) {
                    ReconnectState state;
                    state.connected = true;
                    state.attempts = reconnect_attempts_;
                    state.backoff_ms = 0;
                    state.last_attempt = Timestamp::now();
                    state.next_attempt = Timestamp{};
                    reconnect_cb_(state);
                }
                if (result.is_ok()) {
                    reconnect_attempts_ = 0;
                    last_reconnect_error_.clear();
                }
            }
        }
        return;
    }

    boost::beast::error_code ec;
    auto timeout = std::chrono::milliseconds(
        config_.read_timeout_ms > 0 ? config_.read_timeout_ms : 50);
    if (ws_) {
        boost::beast::get_lowest_layer(*ws_).expires_after(timeout);
        ws_->read(buffer_, ec);
#ifdef REGIMEFLOW_USE_OPENSSL
    } else if (ws_tls_) {
        boost::beast::get_lowest_layer(*ws_tls_).expires_after(timeout);
        ws_tls_->read(buffer_, ec);
#endif
    } else {
        connected_ = false;
        return;
    }
    if (ec == boost::beast::websocket::error::closed) {
        connected_ = false;
        return;
    }
    if (ec == boost::beast::error::timeout) {
        return;
    }
    if (ec) {
        connected_ = false;
        last_reconnect_error_ = ec.message();
        return;
    }

    auto msg = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    handle_message(msg);
#else
    (void)bar_cb_;
    (void)tick_cb_;
    (void)book_cb_;
#endif
}

}  // namespace regimeflow::data
