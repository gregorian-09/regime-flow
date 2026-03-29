#include "regimeflow/live/binance_adapter.h"

#include "regimeflow/common/json.h"
#include "regimeflow/common/types.h"

#include <algorithm>
#include <array>
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

namespace regimeflow::live
{
    namespace {

#ifdef REGIMEFLOW_USE_CURL
        size_t curl_write_cb(const char* ptr, const size_t size, const size_t nmemb, void* userdata) {
            auto* out = static_cast<std::string*>(userdata);
            out->append(ptr, size * nmemb);
            return size * nmemb;
        }
#endif

        std::string to_upper(std::string value) {
            std::ranges::transform(value, value.begin(),
                                   [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return value;
        }

        std::string to_lower(std::string value) {
            std::ranges::transform(value, value.begin(),
                                   [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        const common::JsonValue* find_field(const common::JsonValue::Object& obj,
                                            const std::string& key) {
            const auto it = obj.find(key);
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
            if (const auto str = value->as_string()) {
                return *str;
            }
            return {};
        }

        bool get_number(const common::JsonValue::Object& obj, const std::string& key, double& out) {
            auto* value = find_field(obj, key);
            if (!value) {
                return false;
            }
            if (const auto num = value->as_number()) {
                out = *num;
                return true;
            }
            if (const auto str = value->as_string()) {
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

        std::string normalize_symbol(std::string value) {
            return to_upper(std::move(value));
        }

#ifdef REGIMEFLOW_USE_CURL
        Error make_binance_transport_error(const char* operation, const CURLcode code) {
            Error error(code == CURLE_OPERATION_TIMEDOUT ? Error::Code::TimeoutError
                                                         : Error::Code::NetworkError,
                        std::string("Binance ")
                            .append(operation)
                            .append(" transport failure: ")
                            .append(curl_easy_strerror(code)));
            error.details = std::string("category=")
                .append(code == CURLE_OPERATION_TIMEDOUT ? "timeout" : "network")
                .append(";operation=")
                .append(operation)
                .append(";curl_code=")
                .append(std::to_string(static_cast<int>(code)));
            return error;
        }
#endif

        Error make_binance_http_error(const char* operation,
                                      const long status,
                                      const std::string& response) {
            Error error;
            if (status == 401 || status == 403) {
                error = Error(Error::Code::BrokerError, "Binance authentication failed");
                error.details = std::string("category=auth;operation=").append(operation);
            } else if (status == 408 || status == 504) {
                error = Error(Error::Code::TimeoutError, "Binance request timed out");
                error.details = std::string("category=timeout;operation=").append(operation);
            } else if (status == 429 || status == 418) {
                error = Error(Error::Code::BrokerError, "Binance rate limit exceeded");
                error.details = std::string("category=rate_limit;operation=").append(operation);
            } else if (status >= 400 && status < 500) {
                error = Error(Error::Code::BrokerError, "Binance rejected request");
                error.details = std::string("category=rejection;operation=").append(operation);
            } else {
                error = Error(Error::Code::NetworkError, "Binance upstream HTTP failure");
                error.details = std::string("category=network;operation=").append(operation);
            }
            error.details = error.details.value() + ";http_status=" + std::to_string(status)
                + ";response=" + response;
            return error;
        }

        constexpr std::array<std::string_view, 6> kStableQuotes = {
            "USDT", "USDC", "FDUSD", "BUSD", "TUSD", "USD"
        };

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
            if (auto res = stream_->connect(); res.is_err()) {
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
            symbols_.erase(std::ranges::remove(symbols_,
                                               build_trade_stream_symbol(sym)).begin(),
                           symbols_.end());
            raw_symbols_.erase(std::ranges::remove(raw_symbols_, sym).begin(),
                               raw_symbols_.end());
        }
        if (stream_) {
            stream_->unsubscribe(stream_symbols);
        }
    }

    Result<std::string> BinanceAdapter::submit_order(const engine::Order& order) {
        if (!connected_) {
            return Result<std::string>(Error(Error::Code::BrokerError, "Not connected"));
        }
        if (config_.api_key.empty() || config_.secret_key.empty()) {
            return Result<std::string>(Error(Error::Code::BrokerError, "Binance API keys not configured"));
        }
        std::ostringstream query;
        std::string symbol = to_upper(SymbolRegistry::instance().lookup(order.symbol));
        if (symbol.empty()) {
            return Result<std::string>(Error(Error::Code::InvalidArgument, "Invalid symbol"));
        }
        query << "symbol=" << symbol;
        query << "&side=" << (order.side == engine::OrderSide::Buy ? "BUY" : "SELL");
        query << "&type=" << (order.type == engine::OrderType::Limit ? "LIMIT" : "MARKET");
        query << "&quantity=" << order.quantity;
        if (order.type == engine::OrderType::Limit && order.limit_price > 0.0) {
            query << "&price=" << order.limit_price;
            const auto tif = [&]() -> std::string {
                switch (order.tif) {
                case engine::TimeInForce::GTC: return "GTC";
                case engine::TimeInForce::IOC: return "IOC";
                case engine::TimeInForce::FOK: return "FOK";
                default: return "GTC";
                }
            }();
            query << "&timeInForce=" << tif;
        }
        query << "&timestamp=" << Timestamp::now().milliseconds();
        query << "&recvWindow=" << config_.recv_window_ms;
        auto res = signed_post("/api/v3/order", query.str());
        if (res.is_err()) {
            return res;
        }
        auto order_id = parse_submitted_order_id(res.value());
        if (order_id.is_err()) {
            return order_id;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        order_symbols_[order_id.value()] = symbol;
        return order_id;
    }

    Result<void> BinanceAdapter::cancel_order(const std::string& broker_order_id) {
        if (!connected_) {
            return Result<void>(Error(Error::Code::BrokerError, "Not connected"));
        }
        if (config_.api_key.empty() || config_.secret_key.empty()) {
            return Result<void>(Error(Error::Code::BrokerError, "Binance API keys not configured"));
        }
        if (broker_order_id.empty()) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Missing broker order id"));
        }
        std::string symbol;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (const auto it = order_symbols_.find(broker_order_id); it != order_symbols_.end()) {
                symbol = it->second;
            }
        }
        if (symbol.empty()) {
            for (const auto& report : get_open_orders()) {
                if (report.broker_order_id == broker_order_id) {
                    symbol = normalize_symbol(report.symbol);
                    break;
                }
            }
        }
        if (symbol.empty()) {
            return Result<void>(Error(Error::Code::InvalidState,
                                      "Binance order-symbol mapping required to cancel order"));
        }
        std::ostringstream query;
        query << "symbol=" << symbol;
        query << "&orderId=" << broker_order_id;
        query << "&timestamp=" << Timestamp::now().milliseconds();
        query << "&recvWindow=" << config_.recv_window_ms;
        if (auto res = signed_delete("/api/v3/order", query.str()); res.is_err()) {
            const auto& err = res.error();
            Error copy(err.code, err.message, err.location);
            copy.details = err.details;
            return Result<void>(copy);
        }
        return Ok();
    }

    Result<void> BinanceAdapter::modify_order(const std::string& broker_order_id,
                                              const engine::OrderModification& mod) {
        if (!connected_) {
            return Result<void>(Error(Error::Code::BrokerError, "Not connected"));
        }
        if (config_.api_key.empty() || config_.secret_key.empty()) {
            return Result<void>(Error(Error::Code::BrokerError, "Binance API keys not configured"));
        }
        if (broker_order_id.empty()) {
            return Result<void>(Error(Error::Code::InvalidArgument, "Missing broker order id"));
        }
        if (!mod.quantity && !mod.limit_price && !mod.stop_price) {
            return Ok();
        }
        return Result<void>(Error(Error::Code::InvalidState, "Binance order modification not supported"));
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
        if (auto* balances = find_field(*obj, "balances"); balances && balances->as_array()) {
            double cash = 0.0;
            double equity = 0.0;
            for (const auto& entry : *balances->as_array()) {
                auto* bal = entry.as_object();
                if (!bal) {
                    continue;
                }
                std::string asset = normalize_symbol(get_string(*bal, "asset"));
                double free_amt = 0.0;
                double locked_amt = 0.0;
                get_number(*bal, "free", free_amt);
                get_number(*bal, "locked", locked_amt);
                const double total = free_amt + locked_amt;
                if (total <= 0.0) {
                    continue;
                }
                bool is_cash_asset = false;
                for (const auto quote : kStableQuotes) {
                    if (asset == quote) {
                        is_cash_asset = true;
                        break;
                    }
                }
                if (is_cash_asset) {
                    cash += total;
                    equity += total;
                    continue;
                }
                const std::string resolved_symbol = resolve_balance_symbol(asset);
                if (resolved_symbol.empty()) {
                    continue;
                }
                if (const auto price = fetch_public_price(resolved_symbol)) {
                    equity += total * *price;
                }
            }
            info.cash = cash;
            info.equity = equity > 0.0 ? equity : cash;
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
            const std::string asset = normalize_symbol(get_string(*bal, "asset"));
            double free_amt = 0.0;
            double locked_amt = 0.0;
            get_number(*bal, "free", free_amt);
            get_number(*bal, "locked", locked_amt);
            double total = free_amt + locked_amt;
            if (total <= 0.0) {
                continue;
            }
            Position pos;
            const std::string resolved_symbol = resolve_balance_symbol(asset);
            pos.symbol = resolved_symbol.empty() ? asset : resolved_symbol;
            pos.quantity = total;
            if (!resolved_symbol.empty()) {
                if (const auto price = fetch_public_price(resolved_symbol)) {
                    pos.market_value = total * *price;
                }
            }
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
            report.symbol = normalize_symbol(get_string(*obj, "symbol"));
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
                std::lock_guard<std::mutex> lock(mutex_);
                order_symbols_[report.broker_order_id] = report.symbol;
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

    bool BinanceAdapter::supports_tif(const engine::OrderType type, const engine::TimeInForce tif) const {
        if (tif == engine::TimeInForce::GTD) {
            return false;
        }
        if (type == engine::OrderType::Limit) {
            return tif == engine::TimeInForce::GTC
                || tif == engine::TimeInForce::IOC
                || tif == engine::TimeInForce::FOK;
        }
        if (type == engine::OrderType::Market) {
            // Binance market orders do not carry a timeInForce field on submit.
            // Accept the engine default Day setting so default market orders remain
            // broker-compatible without caller-side mutation.
            return tif == engine::TimeInForce::Day
                || tif == engine::TimeInForce::IOC;
        }
        return false;
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
            return Result<std::string>(Error(Error::Code::NetworkError, "curl init failed"));
        }
        std::string response;
        const std::string url = config_.base_url + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        struct curl_slist* headers = nullptr;
        if (!config_.api_key.empty()) {
            headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        const CURLcode res = curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        if (headers) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return Result<std::string>(make_binance_transport_error("rest_get", res));
        }
        if (status < 200 || status >= 300) {
            return Result<std::string>(make_binance_http_error("rest_get", status, response));
        }
        return Result<std::string>(response);
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
            return Result<std::string>(Error(Error::Code::NetworkError, "curl init failed"));
        }
        std::string response;
        const std::string url = config_.base_url + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        const CURLcode res = curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return Result<std::string>(make_binance_transport_error("rest_post", res));
        }
        if (status < 200 || status >= 300) {
            return Result<std::string>(make_binance_http_error("rest_post", status, response));
        }
        return Result<std::string>(response);
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
            return Result<std::string>(Error(Error::Code::NetworkError, "curl init failed"));
        }
        std::string response;
        const std::string url = config_.base_url + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        const CURLcode res = curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return Result<std::string>(make_binance_transport_error("rest_delete", res));
        }
        if (status < 200 || status >= 300) {
            return Result<std::string>(make_binance_http_error("rest_delete", status, response));
        }
        return Result<std::string>(response);
#else
        (void)path;
        return Error(Error::Code::NetworkError, "libcurl not enabled");
#endif
    }

    Result<std::string> BinanceAdapter::signed_get(const std::string& path,
                                                   const std::string& query) const {
#ifdef REGIMEFLOW_USE_OPENSSL
        const std::string signature = hmac_sha256_hex(config_.secret_key, query);
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
        const std::string signature = hmac_sha256_hex(config_.secret_key, query);
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
        const std::string signature = hmac_sha256_hex(config_.secret_key, query);
        return rest_delete(path + "?" + query + "&signature=" + signature);
#else
        (void)path;
        (void)query;
        return Error(Error::Code::InvalidState, "OpenSSL not enabled for Binance signing");
#endif
    }

    Result<std::string> BinanceAdapter::parse_submitted_order_id(const std::string& payload) {
        auto parsed = common::parse_json(payload);
        if (parsed.is_err()) {
            Error error(Error::Code::ParseError, "Unable to parse Binance submit response");
            error.details = "category=schema;operation=parse_submit_response";
            return Result<std::string>(std::move(error));
        }
        auto* obj = parsed.value().as_object();
        if (!obj) {
            Error error(Error::Code::ParseError, "Binance submit response must be a JSON object");
            error.details = "category=schema;operation=parse_submit_response";
            return Result<std::string>(std::move(error));
        }
        const auto id = get_string(*obj, "orderId");
        if (id.empty()) {
            Error error(Error::Code::ParseError, "Binance submit response missing broker order id");
            error.details = "category=schema;operation=parse_submit_response";
            return Result<std::string>(std::move(error));
        }
        return Result<std::string>(id);
    }

    std::string BinanceAdapter::resolve_balance_symbol(const std::string& asset) const {
        if (asset.empty()) {
            return {};
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& configured : raw_symbols_) {
                const std::string symbol = normalize_symbol(configured);
                if (symbol.rfind(asset, 0) != 0) {
                    continue;
                }
                for (const auto quote : kStableQuotes) {
                    if (symbol.size() > asset.size() && symbol.ends_with(quote)) {
                        return symbol;
                    }
                }
            }
        }
        return {};
    }

    std::optional<double> BinanceAdapter::fetch_public_price(const std::string& symbol) const {
        if (symbol.empty()) {
            return std::nullopt;
        }
        const auto response = rest_get("/api/v3/ticker/price?symbol=" + symbol);
        if (response.is_err()) {
            return std::nullopt;
        }
        auto parsed = common::parse_json(response.value());
        if (parsed.is_err()) {
            return std::nullopt;
        }
        const auto* obj = parsed.value().as_object();
        if (!obj) {
            return std::nullopt;
        }
        double price = 0.0;
        if (!get_number(*obj, "price", price) || price <= 0.0) {
            return std::nullopt;
        }
        return price;
    }

    void BinanceAdapter::handle_stream_message(const std::string& msg) const
    {
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
        if (std::string event = get_string(*payload, "e"); event != "trade" && event != "aggTrade") {
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
