#include "regimeflow/data/alpaca_data_client.h"

#include <algorithm>
#include <mutex>
#include <sstream>

#ifdef REGIMEFLOW_USE_CURL
#include <curl/curl.h>
#endif

namespace regimeflow::data {

namespace {

#ifdef REGIMEFLOW_USE_CURL
size_t curl_write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* out = static_cast<std::string*>(userp);
    out->append(static_cast<const char*>(contents), total);
    return total;
}
#endif

std::string join_symbols(const std::vector<std::string>& symbols) {
    std::ostringstream out;
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << symbols[i];
    }
    return out.str();
}

}  // namespace

AlpacaDataClient::AlpacaDataClient(Config config) : config_(std::move(config)) {}

Result<std::string> AlpacaDataClient::list_assets(const std::string& status,
                                                  const std::string& asset_class) const {
    std::ostringstream query;
    if (!status.empty()) {
        query << "status=" << status;
    }
    if (!asset_class.empty()) {
        if (!query.str().empty()) {
            query << "&";
        }
        query << "asset_class=" << asset_class;
    }
    return rest_get(config_.trading_base_url, "/v2/assets", query.str());
}

Result<std::string> AlpacaDataClient::get_bars(const std::vector<std::string>& symbols,
                                               const std::string& timeframe,
                                               const std::string& start,
                                               const std::string& end,
                                               int limit,
                                               const std::string& page_token) const {
    if (symbols.empty()) {
        return Error(Error::Code::InvalidArgument, "No symbols provided");
    }
    if (timeframe.empty()) {
        return Error(Error::Code::InvalidArgument, "Missing timeframe");
    }
    std::ostringstream query;
    query << "symbols=" << join_symbols(symbols)
          << "&timeframe=" << timeframe;
    if (!start.empty()) {
        query << "&start=" << start;
    }
    if (!end.empty()) {
        query << "&end=" << end;
    }
    if (limit > 0) {
        query << "&limit=" << limit;
    }
    if (!page_token.empty()) {
        query << "&page_token=" << page_token;
    }
    return rest_get(config_.data_base_url, "/v2/stocks/bars", query.str());
}

Result<std::string> AlpacaDataClient::get_trades(const std::vector<std::string>& symbols,
                                                 const std::string& start,
                                                 const std::string& end,
                                                 int limit,
                                                 const std::string& page_token) const {
    if (symbols.empty()) {
        return Error(Error::Code::InvalidArgument, "No symbols provided");
    }
    std::ostringstream query;
    query << "symbols=" << join_symbols(symbols);
    if (!start.empty()) {
        query << "&start=" << start;
    }
    if (!end.empty()) {
        query << "&end=" << end;
    }
    if (limit > 0) {
        query << "&limit=" << limit;
    }
    if (!page_token.empty()) {
        query << "&page_token=" << page_token;
    }
    return rest_get(config_.data_base_url, "/v2/stocks/trades", query.str());
}

Result<std::string> AlpacaDataClient::get_snapshot(const std::string& symbol) const {
    if (symbol.empty()) {
        return Error(Error::Code::InvalidArgument, "Missing symbol");
    }
    std::string path = "/v2/stocks/" + symbol + "/snapshot";
    return rest_get(config_.data_base_url, path, "");
}

Result<std::string> AlpacaDataClient::rest_get(const std::string& base_url,
                                               const std::string& path,
                                               const std::string& query) const {
#ifdef REGIMEFLOW_USE_CURL
    if (config_.api_key.empty() || config_.secret_key.empty()) {
        return Error(Error::Code::ConfigError, "Alpaca API keys not configured");
    }
    if (base_url.empty()) {
        return Error(Error::Code::ConfigError, "Missing base URL");
    }
    std::string url = base_url + path;
    if (!query.empty()) {
        url += "?" + query;
    }
    static std::once_flag init_flag;
    std::call_once(init_flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Error(Error::Code::NetworkError, "curl init failed");
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    std::string key_header = "APCA-API-KEY-ID: " + config_.api_key;
    std::string secret_header = "APCA-API-SECRET-KEY: " + config_.secret_key;
    headers = curl_slist_append(headers, key_header.c_str());
    headers = curl_slist_append(headers, secret_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return Error(Error::Code::NetworkError, curl_easy_strerror(res));
    }
    if (status >= 400) {
        return Error(Error::Code::NetworkError, "HTTP error: " + std::to_string(status));
    }
    return response;
#else
    (void)base_url;
    (void)path;
    (void)query;
    return Error(Error::Code::InvalidState, "CURL disabled");
#endif
}

}  // namespace regimeflow::data
