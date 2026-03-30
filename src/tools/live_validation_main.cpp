#include "regimeflow/common/config.h"
#include "regimeflow/common/result.h"
#include "regimeflow/common/yaml_config.h"
#include "regimeflow/live/alpaca_adapter.h"
#include "regimeflow/live/binance_adapter.h"
#include "regimeflow/live/live_engine.h"
#include "regimeflow/live/secret_hygiene.h"
#if defined(REGIMEFLOW_ENABLE_IBAPI)
#include "regimeflow/live/ib_adapter.h"
#endif

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

namespace
{
    std::optional<std::string> get_string(const regimeflow::Config& cfg, const std::string& key) {
        return cfg.get_as<std::string>(key);
    }

    std::optional<int64_t> get_int(const regimeflow::Config& cfg, const std::string& key) {
        return cfg.get_as<int64_t>(key);
    }

    std::optional<double> get_double(const regimeflow::Config& cfg, const std::string& key) {
        return cfg.get_as<double>(key);
    }

    std::optional<bool> get_bool(const regimeflow::Config& cfg, const std::string& key) {
        return cfg.get_as<bool>(key);
    }

    std::vector<std::string> get_string_array(const regimeflow::Config& cfg, const std::string& key) {
        std::vector<std::string> out;
        if (const auto arr = cfg.get_as<regimeflow::ConfigValue::Array>(key); arr.has_value()) {
            for (const auto& value : *arr) {
                if (const auto* item = value.get_if<std::string>()) {
                    out.emplace_back(*item);
                }
            }
        }
        return out;
    }

    regimeflow::Config get_object_config(const regimeflow::Config& cfg, const std::string& key) {
        if (const auto obj = cfg.get_as<regimeflow::ConfigValue::Object>(key); obj.has_value()) {
            return regimeflow::Config(*obj);
        }
        return {};
    }

    std::map<std::string, std::string> get_object_map(const regimeflow::Config& cfg,
                                                      const std::string& key) {
        std::map<std::string, std::string> out;
        const auto object = cfg.get_as<regimeflow::ConfigValue::Object>(key);
        if (!object) {
            return out;
        }
        const auto flatten = [&out](const auto& self,
                                    const regimeflow::ConfigValue::Object& current,
                                    const std::string& prefix) -> void {
            for (const auto& [name, value] : current) {
                const std::string full_key = prefix.empty() ? name : prefix + "." + name;
                if (const auto* nested = value.template get_if<regimeflow::ConfigValue::Object>()) {
                    self(self, *nested, full_key);
                } else if (const auto* str = value.template get_if<std::string>()) {
                    out[full_key] = *str;
                } else if (const auto* boolean = value.template get_if<bool>()) {
                    out[full_key] = *boolean ? "true" : "false";
                } else if (const auto* integer = value.template get_if<int64_t>()) {
                    out[full_key] = std::to_string(*integer);
                } else if (const auto* number = value.template get_if<double>()) {
                    out[full_key] = std::to_string(*number);
                }
            }
        };
        flatten(flatten, *object, "");
        return out;
    }

    std::string trim(std::string value) {
        const auto whitespace = " \t\r\n";
        value.erase(0, value.find_first_not_of(whitespace));
        value.erase(value.find_last_not_of(whitespace) + 1);
        return value;
    }

    std::optional<std::string> read_env(const char* key) {
        return regimeflow::live::read_secret_env(key);
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
            const auto pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            if (key.empty() || read_env(key.c_str()).has_value()) {
                continue;
            }
            if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"')
                                   || (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }
#if defined(_WIN32)
            _putenv_s(key.c_str(), value.c_str());
#else
            setenv(key.c_str(), value.c_str(), 0);
#endif
        }
    }

    void set_broker_config_from_env(std::map<std::string, std::string>& broker_config,
                                    const std::string& key,
                                    const char* env_key) {
        if (broker_config.contains(key)) {
            return;
        }
        if (const auto value = read_env(env_key)) {
            broker_config[key] = *value;
        }
    }

#if defined(REGIMEFLOW_ENABLE_IBAPI)
    bool parse_bool(const std::string& value) {
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }

    void apply_ib_contract_field(regimeflow::live::IBAdapter::ContractConfig& contract,
                                 const std::string_view field,
                                 const std::string& value) {
        if (field == "symbol_override") {
            contract.symbol_override = value;
        } else if (field == "security_type") {
            contract.security_type = value;
        } else if (field == "exchange") {
            contract.exchange = value;
        } else if (field == "currency") {
            contract.currency = value;
        } else if (field == "primary_exchange") {
            contract.primary_exchange = value;
        } else if (field == "local_symbol") {
            contract.local_symbol = value;
        } else if (field == "trading_class") {
            contract.trading_class = value;
        } else if (field == "last_trade_date_or_contract_month") {
            contract.last_trade_date_or_contract_month = value;
        } else if (field == "right") {
            contract.right = value;
        } else if (field == "multiplier") {
            contract.multiplier = value;
        } else if (field == "strike") {
            contract.strike = std::stod(value);
        } else if (field == "con_id") {
            contract.con_id = static_cast<int32_t>(std::stoi(value));
        } else if (field == "include_expired") {
            contract.include_expired = parse_bool(value);
        }
    }
#endif

    regimeflow::Result<regimeflow::live::LiveConfig> load_live_config(const std::string& path) {
        using regimeflow::Duration;
        using regimeflow::Error;
        using regimeflow::live::LiveConfig;

        const regimeflow::Config root = regimeflow::YamlConfigLoader::load_file(path);
        LiveConfig cfg;
        const auto broker = get_string(root, "live.broker");
        if (!broker || broker->empty()) {
            return regimeflow::Result<LiveConfig>(Error(Error::Code::ConfigError, "Missing live.broker"));
        }
        cfg.broker_type = *broker;
        cfg.broker_asset_class = get_string(root, "live.broker_asset_class").value_or("equity");
        cfg.symbols = get_string_array(root, "live.symbols");
        cfg.paper_trading = get_bool(root, "live.paper").value_or(true);
        cfg.strategy_name = get_string(root, "strategy.name").value_or("buy_and_hold");
        cfg.strategy_config = get_object_config(root, "strategy.params");
        cfg.risk_config = get_object_config(root, "live.risk");
        cfg.broker_config = get_object_map(root, "live.broker_config");
        cfg.broker_config["paper"] = cfg.paper_trading ? "true" : "false";
        cfg.log_dir = get_string(root, "live.log_dir").value_or("./logs");
        cfg.max_order_value = get_double(root, "live.max_order_value").value_or(cfg.max_order_value);

        if (const auto interval_ms = get_int(root, "live.heartbeat.interval_ms")) {
            cfg.heartbeat_timeout = Duration::milliseconds(*interval_ms);
        }

        if (cfg.broker_type == "alpaca") {
            set_broker_config_from_env(cfg.broker_config, "api_key", "ALPACA_API_KEY");
            set_broker_config_from_env(cfg.broker_config, "secret_key", "ALPACA_API_SECRET");
            set_broker_config_from_env(cfg.broker_config, "base_url", "ALPACA_PAPER_BASE_URL");
            set_broker_config_from_env(cfg.broker_config, "stream_url", "ALPACA_STREAM_URL");
        } else if (cfg.broker_type == "binance") {
            set_broker_config_from_env(cfg.broker_config, "api_key", "BINANCE_API_KEY");
            set_broker_config_from_env(cfg.broker_config, "secret_key", "BINANCE_SECRET_KEY");
            set_broker_config_from_env(cfg.broker_config, "base_url", "BINANCE_BASE_URL");
            set_broker_config_from_env(cfg.broker_config, "stream_url", "BINANCE_STREAM_URL");
            set_broker_config_from_env(cfg.broker_config, "recv_window_ms", "BINANCE_RECV_WINDOW_MS");
        } else if (cfg.broker_type == "ib") {
            set_broker_config_from_env(cfg.broker_config, "host", "IB_HOST");
            set_broker_config_from_env(cfg.broker_config, "port", "IB_PORT");
            set_broker_config_from_env(cfg.broker_config, "client_id", "IB_CLIENT_ID");
        }

        if (const auto resolved = regimeflow::live::resolve_secret_config(cfg.broker_config); resolved.is_err()) {
            return regimeflow::Result<LiveConfig>(resolved.error());
        }
        regimeflow::live::register_sensitive_config(cfg.broker_config);
        return regimeflow::Result<LiveConfig>(std::move(cfg));
    }

    std::unique_ptr<regimeflow::live::BrokerAdapter> build_broker(const regimeflow::live::LiveConfig& config) {
        using regimeflow::live::AlpacaAdapter;
        using regimeflow::live::BinanceAdapter;
        if (config.broker_type == "alpaca") {
            AlpacaAdapter::Config cfg;
            if (const auto it = config.broker_config.find("api_key"); it != config.broker_config.end()) cfg.api_key = it->second;
            if (const auto it = config.broker_config.find("secret_key"); it != config.broker_config.end()) cfg.secret_key = it->second;
            if (const auto it = config.broker_config.find("base_url"); it != config.broker_config.end()) cfg.base_url = it->second;
            if (const auto it = config.broker_config.find("data_url"); it != config.broker_config.end()) cfg.data_url = it->second;
            if (const auto it = config.broker_config.find("stream_url"); it != config.broker_config.end()) cfg.stream_url = it->second;
            if (const auto it = config.broker_config.find("stream_auth_template"); it != config.broker_config.end()) cfg.stream_auth_template = it->second;
            if (const auto it = config.broker_config.find("stream_subscribe_template"); it != config.broker_config.end()) cfg.stream_subscribe_template = it->second;
            if (const auto it = config.broker_config.find("stream_unsubscribe_template"); it != config.broker_config.end()) cfg.stream_unsubscribe_template = it->second;
            if (const auto it = config.broker_config.find("stream_ca_bundle_path"); it != config.broker_config.end()) cfg.stream_ca_bundle_path = it->second;
            if (const auto it = config.broker_config.find("stream_expected_hostname"); it != config.broker_config.end()) cfg.stream_expected_hostname = it->second;
            if (const auto it = config.broker_config.find("enable_streaming"); it != config.broker_config.end()) cfg.enable_streaming = (it->second == "true");
            if (const auto it = config.broker_config.find("paper"); it != config.broker_config.end()) cfg.paper = (it->second == "true");
            if (const auto it = config.broker_config.find("timeout_seconds"); it != config.broker_config.end()) cfg.timeout_seconds = std::stoi(it->second);
            cfg.asset_class = config.broker_asset_class;
            return std::make_unique<AlpacaAdapter>(std::move(cfg));
        }
        if (config.broker_type == "binance") {
            BinanceAdapter::Config cfg;
            if (const auto it = config.broker_config.find("api_key"); it != config.broker_config.end()) cfg.api_key = it->second;
            if (const auto it = config.broker_config.find("secret_key"); it != config.broker_config.end()) cfg.secret_key = it->second;
            if (const auto it = config.broker_config.find("base_url"); it != config.broker_config.end()) cfg.base_url = it->second;
            if (const auto it = config.broker_config.find("stream_url"); it != config.broker_config.end()) cfg.stream_url = it->second;
            if (const auto it = config.broker_config.find("stream_subscribe_template"); it != config.broker_config.end()) cfg.stream_subscribe_template = it->second;
            if (const auto it = config.broker_config.find("stream_unsubscribe_template"); it != config.broker_config.end()) cfg.stream_unsubscribe_template = it->second;
            if (const auto it = config.broker_config.find("stream_ca_bundle_path"); it != config.broker_config.end()) cfg.stream_ca_bundle_path = it->second;
            if (const auto it = config.broker_config.find("stream_expected_hostname"); it != config.broker_config.end()) cfg.stream_expected_hostname = it->second;
            if (const auto it = config.broker_config.find("enable_streaming"); it != config.broker_config.end()) cfg.enable_streaming = (it->second == "true");
            if (const auto it = config.broker_config.find("timeout_seconds"); it != config.broker_config.end()) cfg.timeout_seconds = std::stoi(it->second);
            if (const auto it = config.broker_config.find("recv_window_ms"); it != config.broker_config.end()) cfg.recv_window_ms = std::stoll(it->second);
            return std::make_unique<BinanceAdapter>(std::move(cfg));
        }
#if defined(REGIMEFLOW_ENABLE_IBAPI)
        if (config.broker_type == "ib") {
            regimeflow::live::IBAdapter::Config cfg;
            if (const auto it = config.broker_config.find("host"); it != config.broker_config.end()) cfg.host = it->second;
            if (const auto it = config.broker_config.find("port"); it != config.broker_config.end()) cfg.port = std::stoi(it->second);
            if (const auto it = config.broker_config.find("client_id"); it != config.broker_config.end()) cfg.client_id = std::stoi(it->second);
            for (const auto& [key, value] : config.broker_config) {
                if (!key.starts_with("defaults.") && !key.starts_with("contracts.")) {
                    continue;
                }
                if (key.starts_with("defaults.")) {
                    apply_ib_contract_field(cfg.default_contract,
                                            std::string_view(key).substr(std::string_view("defaults.").size()),
                                            value);
                    continue;
                }
                const std::string_view remainder =
                    std::string_view(key).substr(std::string_view("contracts.").size());
                const auto dot_pos = remainder.find('.');
                if (dot_pos == std::string_view::npos || dot_pos == 0 || dot_pos + 1 >= remainder.size()) {
                    continue;
                }
                const std::string symbol(remainder.substr(0, dot_pos));
                apply_ib_contract_field(cfg.contracts[symbol], remainder.substr(dot_pos + 1), value);
            }
            return std::make_unique<regimeflow::live::IBAdapter>(std::move(cfg));
        }
#endif
        return nullptr;
    }

    struct ValidationOptions {
        std::string config_path;
        std::string mode = "submit_cancel_reconcile";
        std::optional<double> quantity;
        std::optional<double> limit_price;
        int timeout_seconds = 30;
    };

    struct ProbeState {
        std::mutex mutex;
        double last_price = 0.0;
        std::vector<regimeflow::live::ExecutionReport> reports;
    };

    void record_market_price(ProbeState& state, const regimeflow::live::MarketDataUpdate& update) {
        std::visit([&](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            double price = 0.0;
            if constexpr (std::is_same_v<T, regimeflow::data::Bar>) {
                price = data.close;
            } else if constexpr (std::is_same_v<T, regimeflow::data::Tick>) {
                price = data.price;
            } else if constexpr (std::is_same_v<T, regimeflow::data::Quote>) {
                price = data.mid();
            } else if constexpr (std::is_same_v<T, regimeflow::data::OrderBook>) {
                if (!data.bids.empty() && !data.asks.empty()) {
                    price = (data.bids.front().price + data.asks.front().price) / 2.0;
                }
            }
            if (price > 0.0) {
                std::lock_guard<std::mutex> lock(state.mutex);
                state.last_price = price;
            }
        }, update.data);
    }

    void broker_poll_sleep(regimeflow::live::BrokerAdapter& broker,
                           const std::chrono::milliseconds sleep_for = std::chrono::milliseconds(100)) {
        broker.poll();
        std::this_thread::sleep_for(sleep_for);
    }

    template<typename Predicate>
    bool wait_for(regimeflow::live::BrokerAdapter& broker,
                  const std::chrono::seconds timeout,
                  Predicate&& predicate) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate()) {
                return true;
            }
            broker_poll_sleep(broker);
        }
        return predicate();
    }

    regimeflow::engine::TimeInForce market_tif_for_broker(const regimeflow::live::LiveConfig& config) {
        if (config.broker_type == "binance") {
            return regimeflow::engine::TimeInForce::IOC;
        }
        if (config.broker_type == "alpaca" && config.broker_asset_class == "crypto") {
            return regimeflow::engine::TimeInForce::IOC;
        }
        return regimeflow::engine::TimeInForce::Day;
    }

    double default_quantity_for(const regimeflow::live::LiveConfig& config) {
        return config.broker_asset_class == "crypto" ? 0.001 : 1.0;
    }

    std::string render_error(const regimeflow::Error& error) {
        std::string rendered = regimeflow::live::redact_sensitive_values(error.to_string());
        if (error.details && !error.details->empty()) {
            rendered.append(" details=")
                .append(regimeflow::live::redact_sensitive_values(*error.details));
        }
        return rendered;
    }

    int run_validation(const ValidationOptions& options) {
        load_dotenv(".env");
        const auto config_result = load_live_config(options.config_path);
        if (config_result.is_err()) {
            std::cerr << "Config error: " << render_error(config_result.error()) << "\n";
            return 1;
        }
        const auto& cfg = config_result.value();
        auto broker = build_broker(cfg);
        if (!broker) {
            std::cerr << "Unable to construct broker adapter for " << cfg.broker_type << "\n";
            return 1;
        }

        ProbeState state;
        broker->on_market_data([&](const regimeflow::live::MarketDataUpdate& update) {
            record_market_price(state, update);
        });
        broker->on_execution_report([&](const regimeflow::live::ExecutionReport& report) {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.reports.push_back(report);
        });

        if (const auto connect = broker->connect(); connect.is_err()) {
            std::cerr << "Connect failed: " << render_error(connect.error()) << "\n";
            return 1;
        }

        const auto disconnect_guard = [&]() {
            if (const auto disconnect = broker->disconnect(); disconnect.is_err()) {
                std::cerr << "Disconnect warning: " << render_error(disconnect.error()) << "\n";
            }
        };

        if (cfg.symbols.empty()) {
            disconnect_guard();
            std::cerr << "No live symbols configured for validation\n";
            return 1;
        }
        broker->subscribe_market_data(cfg.symbols);

        const auto symbol = regimeflow::SymbolRegistry::instance().intern(cfg.symbols.front());
        const double quantity = options.quantity.value_or(default_quantity_for(cfg));
        double last_price = 0.0;
        (void)wait_for(*broker, std::chrono::seconds(options.timeout_seconds), [&]() {
            std::lock_guard<std::mutex> lock(state.mutex);
            last_price = state.last_price;
            return last_price > 0.0;
        });

        const auto account_before = broker->get_account_info();
        const auto positions_before = broker->get_positions();
        std::cout << "connected=true broker=" << cfg.broker_type
                  << " symbols=" << cfg.symbols.front()
                  << " account_equity=" << account_before.equity
                  << " positions=" << positions_before.size() << "\n";

        if (options.mode == "submit_cancel_reconcile") {
            const double limit_price = options.limit_price.value_or(last_price > 0.0 ? last_price * 0.95 : 0.0);
            if (limit_price <= 0.0) {
                disconnect_guard();
                std::cerr << "No market price available. Pass --limit-price or enable market data.\n";
                return 1;
            }

            auto order = regimeflow::engine::Order::limit(symbol, regimeflow::engine::OrderSide::Buy, quantity, limit_price);
            order.tif = regimeflow::engine::TimeInForce::GTC;
            const auto submit = broker->submit_order(order);
            if (submit.is_err()) {
                disconnect_guard();
                std::cerr << "Submit failed: " << render_error(submit.error()) << "\n";
                return 1;
            }
            const std::string broker_order_id = submit.value();
            const bool seen_open = wait_for(*broker, std::chrono::seconds(options.timeout_seconds), [&]() {
                const auto open_orders = broker->get_open_orders();
                return std::ranges::any_of(open_orders, [&](const regimeflow::live::ExecutionReport& report) {
                    return report.broker_order_id == broker_order_id;
                });
            });
            if (!seen_open) {
                disconnect_guard();
                std::cerr << "Submitted order was not visible in broker reconciliation\n";
                return 1;
            }
            if (const auto cancel = broker->cancel_order(broker_order_id); cancel.is_err()) {
                disconnect_guard();
                std::cerr << "Cancel failed: " << render_error(cancel.error()) << "\n";
                return 1;
            }
            const bool cleared = wait_for(*broker, std::chrono::seconds(options.timeout_seconds), [&]() {
                const auto open_orders = broker->get_open_orders();
                return std::none_of(open_orders.begin(), open_orders.end(), [&](const auto& report) {
                    return report.broker_order_id == broker_order_id;
                });
            });
            disconnect_guard();
            if (!cleared) {
                std::cerr << "Cancelled order still present during reconciliation\n";
                return 1;
            }
            std::cout << "validation=submit_cancel_reconcile result=ok broker_order_id=" << broker_order_id << "\n";
            return 0;
        }

        if (options.mode == "fill_reconcile") {
            auto buy = regimeflow::engine::Order::market(symbol, regimeflow::engine::OrderSide::Buy, quantity);
            buy.tif = market_tif_for_broker(cfg);
            const auto buy_submit = broker->submit_order(buy);
            if (buy_submit.is_err()) {
                disconnect_guard();
                std::cerr << "Entry submit failed: " << render_error(buy_submit.error()) << "\n";
                return 1;
            }

            const auto start_quantity = [&]() {
                double qty = 0.0;
                for (const auto& position : positions_before) {
                    if (position.symbol == cfg.symbols.front()) {
                        qty += position.quantity;
                    }
                }
                return qty;
            }();

            const bool filled = wait_for(*broker, std::chrono::seconds(options.timeout_seconds), [&]() {
                const auto positions = broker->get_positions();
                double qty = 0.0;
                for (const auto& position : positions) {
                    if (position.symbol == cfg.symbols.front()) {
                        qty += position.quantity;
                    }
                }
                return qty > start_quantity;
            });
            if (!filled) {
                disconnect_guard();
                std::cerr << "Entry fill was not observed through positions reconciliation\n";
                return 1;
            }

            auto sell = regimeflow::engine::Order::market(symbol, regimeflow::engine::OrderSide::Sell, quantity);
            sell.tif = market_tif_for_broker(cfg);
            const auto sell_submit = broker->submit_order(sell);
            if (sell_submit.is_err()) {
                disconnect_guard();
                std::cerr << "Exit submit failed: " << render_error(sell_submit.error()) << "\n";
                return 1;
            }

            const bool flattened = wait_for(*broker, std::chrono::seconds(options.timeout_seconds), [&]() {
                const auto positions = broker->get_positions();
                double qty = 0.0;
                for (const auto& position : positions) {
                    if (position.symbol == cfg.symbols.front()) {
                        qty += position.quantity;
                    }
                }
                return qty <= start_quantity;
            });
            disconnect_guard();
            if (!flattened) {
                std::cerr << "Exit fill was not observed through positions reconciliation\n";
                return 1;
            }
            std::cout << "validation=fill_reconcile result=ok entry=" << buy_submit.value()
                      << " exit=" << sell_submit.value() << "\n";
            return 0;
        }

        disconnect_guard();
        std::cerr << "Unknown validation mode: " << options.mode << "\n";
        return 1;
    }
}  // namespace

int main(int argc, char** argv) {
    ValidationOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--config" && i + 1 < argc) {
            options.config_path = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            options.mode = argv[++i];
        } else if (arg == "--quantity" && i + 1 < argc) {
            options.quantity = std::stod(argv[++i]);
        } else if (arg == "--limit-price" && i + 1 < argc) {
            options.limit_price = std::stod(argv[++i]);
        } else if (arg == "--timeout-seconds" && i + 1 < argc) {
            options.timeout_seconds = std::stoi(argv[++i]);
        } else {
            std::cerr << "Usage: regimeflow_live_validate --config <path> "
                         "[--mode submit_cancel_reconcile|fill_reconcile] "
                         "[--quantity <qty>] [--limit-price <price>] [--timeout-seconds <n>]\n";
            return 1;
        }
    }

    if (options.config_path.empty()) {
        std::cerr << "Missing --config\n";
        return 1;
    }
    return run_validation(options);
}
