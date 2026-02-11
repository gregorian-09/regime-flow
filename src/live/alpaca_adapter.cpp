#include "regimeflow/live/alpaca_adapter.h"

#include "regimeflow/common/json.h"
#include "regimeflow/common/types.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <mutex>

#ifdef REGIMEFLOW_USE_CURL
#include <curl/curl.h>
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

std::string replace_token(std::string value, const std::string& token, const std::string& replacement) {
    auto pos = value.find(token);
    if (pos != std::string::npos) {
        value.replace(pos, token.size(), replacement);
    }
    return value;
}

LiveOrderStatus parse_order_status(std::string value) {
    for (auto& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (value == "new" || value == "accepted") {
        return LiveOrderStatus::New;
    }
    if (value == "partially_filled") {
        return LiveOrderStatus::PartiallyFilled;
    }
    if (value == "filled") {
        return LiveOrderStatus::Filled;
    }
    if (value == "canceled" || value == "cancelled") {
        return LiveOrderStatus::Cancelled;
    }
    if (value == "rejected") {
        return LiveOrderStatus::Rejected;
    }
    return LiveOrderStatus::New;
}

LiveOrderStatus parse_trade_update_status_local(const std::string& value) {
    std::string v = value;
    for (auto& c : v) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (v == "fill" || v == "filled") {
        return LiveOrderStatus::Filled;
    }
    if (v == "partial_fill" || v == "partially_filled") {
        return LiveOrderStatus::PartiallyFilled;
    }
    if (v == "canceled" || v == "cancelled") {
        return LiveOrderStatus::Cancelled;
    }
    if (v == "rejected") {
        return LiveOrderStatus::Rejected;
    }
    if (v == "new" || v == "accepted") {
        return LiveOrderStatus::New;
    }
    return LiveOrderStatus::New;
}

const common::JsonValue::Object* as_object(const common::JsonValue& value) {
    return value.as_object();
}

const common::JsonValue* find_field(const common::JsonValue::Object& obj,
                                    const std::string& key) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return nullptr;
    }
    return &it->second;
}

Result<std::string> require_string(const common::JsonValue::Object& obj,
                                   const std::string& key) {
    auto* value = find_field(obj, key);
    if (!value) {
        return Error(Error::Code::ParseError, "Missing JSON field: " + key);
    }
    if (auto str = value->as_string()) {
        return *str;
    }
    return Error(Error::Code::ParseError, "Invalid JSON string field: " + key);
}

Result<std::string> optional_string(const common::JsonValue::Object& obj,
                                    const std::string& key) {
    auto* value = find_field(obj, key);
    if (!value) {
        return std::string();
    }
    if (auto str = value->as_string()) {
        return *str;
    }
    return Error(Error::Code::ParseError, "Invalid JSON string field: " + key);
}

Result<double> optional_number(const common::JsonValue::Object& obj,
                               const std::string& key,
                               bool required) {
    auto* value = find_field(obj, key);
    if (!value) {
        if (required) {
            return Error(Error::Code::ParseError, "Missing JSON field: " + key);
        }
        return 0.0;
    }
    if (auto num = value->as_number()) {
        return *num;
    }
    if (auto str = value->as_string()) {
        try {
            return std::stod(*str);
        } catch (...) {
            return Error(Error::Code::ParseError, "Invalid numeric field: " + key);
        }
    }
    return Error(Error::Code::ParseError, "Invalid numeric field: " + key);
}

Result<ExecutionReport> parse_trade_update(const common::JsonValue& root) {
    const auto* root_obj = as_object(root);
    if (!root_obj) {
        return Error(Error::Code::ParseError, "Trade update JSON is not object");
    }
    std::string type;
    if (auto t = optional_string(*root_obj, "T"); t.is_ok()) type = t.value();
    if (type.empty()) {
        auto t2 = optional_string(*root_obj, "type");
        if (t2.is_ok()) type = t2.value();
    }
    if (type != "trade_updates" && type != "trade_update") {
        return Error(Error::Code::ParseError, "Not a trade update message");
    }

    const common::JsonValue::Object* payload = root_obj;
    if (auto* data = find_field(*root_obj, "data")) {
        if (auto* obj = data->as_object()) {
            payload = obj;
        }
    }

    const common::JsonValue::Object* order_obj = payload;
    if (auto* order = find_field(*payload, "order")) {
        if (auto* obj = order->as_object()) {
            order_obj = obj;
        }
    }

    ExecutionReport report;
    auto id = optional_string(*order_obj, "id");
    if (id.is_ok() && !id.value().empty()) {
        report.broker_order_id = id.value();
    } else {
        auto alt = optional_string(*payload, "order_id");
        if (alt.is_ok()) {
            report.broker_order_id = alt.value();
        }
    }
    if (report.broker_order_id.empty()) {
        return Error(Error::Code::ParseError, "Missing order id");
    }

    auto symbol = optional_string(*order_obj, "symbol");
    if (!symbol.is_ok() || symbol.value().empty()) {
        symbol = optional_string(*payload, "symbol");
    }
    if (!symbol.is_ok() || symbol.value().empty()) {
        return Error(Error::Code::ParseError, "Missing symbol");
    }
    report.symbol = symbol.value();

    auto side = optional_string(*order_obj, "side");
    if (!side.is_ok() || side.value().empty()) {
        side = optional_string(*payload, "side");
    }
    if (!side.is_ok() || side.value().empty()) {
        return Error(Error::Code::ParseError, "Missing side");
    }
    report.side = side.value() == "sell" ? engine::OrderSide::Sell : engine::OrderSide::Buy;

    auto qty = optional_number(*order_obj, "filled_qty", false);
    if (!qty.is_ok() || qty.value() == 0.0) {
        qty = optional_number(*order_obj, "qty", false);
    }
    if (qty.is_ok()) {
        report.quantity = qty.value();
    }

    auto price = optional_number(*order_obj, "filled_avg_price", false);
    if (!price.is_ok() || price.value() == 0.0) {
        price = optional_number(*order_obj, "price", false);
    }
    if (price.is_ok()) {
        report.price = price.value();
    }

    std::string status;
    if (auto status_val = optional_string(*order_obj, "status"); status_val.is_ok()) {
        status = status_val.value();
    }
    if (status.empty()) {
        auto event = optional_string(*payload, "event");
        if (event.is_ok()) {
            report.status = parse_trade_update_status_local(event.value());
        } else {
            return Error(Error::Code::ParseError, "Missing status/event");
        }
    } else {
        report.status = parse_order_status(status);
    }
    report.timestamp = Timestamp::now();
    return report;
}

}  // namespace

AlpacaAdapter::AlpacaAdapter(Config config) : config_(std::move(config)) {}

Result<void> AlpacaAdapter::connect() {
    if (config_.api_key.empty() || config_.secret_key.empty()) {
        return Error(Error::Code::BrokerError, "Alpaca API keys not configured");
    }
    connected_ = true;
    if (config_.enable_streaming && !config_.stream_url.empty()) {
        data::WebSocketFeed::Config stream_cfg;
        stream_cfg.url = config_.stream_url;
        stream_cfg.subscribe_template = config_.stream_subscribe_template;
        stream_cfg.unsubscribe_template = config_.stream_unsubscribe_template;
        stream_cfg.ca_bundle_path = config_.stream_ca_bundle_path;
        stream_cfg.expected_hostname = config_.stream_expected_hostname;
        stream_ = std::make_unique<data::WebSocketFeed>(stream_cfg);
        stream_->on_bar([this](const data::Bar& bar) {
            if (market_cb_) {
                MarketDataUpdate update;
                update.data = bar;
                market_cb_(update);
            }
        });
        stream_->on_tick([this](const data::Tick& tick) {
            if (market_cb_) {
                MarketDataUpdate update;
                update.data = tick;
                market_cb_(update);
            }
        });
        stream_->on_book([this](const data::OrderBook& book) {
            if (market_cb_) {
                MarketDataUpdate update;
                update.data = book;
                market_cb_(update);
            }
        });
        stream_->on_raw([this](const std::string& msg) {
            handle_stream_message(msg);
        });
        auto res = stream_->connect();
        if (res.is_err()) {
            return res;
        }
        if (!config_.stream_auth_template.empty()) {
            auto auth = replace_token(config_.stream_auth_template, "{api_key}", config_.api_key);
            auth = replace_token(auth, "{secret_key}", config_.secret_key);
            stream_->send_raw(auth);
        }
        if (!symbols_.empty()) {
            stream_->subscribe(symbols_);
        }
    }
    return Ok();
}

Result<void> AlpacaAdapter::disconnect() {
    connected_ = false;
    if (stream_) {
        stream_->disconnect();
    }
    return Ok();
}

bool AlpacaAdapter::is_connected() const {
    return connected_.load();
}

void AlpacaAdapter::subscribe_market_data(const std::vector<std::string>& symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    symbols_ = symbols;
    if (stream_) {
        stream_->subscribe(symbols);
    }
}

void AlpacaAdapter::unsubscribe_market_data(const std::vector<std::string>& symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& sym : symbols) {
        symbols_.erase(std::remove(symbols_.begin(), symbols_.end(), sym), symbols_.end());
    }
    if (stream_) {
        stream_->unsubscribe(symbols);
    }
}

Result<std::string> AlpacaAdapter::submit_order(const engine::Order& order) {
    if (!connected_) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    std::ostringstream body;
    body << "{\"symbol\":\"" << SymbolRegistry::instance().lookup(order.symbol)
         << "\",\"qty\":" << order.quantity
         << ",\"side\":\"" << (order.side == engine::OrderSide::Buy ? "buy" : "sell")
         << "\",\"type\":\"" << (order.type == engine::OrderType::Limit ? "limit" : "market")
         << "\"}";

    auto res = rest_post("/v2/orders", body.str());
    if (res.is_err()) {
        return res;
    }
    auto parsed = common::parse_json(res.value());
    if (parsed.is_ok()) {
        if (auto* obj = parsed.value().as_object()) {
            if (auto id = optional_string(*obj, "id"); id.is_ok() && !id.value().empty()) {
                return id.value();
            }
        }
    }
    return std::string("order-") + std::to_string(Timestamp::now().microseconds());
}

Result<void> AlpacaAdapter::cancel_order(const std::string& broker_order_id) {
    if (!connected_) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    auto res = rest_delete("/v2/orders/" + broker_order_id);
    if (res.is_err()) {
        const auto& err = res.error();
        Error copy(err.code, err.message, err.location);
        copy.details = err.details;
        return copy;
    }
    return Ok();
}

Result<void> AlpacaAdapter::modify_order(const std::string& broker_order_id,
                                         const engine::OrderModification& mod) {
    if (!connected_) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    std::ostringstream body;
    body << "{";
    bool first = true;
    if (mod.quantity) {
        body << "\"qty\":" << *mod.quantity;
        first = false;
    }
    if (mod.limit_price) {
        if (!first) body << ",";
        body << "\"limit_price\":" << *mod.limit_price;
        first = false;
    }
    if (mod.stop_price) {
        if (!first) body << ",";
        body << "\"stop_price\":" << *mod.stop_price;
    }
    body << "}";
    auto res = rest_post("/v2/orders/" + broker_order_id, body.str());
    if (res.is_err()) {
        const auto& err = res.error();
        Error copy(err.code, err.message, err.location);
        copy.details = err.details;
        return copy;
    }
    return Ok();
}

AccountInfo AlpacaAdapter::get_account_info() {
    AccountInfo info;
    if (!connected_) {
        return info;
    }
    auto res = rest_get("/v2/account");
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
    auto equity = optional_number(*obj, "equity", true);
    auto cash = optional_number(*obj, "cash", true);
    auto buying = optional_number(*obj, "buying_power", true);
    if (equity.is_ok()) info.equity = equity.value();
    if (cash.is_ok()) info.cash = cash.value();
    if (buying.is_ok()) info.buying_power = buying.value();
    return info;
}

std::vector<Position> AlpacaAdapter::get_positions() {
    std::vector<Position> out;
    if (!connected_) {
        return out;
    }
    auto res = rest_get("/v2/positions");
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
    out.reserve(arr->size());
    for (const auto& value : *arr) {
        auto* obj = value.as_object();
        if (!obj) {
            continue;
        }
        Position pos;
        auto symbol = require_string(*obj, "symbol");
        auto qty = optional_number(*obj, "qty", true);
        auto avg = optional_number(*obj, "avg_entry_price", false);
        auto mv = optional_number(*obj, "market_value", false);
        if (!symbol.is_ok() || !qty.is_ok()) {
            continue;
        }
        pos.symbol = symbol.value();
        pos.quantity = qty.value();
        if (avg.is_ok()) pos.average_price = avg.value();
        if (mv.is_ok()) pos.market_value = mv.value();
        out.push_back(pos);
    }
    return out;
}

std::vector<ExecutionReport> AlpacaAdapter::get_open_orders() {
    std::vector<ExecutionReport> out;
    if (!connected_) {
        return out;
    }
    auto res = rest_get("/v2/orders?status=open&limit=500");
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
    out.reserve(arr->size());
    for (const auto& value : *arr) {
        auto* obj = value.as_object();
        if (!obj) {
            continue;
        }
        auto id = require_string(*obj, "id");
        auto symbol = require_string(*obj, "symbol");
        auto side = require_string(*obj, "side");
        auto qty = optional_number(*obj, "qty", true);
        auto status = require_string(*obj, "status");
        auto avg_price = optional_number(*obj, "filled_avg_price", false);
        if (!id.is_ok() || !symbol.is_ok() || !side.is_ok() || !qty.is_ok() || !status.is_ok()) {
            continue;
        }
        ExecutionReport report;
        report.broker_order_id = id.value();
        report.symbol = symbol.value();
        report.side = side.value() == "sell" ? engine::OrderSide::Sell : engine::OrderSide::Buy;
        report.quantity = qty.value();
        if (avg_price.is_ok()) report.price = avg_price.value();
        report.status = parse_order_status(status.value());
        report.timestamp = Timestamp::now();
        out.push_back(report);
    }
    return out;
}

void AlpacaAdapter::on_market_data(std::function<void(const MarketDataUpdate&)> cb) {
    market_cb_ = std::move(cb);
}

void AlpacaAdapter::on_execution_report(std::function<void(const ExecutionReport&)> cb) {
    exec_cb_ = std::move(cb);
}

void AlpacaAdapter::on_position_update(std::function<void(const Position&)> cb) {
    position_cb_ = std::move(cb);
}

int AlpacaAdapter::max_orders_per_second() const {
    return 10;
}

int AlpacaAdapter::max_messages_per_second() const {
    return 30;
}

void AlpacaAdapter::poll() {
    if (!connected_ || !market_cb_) {
        return;
    }
    if (stream_) {
        stream_->poll();
    }
}

Result<std::string> AlpacaAdapter::rest_get(const std::string& path) const {
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
    headers = curl_slist_append(headers, ("APCA-API-KEY-ID: " + config_.api_key).c_str());
    headers = curl_slist_append(headers, ("APCA-API-SECRET-KEY: " + config_.secret_key).c_str());
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

Result<std::string> AlpacaAdapter::rest_post(const std::string& path, const std::string& body) const {
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
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("APCA-API-KEY-ID: " + config_.api_key).c_str());
    headers = curl_slist_append(headers, ("APCA-API-SECRET-KEY: " + config_.secret_key).c_str());
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

Result<std::string> AlpacaAdapter::rest_delete(const std::string& path) const {
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
    headers = curl_slist_append(headers, ("APCA-API-KEY-ID: " + config_.api_key).c_str());
    headers = curl_slist_append(headers, ("APCA-API-SECRET-KEY: " + config_.secret_key).c_str());
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

LiveOrderStatus AlpacaAdapter::parse_trade_update_status(const std::string& value) {
    return parse_trade_update_status_local(value);
}

void AlpacaAdapter::handle_stream_message(const std::string& msg) {
    if (!exec_cb_) {
        return;
    }
    auto parsed = common::parse_json(msg);
    if (parsed.is_err()) {
        return;
    }
    auto report = parse_trade_update(parsed.value());
    if (report.is_ok()) {
        exec_cb_(report.value());
    }
}

}  // namespace regimeflow::live
