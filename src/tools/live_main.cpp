#include "regimeflow/common/config.h"
#include "regimeflow/common/result.h"
#include "regimeflow/common/yaml_config.h"
#include "regimeflow/live/live_engine.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ctime>
#include <map>
#include <thread>
#include <iomanip>

namespace {

std::atomic<bool> g_running{true};

void log_line(const std::string& message);

void handle_signal(int) {
    g_running.store(false);
}

std::optional<std::string> get_string(const regimeflow::Config& cfg, const std::string& key) {
    return cfg.get_as<std::string>(key);
}

std::optional<int64_t> get_int(const regimeflow::Config& cfg, const std::string& key) {
    return cfg.get_as<int64_t>(key);
}

std::optional<bool> get_bool(const regimeflow::Config& cfg, const std::string& key) {
    return cfg.get_as<bool>(key);
}

std::vector<std::string> get_string_array(const regimeflow::Config& cfg,
                                          const std::string& key) {
    std::vector<std::string> out;
    auto arr = cfg.get_as<regimeflow::ConfigValue::Array>(key);
    if (!arr) {
        return out;
    }
    for (const auto& item : *arr) {
        if (const auto* s = item.get_if<std::string>()) {
            out.push_back(*s);
        }
    }
    return out;
}

std::string trim(std::string value) {
    const char* ws = " \t\r\n";
    value.erase(0, value.find_first_not_of(ws));
    value.erase(value.find_last_not_of(ws) + 1);
    return value;
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

regimeflow::Config get_object_config(const regimeflow::Config& cfg, const std::string& key) {
    auto obj = cfg.get_as<regimeflow::ConfigValue::Object>(key);
    if (!obj) {
        return {};
    }
    return regimeflow::Config(*obj);
}

std::map<std::string, std::string> get_object_map(const regimeflow::Config& cfg,
                                                  const std::string& key) {
    std::map<std::string, std::string> out;
    auto obj = cfg.get_as<regimeflow::ConfigValue::Object>(key);
    if (!obj) {
        return out;
    }
    for (const auto& [k, v] : *obj) {
        if (const auto* s = v.get_if<std::string>()) {
            out[k] = *s;
        } else if (const auto* b = v.get_if<bool>()) {
            out[k] = *b ? "true" : "false";
        } else if (const auto* i = v.get_if<int64_t>()) {
            out[k] = std::to_string(*i);
        } else if (const auto* d = v.get_if<double>()) {
            out[k] = std::to_string(*d);
        }
    }
    return out;
}

void set_broker_config_from_env(std::map<std::string, std::string>& broker_config,
                                const std::string& key,
                                const char* env_key) {
    if (broker_config.find(key) != broker_config.end()) {
        return;
    }
    if (const char* val = std::getenv(env_key)) {
        broker_config[key] = val;
    }
}

regimeflow::Result<regimeflow::live::LiveConfig> load_live_config(const std::string& path) {
    using regimeflow::Config;
    using regimeflow::ConfigValue;
    using regimeflow::live::LiveConfig;
    using regimeflow::Result;

    Config root = regimeflow::YamlConfigLoader::load_file(path);

    LiveConfig cfg;
    auto broker = get_string(root, "live.broker");
    if (!broker || broker->empty()) {
        return regimeflow::Error(regimeflow::Error::Code::ConfigError,
                                 "Missing live.broker");
    }
    cfg.broker_type = *broker;
    cfg.symbols = get_string_array(root, "live.symbols");
    cfg.paper_trading = get_bool(root, "live.paper").value_or(true);
    cfg.strategy_name = get_string(root, "strategy.name").value_or("buy_and_hold");
    cfg.strategy_config = get_object_config(root, "strategy.params");
    cfg.risk_config = get_object_config(root, "live.risk");

    if (auto enabled = get_bool(root, "live.reconnect.enabled")) {
        cfg.enable_auto_reconnect = *enabled;
    }
    if (auto ms = get_int(root, "live.reconnect.initial_ms")) {
        cfg.reconnect_initial = regimeflow::Duration::milliseconds(*ms);
    }
    if (auto ms = get_int(root, "live.reconnect.max_ms")) {
        cfg.reconnect_max = regimeflow::Duration::milliseconds(*ms);
    }

    bool heartbeat_enabled = get_bool(root, "live.heartbeat.enabled").value_or(false);
    if (heartbeat_enabled) {
        auto interval_ms = get_int(root, "live.heartbeat.interval_ms").value_or(0);
        if (interval_ms > 0) {
            cfg.heartbeat_timeout = regimeflow::Duration::milliseconds(interval_ms);
        }
    }

    cfg.broker_config = get_object_map(root, "live.broker_config");

    cfg.broker_config["paper"] = cfg.paper_trading ? "true" : "false";

    if (cfg.broker_type == "alpaca") {
        set_broker_config_from_env(cfg.broker_config, "api_key", "ALPACA_API_KEY");
        set_broker_config_from_env(cfg.broker_config, "secret_key", "ALPACA_API_SECRET");
        set_broker_config_from_env(cfg.broker_config, "base_url", "ALPACA_PAPER_BASE_URL");
        if (cfg.broker_config.find("api_key") == cfg.broker_config.end() ||
            cfg.broker_config.find("secret_key") == cfg.broker_config.end()) {
            return regimeflow::Error(regimeflow::Error::Code::ConfigError,
                                     "Missing Alpaca API key/secret");
        }
        if (cfg.broker_config.find("base_url") == cfg.broker_config.end()) {
            return regimeflow::Error(regimeflow::Error::Code::ConfigError,
                                     "Missing Alpaca base URL (ALPACA_PAPER_BASE_URL)");
        }
    }

    return cfg;
}

void log_line(const std::string& message) {
    auto now = std::time(nullptr);
    std::tm tm {};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << message << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    load_dotenv(".env");
    if (argc < 3 || std::string(argv[1]) != "--config") {
        std::cerr << "Usage: regimeflow_live --config <path>\n";
        return 1;
    }

    auto config_res = load_live_config(argv[2]);
    if (config_res.is_err()) {
        std::cerr << "Config error: " << config_res.error().to_string() << "\n";
        return 1;
    }

    regimeflow::live::LiveTradingEngine engine(config_res.value());

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto res = engine.start();
    if (res.is_err()) {
        std::cerr << "Failed to start live engine: " << res.error().to_string() << "\n";
        return 1;
    }

    log_line("Live engine started (connected)");

    engine.on_error([](const std::string& msg) {
        log_line("ERROR: " + msg);
    });

    auto heartbeat_timeout = config_res.value().heartbeat_timeout;
    regimeflow::Timestamp last_heartbeat_log;
    regimeflow::Timestamp last_reconnect_attempt;
    regimeflow::Timestamp last_reconnect_success;

    std::cout << "Press Ctrl+C to stop.\n";
    while (g_running.load() && engine.is_running()) {
        auto health = engine.get_system_health();
        if (health.last_reconnect_attempt.microseconds() != 0 &&
            health.last_reconnect_attempt != last_reconnect_attempt) {
            last_reconnect_attempt = health.last_reconnect_attempt;
            log_line("Reconnect attempt at " +
                     last_reconnect_attempt.to_string("%Y-%m-%d %H:%M:%S"));
        }
        if (health.last_reconnect_success.microseconds() != 0 &&
            health.last_reconnect_success != last_reconnect_success) {
            last_reconnect_success = health.last_reconnect_success;
            log_line("Reconnect success at " +
                     last_reconnect_success.to_string("%Y-%m-%d %H:%M:%S"));
        }
        if (heartbeat_timeout.total_microseconds() > 0 &&
            health.last_market_data.microseconds() != 0) {
            auto since = regimeflow::Timestamp::now() - health.last_market_data;
            bool ok = since.total_microseconds() < heartbeat_timeout.total_microseconds();
            if (last_heartbeat_log.microseconds() == 0 ||
                (regimeflow::Timestamp::now() - last_heartbeat_log).total_seconds() >= 10) {
                last_heartbeat_log = regimeflow::Timestamp::now();
                log_line(std::string("Heartbeat ") + (ok ? "OK" : "STALE"));
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    engine.stop();
    log_line("Live engine stopped (disconnected)");
    return 0;
}
