// Alpaca data fetch helper for local testing.

#include "regimeflow/common/json.h"
#include "regimeflow/data/alpaca_data_client.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::string trim(std::string value) {
    const char* ws = " \t\r\n";
    value.erase(0, value.find_first_not_of(ws));
    value.erase(value.find_last_not_of(ws) + 1);
    return value;
}

std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream hex;
                    hex << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(c));
                    out += hex.str();
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void serialize_json(const regimeflow::common::JsonValue& value, std::ostream& out) {
    if (value.is_null()) {
        out << "null";
        return;
    }
    if (const auto* b = value.as_bool()) {
        out << (*b ? "true" : "false");
        return;
    }
    if (const auto* n = value.as_number()) {
        out << std::setprecision(15) << *n;
        return;
    }
    if (const auto* s = value.as_string()) {
        out << "\"" << json_escape(*s) << "\"";
        return;
    }
    if (const auto* arr = value.as_array()) {
        out << "[";
        for (size_t i = 0; i < arr->size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            serialize_json((*arr)[i], out);
        }
        out << "]";
        return;
    }
    if (const auto* obj = value.as_object()) {
        out << "{";
        size_t i = 0;
        for (const auto& [k, v] : *obj) {
            if (i++ > 0) {
                out << ",";
            }
            out << "\"" << json_escape(k) << "\":";
            serialize_json(v, out);
        }
        out << "}";
        return;
    }
}

void load_dotenv(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                                  (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        if (key.empty()) {
            continue;
        }
        if (std::getenv(key.c_str()) != nullptr) {
            continue;
        }
        setenv(key.c_str(), value.c_str(), 0);
    }
}

std::string getenv_or_default(const char* key, const std::string& fallback) {
    const char* value = std::getenv(key);
    if (!value) {
        return fallback;
    }
    return std::string(value);
}

std::vector<std::string> split_symbols(const std::string& value) {
    std::vector<std::string> out;
    std::istringstream stream(value);
    std::string token;
    while (std::getline(stream, token, ',')) {
        if (!token.empty()) {
            out.push_back(token);
        }
    }
    return out;
}

void print_usage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " [options]\n"
              << "Options:\n"
              << "  --symbols=SYM1,SYM2   Symbols (default: AAPL)\n"
              << "  --start=YYYY-MM-DD    Start date (default: 2024-01-01)\n"
              << "  --end=YYYY-MM-DD      End date (default: 2024-01-05)\n"
              << "  --timeframe=TF        Timeframe (default: 1Day)\n"
              << "  --limit=N             Limit per page (default: 0)\n"
              << "  --list-assets         Fetch asset list\n"
              << "  --bars                Fetch bars\n"
              << "  --trades              Fetch trades\n"
              << "  --snapshot            Fetch snapshot (first symbol)\n";
}

}  // namespace

int main(int argc, char** argv) {
    load_dotenv(".env");

    std::string symbols_value = "AAPL";
    std::string start = "2024-01-01";
    std::string end = "2024-01-05";
    std::string timeframe = "1Day";
    int limit = 0;
    bool do_list_assets = false;
    bool do_bars = false;
    bool do_trades = false;
    bool do_snapshot = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        if (arg == "--list-assets") {
            do_list_assets = true;
        } else if (arg == "--bars") {
            do_bars = true;
        } else if (arg == "--trades") {
            do_trades = true;
        } else if (arg == "--snapshot") {
            do_snapshot = true;
        } else if (arg.rfind("--symbols=", 0) == 0) {
            symbols_value = arg.substr(std::string("--symbols=").size());
        } else if (arg.rfind("--start=", 0) == 0) {
            start = arg.substr(std::string("--start=").size());
        } else if (arg.rfind("--end=", 0) == 0) {
            end = arg.substr(std::string("--end=").size());
        } else if (arg.rfind("--timeframe=", 0) == 0) {
            timeframe = arg.substr(std::string("--timeframe=").size());
        } else if (arg.rfind("--limit=", 0) == 0) {
            limit = std::stoi(arg.substr(std::string("--limit=").size()));
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!do_list_assets && !do_bars && !do_trades && !do_snapshot) {
        do_list_assets = true;
        do_bars = true;
        do_trades = true;
        do_snapshot = true;
    }

    regimeflow::data::AlpacaDataClient::Config cfg;
    cfg.api_key = getenv_or_default("ALPACA_API_KEY", "");
    cfg.secret_key = getenv_or_default("ALPACA_API_SECRET", "");
    cfg.trading_base_url = getenv_or_default("ALPACA_TRADING_BASE_URL",
                                             "https://paper-api.alpaca.markets");
    cfg.data_base_url = getenv_or_default("ALPACA_DATA_BASE_URL",
                                          "https://data.alpaca.markets");
    cfg.timeout_seconds = 10;

    regimeflow::data::AlpacaDataClient client(cfg);
    auto symbols = split_symbols(symbols_value);
    if (symbols.empty()) {
        symbols.push_back("AAPL");
    }

    if (do_list_assets) {
        auto res = client.list_assets();
        if (res.is_err()) {
            std::cerr << "list_assets error: " << res.error().to_string() << "\n";
        } else {
            std::cout << "assets:\n" << res.value() << "\n";
        }
    }

    if (do_bars) {
        std::unordered_map<std::string, regimeflow::common::JsonValue::Array> merged;
        std::string page_token;
        std::string last_token;
        constexpr int kMaxPages = 2000;
        for (int page = 0; page < kMaxPages; ++page) {
            auto res = client.get_bars(symbols, timeframe, start, end, limit, page_token);
            if (res.is_err()) {
                std::cerr << "get_bars error: " << res.error().to_string() << "\n";
                break;
            }
            auto parsed = regimeflow::common::parse_json(res.value());
            if (parsed.is_err()) {
                std::cerr << "get_bars parse error\n";
                break;
            }
            const auto* root = parsed.value().as_object();
            if (!root) {
                std::cerr << "get_bars invalid JSON\n";
                break;
            }
            auto it = root->find("bars");
            if (it != root->end()) {
                if (const auto* bars_obj = it->second.as_object()) {
                    for (const auto& [sym, arr_value] : *bars_obj) {
                        if (const auto* arr = arr_value.as_array()) {
                            auto& out_arr = merged[sym];
                            out_arr.insert(out_arr.end(), arr->begin(), arr->end());
                        }
                    }
                }
            }
            std::string next_token;
            auto token_it = root->find("next_page_token");
            if (token_it != root->end()) {
                if (const auto* token = token_it->second.as_string()) {
                    next_token = *token;
                }
            }
            if (next_token.empty() || next_token == last_token) {
                break;
            }
            last_token = next_token;
            page_token = next_token;
        }

        regimeflow::common::JsonValue::Object bars_obj;
        for (const auto& [sym, arr] : merged) {
            bars_obj.emplace(sym, regimeflow::common::JsonValue(arr));
        }
        regimeflow::common::JsonValue::Object root;
        root.emplace("bars", regimeflow::common::JsonValue(bars_obj));
        root.emplace("next_page_token", regimeflow::common::JsonValue(nullptr));
        std::ostringstream out;
        serialize_json(regimeflow::common::JsonValue(root), out);
        std::cout << "bars:\n" << out.str() << "\n";
    }

    if (do_trades) {
        std::unordered_map<std::string, regimeflow::common::JsonValue::Array> merged;
        std::string page_token;
        std::string last_token;
        constexpr int kMaxPages = 2000;
        for (int page = 0; page < kMaxPages; ++page) {
            auto res = client.get_trades(symbols, start, end, limit, page_token);
            if (res.is_err()) {
                std::cerr << "get_trades error: " << res.error().to_string() << "\n";
                break;
            }
            auto parsed = regimeflow::common::parse_json(res.value());
            if (parsed.is_err()) {
                std::cerr << "get_trades parse error\n";
                break;
            }
            const auto* root = parsed.value().as_object();
            if (!root) {
                std::cerr << "get_trades invalid JSON\n";
                break;
            }
            auto it = root->find("trades");
            if (it != root->end()) {
                if (const auto* trades_obj = it->second.as_object()) {
                    for (const auto& [sym, arr_value] : *trades_obj) {
                        if (const auto* arr = arr_value.as_array()) {
                            auto& out_arr = merged[sym];
                            out_arr.insert(out_arr.end(), arr->begin(), arr->end());
                        }
                    }
                }
            }
            std::string next_token;
            auto token_it = root->find("next_page_token");
            if (token_it != root->end()) {
                if (const auto* token = token_it->second.as_string()) {
                    next_token = *token;
                }
            }
            if (next_token.empty() || next_token == last_token) {
                break;
            }
            last_token = next_token;
            page_token = next_token;
        }

        regimeflow::common::JsonValue::Object trades_obj;
        for (const auto& [sym, arr] : merged) {
            trades_obj.emplace(sym, regimeflow::common::JsonValue(arr));
        }
        regimeflow::common::JsonValue::Object root;
        root.emplace("trades", regimeflow::common::JsonValue(trades_obj));
        root.emplace("next_page_token", regimeflow::common::JsonValue(nullptr));
        std::ostringstream out;
        serialize_json(regimeflow::common::JsonValue(root), out);
        std::cout << "trades:\n" << out.str() << "\n";
    }

    if (do_snapshot) {
        auto res = client.get_snapshot(symbols.front());
        if (res.is_err()) {
            std::cerr << "get_snapshot error: " << res.error().to_string() << "\n";
        } else {
            std::cout << "snapshot:\n" << res.value() << "\n";
        }
    }

    return 0;
}
