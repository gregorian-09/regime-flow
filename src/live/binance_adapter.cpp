#include "regimeflow/live/binance_adapter.h"

#include "regimeflow/common/json.h"
#include "regimeflow/common/types.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <mutex>
#include <unordered_map>

#ifdef REGIMEFLOW_USE_CURL
#include <curl/curl.h>
#endif

#ifdef REGIMEFLOW_USE_OPENSSL
#include <openssl/hmac.h>
#endif

namespace regimeflow::live {

namespace {

#ifdef REGIMEFLOW_USE_CURL
size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}
#endif

std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

const common::JsonValue* find_field(const common::JsonValue::Object& obj,
                                    const std::string& key) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string get_string(const common::JsonValue::Object& obj, const std::string& key) {
    auto* value = find_field(obj, key);
    if (!value) {
        return {};
    }
    if (auto str = value->as_string()) {
        return *str;
    }
    return {};
}

bool get_number(const common::JsonValue::Object& obj, const std::string& key, double& out) {
    auto* value = find_field(obj, key);
    if (!value) {
        return false;
    }
    if (auto num = value->as_number()) {
        out = *num;
        return true;
    }
    if (auto str = value->as_string()) {
        try {
            out = std::stod(*str);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

bool get_int64(const common::JsonValue::Object& obj, const std::string& key, int64_t& out) {
    double val = 0.0;
    if (!get_number(obj, key, val)) {
        return false;
    }
    out = static_cast<int64_t>(val);
    return true;
}

LiveOrderStatus map_order_status(std::string value) {
    value = to_upper(std::move(value));
    if (value == "NEW") return LiveOrderStatus::New;
    if (value == "PARTIALLY_FILLED") return LiveOrderStatus::PartiallyFilled;
    if (value == "FILLED") return LiveOrderStatus::Filled;
    if (value == "CANCELED" || value == "CANCELLED") return LiveOrderStatus::Cancelled;
    if (value == "REJECTED" || value == "EXPIRED") return LiveOrderStatus::Rejected;
    return LiveOrderStatus::New;
}

#ifdef REGIMEFLOW_USE_OPENSSL
std::string hmac_sha256_hex(const std::string& key, const std::string& data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(),
         reinterpret_cast<const unsigned char*>(key.data()), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         digest, &len);
    std::ostringstream out;
    out.setf(std::ios::hex, std::ios::basefield);
    out.fill('0');
    for (unsigned int i = 0; i < len; ++i) {
        out.width(2);
        out << static_cast<int>(digest[i]);
    }
    return out.str();
}
#endif

}  // namespace

BinanceAdapter::BinanceAdapter(Config config) : config_(std::move(config)) {}

Result<void> BinanceAdapter::connect() {
    connected_ = true;
    if (config_.enable_streaming && !config_.stream_url.empty()) {
        data::WebSocketFeed::Config stream_cfg;
        stream_cfg.url = config_.stream_url;
        stream_cfg.subscribe_template = config_.stream_subscribe_template;
        stream_cfg.unsubscribe_template = config_.stream_unsubscribe_template;
        stream_cfg.ca_bundle_path = config_.stream_ca_bundle_path;
        stream_cfg.expected_hostname = config_.stream_expected_hostname;
        stream_ = std::make_unique<data::WebSocketFeed>(stream_cfg);
        stream_->on_raw([this](const std::string& msg) { handle_stream_message(msg); });
        auto res = stream_->connect();
        if (res.is_err()) {
            return res;
        }
        if (!symbols_.empty()) {
            stream_->subscribe(symbols_);
        }
    }
    return Ok();
}

Result<void> BinanceAdapter::disconnect() {
    connected_ = false;
    if (stream_) {
        stream_->disconnect();
    }
    return Ok();
}

bool BinanceAdapter::is_connected() const {
    return connected_.load();
}

std::string BinanceAdapter::build_trade_stream_symbol(const std::string& symbol) const {
    return to_lower(symbol) + "@trade";
}

void BinanceAdapter::subscribe_market_data(const std::vector<std::string>& symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    symbols_.clear();
    raw_symbols_.clear();
    symbols_.reserve(symbols.size());
    raw_symbols_.reserve(symbols.size());
    for (const auto& sym : symbols) {
        raw_symbols_.push_back(sym);
        symbols_.push_back(build_trade_stream_symbol(sym));
    }
    if (stream_) {
        stream_->subscribe(symbols_);
    }
}

void BinanceAdapter::unsubscribe_market_data(const std::vector<std::string>& symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> stream_symbols;
    stream_symbols.reserve(symbols.size());
    for (const auto& sym : symbols) {
        stream_symbols.push_back(build_trade_stream_symbol(sym));
        symbols_.erase(std::remove(symbols_.begin(), symbols_.end(),
                                   build_trade_stream_symbol(sym)),
                       symbols_.end());
        raw_symbols_.erase(std::remove(raw_symbols_.begin(), raw_symbols_.end(), sym),
                           raw_symbols_.end());
    }
    if (stream_) {
        stream_->unsubscribe(stream_symbols);
    }
}

Result<std::string> BinanceAdapter::submit_order(const engine::Order& order) {
    if (!connected_) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    if (config_.api_key.empty() || config_.secret_key.empty()) {
        return Error(Error::Code::BrokerError, "Binance API keys not configured");
    }
    std::ostringstream query;
    std::string symbol = to_upper(SymbolRegistry::instance().lookup(order.symbol));
    if (symbol.empty()) {
        return Error(Error::Code::InvalidArgument, "Invalid symbol");
    }
    query << "symbol=" << symbol;
    query << "&side=" << (order.side == engine::OrderSide::Buy ? "BUY" : "SELL");
    query << "&type=" << (order.type == engine::OrderType::Limit ? "LIMIT" : "MARKET");
    query << "&quantity=" << order.quantity;
    if (order.type == engine::OrderType::Limit && order.limit_price > 0.0) {
        query << "&price=" << order.limit_price;
        query << "&timeInForce=GTC";
    }
    query << "&timestamp=" << Timestamp::now().milliseconds();
    query << "&recvWindow=" << config_.recv_window_ms;
    auto res = signed_post("/api/v3/order", query.str());
    if (res.is_err()) {
        return res;
    }
    auto parsed = common::parse_json(res.value());
    if (parsed.is_ok()) {
        if (auto* obj = parsed.value().as_object()) {
            auto id = get_string(*obj, "orderId");
            if (!id.empty()) {
                return id;
            }
        }
    }
    return std::string("order-") + std::to_string(Timestamp::now().microseconds());
}

Result<void> BinanceAdapter::cancel_order(const std::string& broker_order_id) {
    if (!connected_) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    if (config_.api_key.empty() || config_.secret_key.empty()) {
        return Error(Error::Code::BrokerError, "Binance API keys not configured");
    }
    if (broker_order_id.empty()) {
        return Error(Error::Code::InvalidArgument, "Missing broker order id");
    }
    std::string symbol;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!raw_symbols_.empty()) {
            symbol = raw_symbols_.front();
        }
    }
    if (symbol.empty()) {
        return Error(Error::Code::InvalidState, "Symbol required to cancel Binance order");
    }
    std::ostringstream query;
    query << "symbol=" << to_upper(symbol);
    query << "&orderId=" << broker_order_id;
    query << "&timestamp=" << Timestamp::now().milliseconds();
    query << "&recvWindow=" << config_.recv_window_ms;
    auto res = signed_delete("/api/v3/order", query.str());
    if (res.is_err()) {
        const auto& err = res.error();
        Error copy(err.code, err.message, err.location);
        copy.details = err.details;
        return copy;
    }
    return Ok();
}

Result<void> BinanceAdapter::modify_order(const std::string& broker_order_id,
                                          const engine::OrderModification& mod) {
    if (!connected_) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    if (config_.api_key.empty() || config_.secret_key.empty()) {
        return Error(Error::Code::BrokerError, "Binance API keys not configured");
    }
    if (broker_order_id.empty()) {
        return Error(Error::Code::InvalidArgument, "Missing broker order id");
    }
    if (!mod.quantity && !mod.limit_price && !mod.stop_price) {
        return Ok();
    }
    return Error(Error::Code::InvalidState, "Binance order modification not supported");
}

AccountInfo BinanceAdapter::get_account_info() {
    AccountInfo info;
    if (!connected_ || config_.api_key.empty() || config_.secret_key.empty()) {
        return info;
    }
    std::ostringstream query;
    query << "timestamp=" << Timestamp::now().milliseconds();
    query << "&recvWindow=" << config_.recv_window_ms;
    auto res = signed_get("/api/v3/account", query.str());
    if (res.is_err()) {
        return info;
    }
    auto parsed = common::parse_json(res.value());
    if (parsed.is_err()) {
        return info;
    }
    auto* obj = parsed.value().as_object();
    if (!obj) {
        return info;
    }
    auto* balances = find_field(*obj, "balances");
    if (balances && balances->as_array()) {
        double cash = 0.0;
        for (const auto& entry : *balances->as_array()) {
            auto* bal = entry.as_object();
            if (!bal) {
                continue;
            }
            std::string asset = get_string(*bal, "asset");
            double free_amt = 0.0;
            double locked_amt = 0.0;
            get_number(*bal, "free", free_amt);
            get_number(*bal, "locked", locked_amt);
            if (asset == "USDT" || asset == "USD") {
                cash += free_amt + locked_amt;
            }
        }
        info.cash = cash;
        info.equity = cash;
        info.buying_power = cash;
    }
    return info;
}

std::vector<Position> BinanceAdapter::get_positions() {
    std::vector<Position> out;
    if (!connected_ || config_.api_key.empty() || config_.secret_key.empty()) {
        return out;
    }
    std::ostringstream query;
    query << "timestamp=" << Timestamp::now().milliseconds();
    query << "&recvWindow=" << config_.recv_window_ms;
    auto res = signed_get("/api/v3/account", query.str());
    if (res.is_err()) {
        return out;
    }
    auto parsed = common::parse_json(res.value());
    if (parsed.is_err()) {
        return out;
    }
    auto* obj = parsed.value().as_object();
    if (!obj) {
        return out;
    }
    auto* balances = find_field(*obj, "balances");
    if (!balances || !balances->as_array()) {
        return out;
    }
    for (const auto& entry : *balances->as_array()) {
        auto* bal = entry.as_object();
        if (!bal) {
            continue;
        }
        double free_amt = 0.0;
        double locked_amt = 0.0;
        get_number(*bal, "free", free_amt);
        get_number(*bal, "locked", locked_amt);
        double total = free_amt + locked_amt;
        if (total <= 0.0) {
            continue;
        }
        Position pos;
        pos.symbol = get_string(*bal, "asset");
        pos.quantity = total;
        out.push_back(pos);
        if (position_cb_) {
            position_cb_(pos);
        }
    }
    return out;
}

std::vector<ExecutionReport> BinanceAdapter::get_open_orders() {
    std::vector<ExecutionReport> out;
    if (!connected_ || config_.api_key.empty() || config_.secret_key.empty()) {
        return out;
    }
    std::ostringstream query;
    query << "timestamp=" << Timestamp::now().milliseconds();
    query << "&recvWindow=" << config_.recv_window_ms;
    auto res = signed_get("/api/v3/openOrders", query.str());
    if (res.is_err()) {
        return out;
    }
    auto parsed = common::parse_json(res.value());
    if (parsed.is_err()) {
        return out;
    }
    auto* arr = parsed.value().as_array();
    if (!arr) {
        return out;
    }
    for (const auto& entry : *arr) {
        auto* obj = entry.as_object();
        if (!obj) {
            continue;
        }
        ExecutionReport report;
        report.broker_order_id = get_string(*obj, "orderId");
        report.symbol = get_string(*obj, "symbol");
        std::string side = get_string(*obj, "side");
        report.side = side == "SELL" ? engine::OrderSide::Sell : engine::OrderSide::Buy;
        double qty = 0.0;
        double price = 0.0;
        get_number(*obj, "origQty", qty);
        get_number(*obj, "price", price);
        report.quantity = qty;
        report.price = price;
        report.status = map_order_status(get_string(*obj, "status"));
        report.timestamp = Timestamp::now();
        if (!report.broker_order_id.empty()) {
            out.push_back(report);
        }
    }
    return out;
}

void BinanceAdapter::on_market_data(std::function<void(const MarketDataUpdate&)> cb) {
    market_cb_ = std::move(cb);
}

void BinanceAdapter::on_execution_report(std::function<void(const ExecutionReport&)> cb) {
    exec_cb_ = std::move(cb);
}

void BinanceAdapter::on_position_update(std::function<void(const Position&)> cb) {
    position_cb_ = std::move(cb);
}

int BinanceAdapter::max_orders_per_second() const {
    return 10;
}

int BinanceAdapter::max_messages_per_second() const {
    return 50;
}

void BinanceAdapter::poll() {
    if (!connected_) {
        return;
    }
    if (stream_) {
        stream_->poll();
    }
}

Result<std::string> BinanceAdapter::rest_get(const std::string& path) const {
#ifdef REGIMEFLOW_USE_CURL
    static std::once_flag init_flag;
    std::call_once(init_flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Error(Error::Code::NetworkError, "curl init failed");
    }
    std::string response;
    std::string url = config_.base_url + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    if (!config_.api_key.empty()) {
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return Error(Error::Code::NetworkError, curl_easy_strerror(res));
    }
    if (status < 200 || status >= 300) {
        return Error(Error::Code::NetworkError, "HTTP error: " + std::to_string(status));
    }
    return response;
#else
    (void)path;
    return Error(Error::Code::NetworkError, "libcurl not enabled");
#endif
}

Result<std::string> BinanceAdapter::rest_post(const std::string& path,
                                              const std::string& body) const {
#ifdef REGIMEFLOW_USE_CURL
    static std::once_flag init_flag;
    std::call_once(init_flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Error(Error::Code::NetworkError, "curl init failed");
    }
    std::string response;
    std::string url = config_.base_url + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return Error(Error::Code::NetworkError, curl_easy_strerror(res));
    }
    if (status < 200 || status >= 300) {
        return Error(Error::Code::NetworkError, "HTTP error: " + std::to_string(status));
    }
    return response;
#else
    (void)path;
    (void)body;
    return Error(Error::Code::NetworkError, "libcurl not enabled");
#endif
}

Result<std::string> BinanceAdapter::rest_delete(const std::string& path) const {
#ifdef REGIMEFLOW_USE_CURL
    static std::once_flag init_flag;
    std::call_once(init_flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Error(Error::Code::NetworkError, "curl init failed");
    }
    std::string response;
    std::string url = config_.base_url + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return Error(Error::Code::NetworkError, curl_easy_strerror(res));
    }
    if (status < 200 || status >= 300) {
        return Error(Error::Code::NetworkError, "HTTP error: " + std::to_string(status));
    }
    return response;
#else
    (void)path;
    return Error(Error::Code::NetworkError, "libcurl not enabled");
#endif
}

Result<std::string> BinanceAdapter::signed_get(const std::string& path,
                                               const std::string& query) const {
#ifdef REGIMEFLOW_USE_OPENSSL
    std::string signature = hmac_sha256_hex(config_.secret_key, query);
    return rest_get(path + "?" + query + "&signature=" + signature);
#else
    (void)path;
    (void)query;
    return Error(Error::Code::InvalidState, "OpenSSL not enabled for Binance signing");
#endif
}

Result<std::string> BinanceAdapter::signed_post(const std::string& path,
                                                const std::string& query) const {
#ifdef REGIMEFLOW_USE_OPENSSL
    std::string signature = hmac_sha256_hex(config_.secret_key, query);
    return rest_post(path + "?" + query + "&signature=" + signature, "");
#else
    (void)path;
    (void)query;
    return Error(Error::Code::InvalidState, "OpenSSL not enabled for Binance signing");
#endif
}

Result<std::string> BinanceAdapter::signed_delete(const std::string& path,
                                                  const std::string& query) const {
#ifdef REGIMEFLOW_USE_OPENSSL
    std::string signature = hmac_sha256_hex(config_.secret_key, query);
    return rest_delete(path + "?" + query + "&signature=" + signature);
#else
    (void)path;
    (void)query;
    return Error(Error::Code::InvalidState, "OpenSSL not enabled for Binance signing");
#endif
}

void BinanceAdapter::handle_stream_message(const std::string& msg) {
    if (!market_cb_) {
        return;
    }
    auto parsed = common::parse_json(msg);
    if (parsed.is_err()) {
        return;
    }
    const common::JsonValue::Object* obj = parsed.value().as_object();
    if (!obj) {
        return;
    }
    const common::JsonValue::Object* payload = obj;
    if (auto* data = find_field(*obj, "data")) {
        if (auto* data_obj = data->as_object()) {
            payload = data_obj;
        }
    }
    std::string event = get_string(*payload, "e");
    if (event != "trade" && event != "aggTrade") {
        return;
    }
    std::string symbol = get_string(*payload, "s");
    if (symbol.empty()) {
        return;
    }
    double price = 0.0;
    double qty = 0.0;
    if (!get_number(*payload, "p", price)) {
        return;
    }
    get_number(*payload, "q", qty);
    int64_t ts = 0;
    if (!get_int64(*payload, "T", ts)) {
        get_int64(*payload, "E", ts);
    }
    if (ts == 0) {
        ts = Timestamp::now().microseconds();
    }
    data::Tick tick;
    tick.symbol = SymbolRegistry::instance().intern(symbol);
    tick.timestamp = Timestamp(ts);
    tick.price = price;
    tick.quantity = qty;
    MarketDataUpdate update;
    update.data = tick;
    market_cb_(update);
}

}  // namespace regimeflow::live
