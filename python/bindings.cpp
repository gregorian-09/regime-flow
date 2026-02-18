#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include <cstddef>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "regimeflow/common/config.h"
#include "regimeflow/common/time.h"
#include "regimeflow/common/yaml_config.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/engine/backtest_results.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/metrics/performance_metrics.h"
#include "regimeflow/metrics/report.h"
#include "regimeflow/metrics/report_writer.h"
#include "regimeflow/plugins/registry.h"
#include "regimeflow/metrics/regime_attribution.h"
#include "regimeflow/data/alpaca_data_client.h"
#include "regimeflow/regime/regime_factory.h"
#include "regimeflow/strategy/strategy_factory.h"
#include "regimeflow/strategy/strategy.h"
#include "regimeflow/walkforward/optimizer.h"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace py = pybind11;

namespace regimeflow::python {

using namespace regimeflow;

struct BarRow {
    int64_t timestamp = 0;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    uint64_t volume = 0;
};

static ConfigValue to_config_value(const py::handle& obj);

static ConfigValue::Array to_array(const py::list& list) {
    ConfigValue::Array arr;
    arr.reserve(list.size());
    for (auto item : list) {
        arr.push_back(to_config_value(item));
    }
    return arr;
}

static ConfigValue::Object to_object(const py::dict& dict) {
    ConfigValue::Object obj;
    for (auto item : dict) {
        auto key = py::cast<std::string>(item.first);
        obj[key] = to_config_value(item.second);
    }
    return obj;
}

static ConfigValue to_config_value(const py::handle& obj) {
    if (py::isinstance<py::bool_>(obj)) {
        return ConfigValue(obj.cast<bool>());
    }
    if (py::isinstance<py::int_>(obj)) {
        return ConfigValue(static_cast<int64_t>(obj.cast<long long>()));
    }
    if (py::isinstance<py::float_>(obj)) {
        return ConfigValue(obj.cast<double>());
    }
    if (py::isinstance<py::str>(obj)) {
        return ConfigValue(obj.cast<std::string>());
    }
    if (py::isinstance<py::list>(obj)) {
        return ConfigValue(to_array(obj.cast<py::list>()));
    }
    if (py::isinstance<py::dict>(obj)) {
        return ConfigValue(to_object(obj.cast<py::dict>()));
    }
    return ConfigValue();
}

static py::object config_value_to_py(const ConfigValue& value);

static py::object object_to_pydict(const ConfigValue::Object& obj) {
    py::dict out;
    for (const auto& [key, val] : obj) {
        out[py::str(key)] = config_value_to_py(val);
    }
    return out;
}

static py::object config_value_to_py(const ConfigValue& value) {
    if (auto v = value.get_if<bool>()) {
        return py::bool_(*v);
    }
    if (auto v = value.get_if<int64_t>()) {
        return py::int_(*v);
    }
    if (auto v = value.get_if<double>()) {
        return py::float_(*v);
    }
    if (auto v = value.get_if<std::string>()) {
        return py::str(*v);
    }
    if (auto v = value.get_if<ConfigValue::Array>()) {
        py::list out;
        for (const auto& item : *v) {
            out.append(config_value_to_py(item));
        }
        return out;
    }
    if (auto v = value.get_if<ConfigValue::Object>()) {
        return object_to_pydict(*v);
    }
    return py::none();
}

static Config config_from_dict(const py::dict& dict) {
    return Config(to_object(dict));
}

static void merge_dict_into_config(Config& cfg, const py::dict& dict, const std::string& prefix) {
    for (auto item : dict) {
        auto key = py::cast<std::string>(item.first);
        auto path = prefix.empty() ? key : prefix + "." + key;
        cfg.set_path(path, to_config_value(item.second));
    }
}

static py::object timestamp_to_datetime(const Timestamp& ts) {
    auto datetime = py::module_::import("datetime");
    auto seconds = ts.seconds();
    auto micros = ts.microseconds() - seconds * 1'000'000;
    auto dt = datetime.attr("datetime").attr("fromtimestamp")(seconds);
    auto delta = datetime.attr("timedelta")(py::arg("microseconds") = micros);
    return dt + delta;
}

static Timestamp timestamp_from_datetime(const py::object& dt) {
    double seconds = dt.attr("timestamp")().cast<double>();
    return Timestamp(static_cast<int64_t>(seconds * 1'000'000.0));
}

static Timestamp parse_date(const std::string& value) {
    if (value.size() <= 10) {
        return Timestamp::from_string(value, "%Y-%m-%d");
    }
    return Timestamp::from_string(value, "%Y-%m-%d %H:%M:%S");
}

static const char* regime_type_name(regime::RegimeType regime_type);

static py::dict performance_stats_to_dict(const metrics::PerformanceStats& stats) {
    py::dict out;
    out["total_return"] = stats.total_return;
    out["cagr"] = stats.cagr;
    out["volatility"] = stats.volatility;
    out["sharpe"] = stats.sharpe;
    out["sortino"] = stats.sortino;
    out["calmar"] = stats.calmar;
    out["max_drawdown"] = stats.max_drawdown;
    out["var_95"] = stats.var_95;
    out["cvar_95"] = stats.cvar_95;
    out["best_return"] = stats.best_return;
    out["worst_return"] = stats.worst_return;
    return out;
}

static py::dict performance_summary_to_dict(const metrics::PerformanceSummary& summary) {
    py::dict out;
    out["total_return"] = summary.total_return;
    out["cagr"] = summary.cagr;
    out["avg_daily_return"] = summary.avg_daily_return;
    out["avg_monthly_return"] = summary.avg_monthly_return;
    out["best_day"] = summary.best_day;
    out["worst_day"] = summary.worst_day;
    out["best_day_date"] = summary.best_day_date.microseconds() == 0
        ? py::none()
        : timestamp_to_datetime(summary.best_day_date);
    out["worst_day_date"] = summary.worst_day_date.microseconds() == 0
        ? py::none()
        : timestamp_to_datetime(summary.worst_day_date);
    out["best_month"] = summary.best_month;
    out["worst_month"] = summary.worst_month;
    out["best_month_date"] = summary.best_month_date.microseconds() == 0
        ? py::none()
        : timestamp_to_datetime(summary.best_month_date);
    out["worst_month_date"] = summary.worst_month_date.microseconds() == 0
        ? py::none()
        : timestamp_to_datetime(summary.worst_month_date);
    out["volatility"] = summary.volatility;
    out["downside_deviation"] = summary.downside_deviation;
    out["max_drawdown"] = summary.max_drawdown;
    out["var_95"] = summary.var_95;
    out["var_99"] = summary.var_99;
    out["cvar_95"] = summary.cvar_95;
    out["sharpe_ratio"] = summary.sharpe_ratio;
    out["sortino_ratio"] = summary.sortino_ratio;
    out["calmar_ratio"] = summary.calmar_ratio;
    out["omega_ratio"] = summary.omega_ratio;
    out["ulcer_index"] = summary.ulcer_index;
    out["information_ratio"] = summary.information_ratio;
    out["treynor_ratio"] = summary.treynor_ratio;
    out["tail_ratio"] = summary.tail_ratio;
    out["total_trades"] = summary.total_trades;
    out["winning_trades"] = summary.winning_trades;
    out["losing_trades"] = summary.losing_trades;
    out["open_trades"] = summary.open_trades;
    out["closed_trades"] = summary.closed_trades;
    out["open_trades_unrealized_pnl"] = summary.open_trades_unrealized_pnl;
    out["open_trades_snapshot_date"] = summary.open_trades_snapshot_date.microseconds() == 0
        ? py::none()
        : timestamp_to_datetime(summary.open_trades_snapshot_date);
    out["win_rate"] = summary.win_rate;
    out["profit_factor"] = summary.profit_factor;
    out["avg_win"] = summary.avg_win;
    out["avg_loss"] = summary.avg_loss;
    out["win_loss_ratio"] = summary.win_loss_ratio;
    out["expectancy"] = summary.expectancy;
    out["avg_trade_duration_days"] = summary.avg_trade_duration_days;
    out["annual_turnover"] = summary.annual_turnover;
    return out;
}

static py::dict regime_performance_to_dict(
    const std::map<regime::RegimeType, metrics::RegimePerformance>& results) {
    py::dict out;
    for (const auto& [regime_type, metrics] : results) {
        py::dict entry;
        entry["return"] = metrics.total_return;
        entry["sharpe"] = metrics.sharpe;
        entry["time_pct"] = metrics.time_pct;
        entry["max_drawdown"] = metrics.max_drawdown;
        entry["observations"] = metrics.observations;
        out[py::str(regime_type_name(regime_type))] = entry;
    }
    return out;
}

static py::dict transition_stats_to_dict(
    const std::map<std::pair<regime::RegimeType, regime::RegimeType>, metrics::TransitionStats>& stats) {
    py::dict out;
    for (const auto& [key, value] : stats) {
        const auto& from = key.first;
        const auto& to = key.second;
        std::string label = std::string(regime_type_name(from)) + "->" + regime_type_name(to);
        py::dict entry;
        entry["observations"] = value.observations;
        entry["avg_return"] = value.avg_return;
        entry["volatility"] = value.volatility;
        out[py::str(label)] = entry;
    }
    return out;
}

static std::string symbol_to_string(SymbolId symbol) {
    return SymbolRegistry::instance().lookup(symbol);
}

static SymbolId symbol_from_string(const std::string& symbol) {
    return SymbolRegistry::instance().intern(symbol);
}

static data::BarType bar_type_from_string(const std::string& bar_type) {
    if (bar_type == "1m") return data::BarType::Time_1Min;
    if (bar_type == "5m") return data::BarType::Time_5Min;
    if (bar_type == "15m") return data::BarType::Time_15Min;
    if (bar_type == "30m") return data::BarType::Time_30Min;
    if (bar_type == "1h") return data::BarType::Time_1Hour;
    if (bar_type == "4h") return data::BarType::Time_4Hour;
    if (bar_type == "1d") return data::BarType::Time_1Day;
    return data::BarType::Time_1Day;
}

static walkforward::WalkForwardConfig::WindowType window_type_from_string(
    const std::string& value) {
    if (value == "anchored") {
        return walkforward::WalkForwardConfig::WindowType::Anchored;
    }
    if (value == "regime") {
        return walkforward::WalkForwardConfig::WindowType::RegimeAware;
    }
    return walkforward::WalkForwardConfig::WindowType::Rolling;
}

static walkforward::WalkForwardConfig::OptMethod opt_method_from_string(
    const std::string& value) {
    if (value == "random") {
        return walkforward::WalkForwardConfig::OptMethod::Random;
    }
    if (value == "bayesian") {
        return walkforward::WalkForwardConfig::OptMethod::Bayesian;
    }
    return walkforward::WalkForwardConfig::OptMethod::Grid;
}

static walkforward::ParameterDef::Type param_type_from_string(const std::string& value) {
    if (value == "int") return walkforward::ParameterDef::Type::Int;
    if (value == "categorical") return walkforward::ParameterDef::Type::Categorical;
    return walkforward::ParameterDef::Type::Double;
}

static walkforward::ParameterDef::Distribution dist_from_string(const std::string& value) {
    if (value == "loguniform") return walkforward::ParameterDef::Distribution::LogUniform;
    if (value == "normal") return walkforward::ParameterDef::Distribution::Normal;
    return walkforward::ParameterDef::Distribution::Uniform;
}

static walkforward::WalkForwardConfig walkforward_config_from_kwargs(const py::kwargs& kwargs) {
    walkforward::WalkForwardConfig cfg;
    for (auto item : kwargs) {
        auto key = py::cast<std::string>(item.first);
        if (key == "window_type") {
            cfg.window_type = window_type_from_string(py::cast<std::string>(item.second));
        } else if (key == "in_sample_months") {
            cfg.in_sample_period = Duration::months(py::cast<int>(item.second));
        } else if (key == "out_of_sample_months") {
            cfg.out_of_sample_period = Duration::months(py::cast<int>(item.second));
        } else if (key == "step_months") {
            cfg.step_size = Duration::months(py::cast<int>(item.second));
        } else if (key == "in_sample_days") {
            cfg.in_sample_period = Duration::days(py::cast<int>(item.second));
        } else if (key == "out_of_sample_days") {
            cfg.out_of_sample_period = Duration::days(py::cast<int>(item.second));
        } else if (key == "step_days") {
            cfg.step_size = Duration::days(py::cast<int>(item.second));
        } else if (key == "optimization_method") {
            cfg.optimization_method = opt_method_from_string(py::cast<std::string>(item.second));
        } else if (key == "max_trials") {
            cfg.max_trials = py::cast<int>(item.second);
        } else if (key == "fitness_metric") {
            cfg.fitness_metric = py::cast<std::string>(item.second);
        } else if (key == "maximize") {
            cfg.maximize = py::cast<bool>(item.second);
        } else if (key == "retrain_regime_each_window") {
            cfg.retrain_regime_each_window = py::cast<bool>(item.second);
        } else if (key == "optimize_per_regime") {
            cfg.optimize_per_regime = py::cast<bool>(item.second);
        } else if (key == "disable_default_regime_training") {
            cfg.disable_default_regime_training = py::cast<bool>(item.second);
        } else if (key == "num_parallel_backtests") {
            cfg.num_parallel_backtests = py::cast<int>(item.second);
        } else if (key == "enable_overfitting_detection") {
            cfg.enable_overfitting_detection = py::cast<bool>(item.second);
        } else if (key == "max_is_oos_ratio") {
            cfg.max_is_oos_ratio = py::cast<double>(item.second);
        } else if (key == "initial_capital") {
            cfg.initial_capital = py::cast<double>(item.second);
        } else if (key == "bar_type") {
            cfg.bar_type = bar_type_from_string(py::cast<std::string>(item.second));
        } else if (key == "periods_per_year") {
            cfg.periods_per_year = py::cast<double>(item.second);
        }
    }
    return cfg;
}

static walkforward::ParameterDef parameter_def_from_kwargs(const py::kwargs& kwargs) {
    walkforward::ParameterDef def;
    for (auto item : kwargs) {
        auto key = py::cast<std::string>(item.first);
        if (key == "name") {
            def.name = py::cast<std::string>(item.second);
        } else if (key == "type") {
            def.type = param_type_from_string(py::cast<std::string>(item.second));
        } else if (key == "min_value") {
            def.min_value = py::cast<double>(item.second);
        } else if (key == "max_value") {
            def.max_value = py::cast<double>(item.second);
        } else if (key == "step") {
            def.step = py::cast<double>(item.second);
        } else if (key == "categories") {
            def.categories.clear();
            for (auto v : py::cast<py::list>(item.second)) {
                if (py::isinstance<py::str>(v)) {
                    def.categories.emplace_back(py::cast<std::string>(v));
                } else if (py::isinstance<py::int_>(v)) {
                    def.categories.emplace_back(py::cast<int>(v));
                } else if (py::isinstance<py::float_>(v)) {
                    def.categories.emplace_back(py::cast<double>(v));
                }
            }
        } else if (key == "distribution") {
            def.distribution = dist_from_string(py::cast<std::string>(item.second));
        }
    }
    return def;
}

static Timestamp parse_date_object(const py::object& value) {
    if (py::isinstance<py::str>(value)) {
        return parse_date(py::cast<std::string>(value));
    }
    if (py::hasattr(value, "timestamp")) {
        return timestamp_from_datetime(value);
    }
    if (py::isinstance<Timestamp>(value)) {
        return value.cast<Timestamp>();
    }
    return Timestamp{};
}

static const char* regime_type_name(regime::RegimeType regime_type) {
    switch (regime_type) {
        case regime::RegimeType::Bull:
            return "BULL";
        case regime::RegimeType::Neutral:
            return "NEUTRAL";
        case regime::RegimeType::Bear:
            return "BEAR";
        case regime::RegimeType::Crisis:
            return "CRISIS";
        case regime::RegimeType::Custom:
            return "CUSTOM";
    }
    return "UNKNOWN";
}

struct BacktestConfig {
    std::string data_source = "csv";
    py::dict data_config;
    std::vector<std::string> symbols;
    std::string start_date;
    std::string end_date;
    std::string bar_type = "1d";
    double initial_capital = 1'000'000.0;
    std::string currency = "USD";
    std::string regime_detector = "hmm";
    py::dict regime_params;
    std::vector<std::string> plugins_search_paths;
    std::vector<std::string> plugins_load;
    std::string slippage_model = "zero";
    py::dict slippage_params;
    std::string commission_model = "zero";
    py::dict commission_params;
    py::dict risk_params;
    py::dict strategy_params;

    static BacktestConfig from_dict(const py::dict& dict) {
        BacktestConfig cfg;
        if (dict.contains("data_source")) cfg.data_source = dict["data_source"].cast<std::string>();
        if (dict.contains("data_config")) cfg.data_config = dict["data_config"].cast<py::dict>();
        if (dict.contains("symbols")) cfg.symbols = dict["symbols"].cast<std::vector<std::string>>();
        if (dict.contains("start_date")) cfg.start_date = dict["start_date"].cast<std::string>();
        if (dict.contains("end_date")) cfg.end_date = dict["end_date"].cast<std::string>();
        if (dict.contains("bar_type")) cfg.bar_type = dict["bar_type"].cast<std::string>();
        if (dict.contains("initial_capital")) cfg.initial_capital = dict["initial_capital"].cast<double>();
        if (dict.contains("currency")) cfg.currency = dict["currency"].cast<std::string>();
        if (dict.contains("regime_detector")) cfg.regime_detector = dict["regime_detector"].cast<std::string>();
        if (dict.contains("regime_params")) cfg.regime_params = dict["regime_params"].cast<py::dict>();
        if (dict.contains("plugins_search_paths")) {
            cfg.plugins_search_paths = dict["plugins_search_paths"].cast<std::vector<std::string>>();
        } else if (dict.contains("plugins")) {
            auto plugins = dict["plugins"].cast<py::dict>();
            if (plugins.contains("search_paths")) {
                cfg.plugins_search_paths = plugins["search_paths"].cast<std::vector<std::string>>();
            }
        }
        if (dict.contains("plugins_load")) {
            cfg.plugins_load = dict["plugins_load"].cast<std::vector<std::string>>();
        } else if (dict.contains("plugins")) {
            auto plugins = dict["plugins"].cast<py::dict>();
            if (plugins.contains("load")) {
                cfg.plugins_load = plugins["load"].cast<std::vector<std::string>>();
            }
        }
        if (dict.contains("slippage_model")) cfg.slippage_model = dict["slippage_model"].cast<std::string>();
        if (dict.contains("slippage_params")) cfg.slippage_params = dict["slippage_params"].cast<py::dict>();
        if (dict.contains("commission_model")) cfg.commission_model = dict["commission_model"].cast<std::string>();
        if (dict.contains("commission_params")) cfg.commission_params = dict["commission_params"].cast<py::dict>();
        if (dict.contains("risk_params")) cfg.risk_params = dict["risk_params"].cast<py::dict>();
        if (dict.contains("strategy_params")) cfg.strategy_params = dict["strategy_params"].cast<py::dict>();
        return cfg;
    }

    static BacktestConfig from_yaml_file(const std::string& path) {
        BacktestConfig cfg;
        auto config = YamlConfigLoader::load_file(path);
        if (auto v = config.get_as<std::string>("data_source")) cfg.data_source = *v;
        if (auto v = config.get_as<ConfigValue::Object>("data")) {
            cfg.data_config = object_to_pydict(*v).cast<py::dict>();
        }
        if (auto v = config.get_as<ConfigValue::Array>("symbols")) {
            cfg.symbols.clear();
            for (const auto& item : *v) {
                if (const auto* s = item.get_if<std::string>()) {
                    cfg.symbols.push_back(*s);
                }
            }
        }
        if (auto v = config.get_as<std::string>("start_date")) cfg.start_date = *v;
        if (auto v = config.get_as<std::string>("end_date")) cfg.end_date = *v;
        if (auto v = config.get_as<std::string>("bar_type")) cfg.bar_type = *v;
        if (auto v = config.get_as<double>("initial_capital")) cfg.initial_capital = *v;
        if (auto v = config.get_as<std::string>("currency")) cfg.currency = *v;
        if (auto v = config.get_as<std::string>("regime_detector")) cfg.regime_detector = *v;
        if (auto v = config.get_as<ConfigValue::Object>("regime_params")) {
            cfg.regime_params = object_to_pydict(*v).cast<py::dict>();
        }
        if (auto v = config.get_as<ConfigValue::Array>("plugins_search_paths")) {
            cfg.plugins_search_paths.clear();
            for (const auto& item : *v) {
                if (const auto* s = item.get_if<std::string>()) {
                    cfg.plugins_search_paths.push_back(*s);
                }
            }
        }
        if (auto v = config.get_as<ConfigValue::Array>("plugins.search_paths")) {
            cfg.plugins_search_paths.clear();
            for (const auto& item : *v) {
                if (const auto* s = item.get_if<std::string>()) {
                    cfg.plugins_search_paths.push_back(*s);
                }
            }
        }
        if (auto v = config.get_as<ConfigValue::Array>("plugins_load")) {
            cfg.plugins_load.clear();
            for (const auto& item : *v) {
                if (const auto* s = item.get_if<std::string>()) {
                    cfg.plugins_load.push_back(*s);
                }
            }
        }
        if (auto v = config.get_as<ConfigValue::Array>("plugins.load")) {
            cfg.plugins_load.clear();
            for (const auto& item : *v) {
                if (const auto* s = item.get_if<std::string>()) {
                    cfg.plugins_load.push_back(*s);
                }
            }
        }
        if (auto v = config.get_as<std::string>("slippage_model")) cfg.slippage_model = *v;
        if (auto v = config.get_as<ConfigValue::Object>("slippage_params")) {
            cfg.slippage_params = object_to_pydict(*v).cast<py::dict>();
        }
        if (auto v = config.get_as<std::string>("commission_model")) cfg.commission_model = *v;
        if (auto v = config.get_as<ConfigValue::Object>("commission_params")) {
            cfg.commission_params = object_to_pydict(*v).cast<py::dict>();
        }
        if (auto v = config.get_as<ConfigValue::Object>("risk_params")) {
            cfg.risk_params = object_to_pydict(*v).cast<py::dict>();
        }
        if (auto v = config.get_as<ConfigValue::Object>("strategy_params")) {
            cfg.strategy_params = object_to_pydict(*v).cast<py::dict>();
        }
        return cfg;
    }
};

class PythonStrategyAdapter : public strategy::Strategy {
public:
    explicit PythonStrategyAdapter(py::object strategy) : strategy_(std::move(strategy)) {}

    void initialize(strategy::StrategyContext& ctx) override {
        ctx_ = &ctx;
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "initialize")) {
            strategy_.attr("initialize")(ctx);
        }
    }

    void on_start() override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_start")) {
            strategy_.attr("on_start")();
        }
    }

    void on_stop() override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_stop")) {
            strategy_.attr("on_stop")();
        }
    }

    void on_bar(const data::Bar& bar) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_bar")) {
            strategy_.attr("on_bar")(bar);
        }
    }

    void on_tick(const data::Tick& tick) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_tick")) {
            strategy_.attr("on_tick")(tick);
        }
    }

    void on_quote(const data::Quote& quote) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_quote")) {
            strategy_.attr("on_quote")(quote);
        }
    }

    void on_order_book(const data::OrderBook& book) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_order_book")) {
            strategy_.attr("on_order_book")(book);
        }
    }

    void on_order_update(const engine::Order& order) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_order_update")) {
            strategy_.attr("on_order_update")(order);
        }
    }

    void on_fill(const engine::Fill& fill) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_fill")) {
            strategy_.attr("on_fill")(fill);
        }
    }

    void on_regime_change(const regime::RegimeTransition& transition) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_regime_change")) {
            strategy_.attr("on_regime_change")(transition);
        }
    }

    void on_end_of_day(const Timestamp& date) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_end_of_day")) {
            strategy_.attr("on_end_of_day")(date);
        }
    }

    void on_timer(const std::string& timer_id) override {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(strategy_, "on_timer")) {
            strategy_.attr("on_timer")(timer_id);
        }
    }

private:
    py::object strategy_;
};

class PyStrategyTrampoline : public strategy::Strategy {
public:
    using strategy::Strategy::Strategy;

    void initialize(strategy::StrategyContext& ctx) override {
        PYBIND11_OVERRIDE_PURE(void, strategy::Strategy, initialize, ctx);
    }

    void on_start() override { PYBIND11_OVERRIDE(void, strategy::Strategy, on_start); }
    void on_stop() override { PYBIND11_OVERRIDE(void, strategy::Strategy, on_stop); }
    void on_bar(const data::Bar& bar) override { PYBIND11_OVERRIDE(void, strategy::Strategy, on_bar, bar); }
    void on_tick(const data::Tick& tick) override { PYBIND11_OVERRIDE(void, strategy::Strategy, on_tick, tick); }
    void on_quote(const data::Quote& quote) override { PYBIND11_OVERRIDE(void, strategy::Strategy, on_quote, quote); }
    void on_order_book(const data::OrderBook& book) override {
        PYBIND11_OVERRIDE(void, strategy::Strategy, on_order_book, book);
    }
    void on_order_update(const engine::Order& order) override {
        PYBIND11_OVERRIDE(void, strategy::Strategy, on_order_update, order);
    }
    void on_fill(const engine::Fill& fill) override { PYBIND11_OVERRIDE(void, strategy::Strategy, on_fill, fill); }
    void on_regime_change(const regime::RegimeTransition& transition) override {
        PYBIND11_OVERRIDE(void, strategy::Strategy, on_regime_change, transition);
    }
    void on_end_of_day(const Timestamp& date) override {
        PYBIND11_OVERRIDE(void, strategy::Strategy, on_end_of_day, date);
    }
    void on_timer(const std::string& timer_id) override {
        PYBIND11_OVERRIDE(void, strategy::Strategy, on_timer, timer_id);
    }
};

class PyBacktestEngine {
public:
    explicit PyBacktestEngine(const BacktestConfig& config)
        : config_(config),
          bar_type_(bar_type_from_string(config.bar_type)),
          data_config_(config_from_dict(config.data_config)),
          execution_config_(),
          risk_config_(config_from_dict(config.risk_params)),
          regime_config_(),
          strategy_config_(config_from_dict(config.strategy_params)),
          plugins_search_paths_(config.plugins_search_paths),
          plugins_load_(config.plugins_load) {
        data_config_.set("type", config.data_source);

        execution_config_.set_path("slippage.type", config.slippage_model);
        merge_dict_into_config(execution_config_, config.slippage_params, "slippage");
        execution_config_.set_path("slippage.params", to_config_value(config.slippage_params));
        execution_config_.set_path("commission.type", config.commission_model);
        merge_dict_into_config(execution_config_, config.commission_params, "commission");
        execution_config_.set_path("commission.params", to_config_value(config.commission_params));

        regime_config_.set("detector", config.regime_detector);
        merge_dict_into_config(regime_config_, config.regime_params, "");
        regime_config_.set("params", to_config_value(config.regime_params));

        TimeRange range;
        if (!config.start_date.empty()) {
            range.start = Timestamp::from_string(config.start_date, "%Y-%m-%d");
        }
        if (!config.end_date.empty()) {
            range.end = Timestamp::from_string(config.end_date, "%Y-%m-%d");
        }
        range_ = range;

        parallel_context_.data_config = data_config_;
        parallel_context_.range = range_;
        parallel_context_.symbols = config.symbols;
        parallel_context_.bar_type = bar_type_;
    }

    engine::BacktestResults run(py::object strategy_obj) {
        auto engine = create_engine();
        ensure_data_source();
        if (!data_source_) {
            throw std::runtime_error("Failed to create data source");
        }
        std::vector<SymbolId> symbol_ids;
        symbol_ids.reserve(config_.symbols.size());
        for (const auto& sym : config_.symbols) {
            symbol_ids.push_back(symbol_from_string(sym));
        }
        auto bar_it = data_source_->create_iterator(symbol_ids, range_, bar_type_);
        auto tick_it = data_source_->create_tick_iterator(symbol_ids, range_);
        auto book_it = data_source_->create_book_iterator(symbol_ids, range_);
        engine->load_data(std::move(bar_it), std::move(tick_it), std::move(book_it));

        std::unique_ptr<strategy::Strategy> strategy;
        if (py::isinstance<py::str>(strategy_obj)) {
            auto name = strategy_obj.cast<std::string>();
            Config cfg = strategy_config_;
            cfg.set("name", name);
            strategy = strategy::StrategyFactory::instance().create(cfg);
            if (!strategy) {
                throw std::runtime_error("Failed to create strategy: " + name);
            }
        } else {
            strategy = std::make_unique<PythonStrategyAdapter>(strategy_obj);
        }

        engine->set_strategy(std::move(strategy), strategy_config_);
        py::gil_scoped_release release;
        engine->run();
        engine_ = std::move(engine);
        return engine_->results();
    }

    std::vector<engine::BacktestResults> run_parallel(const std::vector<py::dict>& param_sets,
                                                      py::object strategy_factory,
                                                      int num_threads) {
        if (!strategy_factory) {
            throw std::runtime_error("Strategy factory not provided");
        }
        auto engine = create_engine();
        engine->set_parallel_context(parallel_context_);

        std::vector<std::map<std::string, double>> params;
        params.reserve(param_sets.size());
        for (const auto& param_set : param_sets) {
            std::map<std::string, double> out;
            for (auto item : param_set) {
                auto key = py::cast<std::string>(item.first);
                auto value = py::cast<double>(item.second);
                out[key] = value;
            }
            params.push_back(std::move(out));
        }

        auto factory = [strategy_factory](const std::map<std::string, double>& set) {
            py::gil_scoped_acquire acquire;
            py::dict args;
            for (const auto& [key, value] : set) {
                args[py::str(key)] = value;
            }
            py::object strat = strategy_factory(args);
            return std::unique_ptr<strategy::Strategy>(
                new PythonStrategyAdapter(strat));
        };

        py::gil_scoped_release release;
        auto results = engine->run_parallel(params, std::move(factory), num_threads);
        return results;
    }

    const engine::Portfolio& portfolio() const {
        if (!engine_) {
            throw std::runtime_error("Backtest has not been run");
        }
        return engine_->portfolio();
    }

    const regime::RegimeState& current_regime() const {
        if (!engine_) {
            throw std::runtime_error("Backtest has not been run");
        }
        return engine_->current_regime();
    }

    py::array get_close_prices(const std::string& symbol,
                               const std::optional<std::string>& start,
                               const std::optional<std::string>& end) {
        ensure_data_source();
        if (!data_source_) {
            throw std::runtime_error("Data source not available");
        }
        TimeRange range = range_;
        if (start) {
            range.start = parse_date(*start);
        }
        if (end) {
            range.end = parse_date(*end);
        }
        auto bars = data_source_->get_bars(symbol_from_string(symbol), range, bar_type_);
        auto data = std::make_shared<std::vector<double>>();
        data->reserve(bars.size());
        for (const auto& bar : bars) {
            data->push_back(bar.close);
        }
        auto capsule = py::capsule(new std::shared_ptr<std::vector<double>>(data),
                                   [](void* p) {
                                       delete reinterpret_cast<std::shared_ptr<std::vector<double>>*>(p);
                                   });
        return py::array_t<double>(data->size(), data->data(), capsule);
    }

    py::array get_bars_array(const std::string& symbol,
                             const std::optional<std::string>& start,
                             const std::optional<std::string>& end) {
        ensure_data_source();
        if (!data_source_) {
            throw std::runtime_error("Data source not available");
        }
        TimeRange range = range_;
        if (start) {
            range.start = parse_date(*start);
        }
        if (end) {
            range.end = parse_date(*end);
        }
        auto bars = data_source_->get_bars(symbol_from_string(symbol), range, bar_type_);
        auto data = std::make_shared<std::vector<BarRow>>();
        data->reserve(bars.size());
        for (const auto& bar : bars) {
            data->push_back(BarRow{
                bar.timestamp.microseconds(),
                bar.open,
                bar.high,
                bar.low,
                bar.close,
                static_cast<uint64_t>(bar.volume),
            });
        }
        auto capsule = py::capsule(new std::shared_ptr<std::vector<BarRow>>(data),
                                   [](void* p) {
                                       delete reinterpret_cast<std::shared_ptr<std::vector<BarRow>>*>(p);
                                   });
        py::array array(py::dtype::of<BarRow>(),
                        {static_cast<ssize_t>(data->size())},
                        {static_cast<ssize_t>(sizeof(BarRow))},
                        data->data(),
                        capsule);
        return array;
    }

private:
    std::unique_ptr<engine::BacktestEngine> create_engine() const {
        configure_plugins();
        auto engine = std::make_unique<engine::BacktestEngine>(config_.initial_capital,
                                                               config_.currency);
        engine->configure_execution(execution_config_);
        engine->configure_risk(risk_config_);
        engine->configure_regime(regime_config_);
        return engine;
    }

    void configure_plugins() const {
        auto& registry = plugins::PluginRegistry::instance();
        for (const auto& path : plugins_search_paths_) {
            registry.scan_plugin_directory(path);
        }
        for (const auto& path : plugins_load_) {
            registry.load_dynamic_plugin(path);
        }
    }

    void ensure_data_source() {
        if (!data_source_) {
            data_source_ = data::DataSourceFactory::create(data_config_);
        }
    }

    BacktestConfig config_;
    data::BarType bar_type_;
    Config data_config_;
    Config execution_config_;
    Config risk_config_;
    Config regime_config_;
    Config strategy_config_;
    TimeRange range_;
    engine::ParallelContext parallel_context_;
    std::vector<std::string> plugins_search_paths_;
    std::vector<std::string> plugins_load_;
    std::unique_ptr<engine::BacktestEngine> engine_;
    std::unique_ptr<data::DataSource> data_source_;
};

class PyWalkForwardOptimizer {
public:
    explicit PyWalkForwardOptimizer(const walkforward::WalkForwardConfig& config)
        : config_(config) {}

    walkforward::WalkForwardResults optimize(const py::list& params,
                                             py::object strategy_factory,
                                             py::object data_source_config,
                                             const py::tuple& date_range,
                                             py::object detector_config = py::none()) {
        if (!strategy_factory) {
            throw std::runtime_error("Strategy factory not provided");
        }
        if (!data_source_config) {
            throw std::runtime_error("Data source config not provided");
        }

        std::vector<walkforward::ParameterDef> param_defs;
        param_defs.reserve(params.size());
        for (auto item : params) {
            if (py::isinstance<walkforward::ParameterDef>(item)) {
                param_defs.push_back(item.cast<walkforward::ParameterDef>());
            } else if (py::isinstance<py::dict>(item)) {
                py::dict dict = py::reinterpret_borrow<py::dict>(item);
                py::kwargs kwargs(dict);
                param_defs.push_back(parameter_def_from_kwargs(kwargs));
            } else {
                throw std::runtime_error("Invalid parameter definition");
            }
        }

        Config data_cfg;
        if (py::isinstance<Config>(data_source_config)) {
            data_cfg = data_source_config.cast<Config>();
        } else if (py::isinstance<py::dict>(data_source_config)) {
            data_cfg = config_from_dict(data_source_config.cast<py::dict>());
        } else {
            throw std::runtime_error("data_source_config must be a dict or Config");
        }
        if (!data_cfg.get("type")) {
            throw std::runtime_error("data_source_config requires 'type'");
        }

        auto data_source = data::DataSourceFactory::create(data_cfg);
        if (!data_source) {
            throw std::runtime_error("Failed to create data source");
        }

        if (date_range.size() != 2) {
            throw std::runtime_error("date_range must be (start, end)");
        }
        TimeRange range;
        range.start = parse_date_object(date_range[0]);
        range.end = parse_date_object(date_range[1]);

        std::function<std::unique_ptr<regime::RegimeDetector>()> detector_factory;
        if (!detector_config.is_none()) {
            Config det_cfg;
            if (py::isinstance<Config>(detector_config)) {
                det_cfg = detector_config.cast<Config>();
            } else if (py::isinstance<py::dict>(detector_config)) {
                det_cfg = config_from_dict(detector_config.cast<py::dict>());
            } else {
                throw std::runtime_error("detector_config must be dict or Config");
            }
            detector_factory = [det_cfg]() mutable {
                return regime::RegimeFactory::create_detector(det_cfg);
            };
        }

        auto factory = [strategy_factory](const walkforward::ParameterSet& set) {
            py::gil_scoped_acquire acquire;
            py::dict args;
            for (const auto& [key, value] : set) {
                if (auto* v = std::get_if<int>(&value)) {
                    args[py::str(key)] = *v;
                } else if (auto* v2 = std::get_if<double>(&value)) {
                    args[py::str(key)] = *v2;
                } else if (auto* v3 = std::get_if<std::string>(&value)) {
                    args[py::str(key)] = *v3;
                }
            }
            py::object strat = strategy_factory(args);
            return std::unique_ptr<strategy::Strategy>(
                new PythonStrategyAdapter(strat));
        };

        walkforward::WalkForwardOptimizer optimizer(config_);
        py::gil_scoped_release release;
        return optimizer.optimize(param_defs, std::move(factory), data_source.get(),
                                  range, detector_factory);
    }

private:
    walkforward::WalkForwardConfig config_;
};

static py::object equity_curve_dataframe(const metrics::EquityCurve& curve) {
    py::list timestamps;
    py::list equities;
    const auto& ts = curve.timestamps();
    const auto& vals = curve.equities();
    for (size_t i = 0; i < ts.size(); ++i) {
        timestamps.append(timestamp_to_datetime(ts[i]));
        equities.append(vals[i]);
    }
    auto pandas = py::module_::import("pandas");
    auto df = pandas.attr("DataFrame")(py::dict(
        py::arg("timestamp") = timestamps,
        py::arg("equity") = equities));
    return df.attr("set_index")("timestamp");
}

static py::object portfolio_equity_dataframe(const std::vector<engine::PortfolioSnapshot>& snapshots) {
    py::list timestamps;
    py::list cash;
    py::list equity;
    py::list gross;
    py::list net;
    py::list leverage;
    for (const auto& snap : snapshots) {
        timestamps.append(timestamp_to_datetime(snap.timestamp));
        cash.append(snap.cash);
        equity.append(snap.equity);
        gross.append(snap.gross_exposure);
        net.append(snap.net_exposure);
        leverage.append(snap.leverage);
    }
    auto pandas = py::module_::import("pandas");
    auto df = pandas.attr("DataFrame")(py::dict(
        py::arg("timestamp") = timestamps,
        py::arg("cash") = cash,
        py::arg("equity") = equity,
        py::arg("gross_exposure") = gross,
        py::arg("net_exposure") = net,
        py::arg("leverage") = leverage));
    return df.attr("set_index")("timestamp");
}

static py::object fills_dataframe(const std::vector<engine::Fill>& fills) {
    py::list timestamps;
    py::list symbols;
    py::list quantities;
    py::list prices;
    py::list commissions;
    py::list slippages;
    py::list order_ids;
    py::list fill_ids;
    py::list is_maker;
    for (const auto& fill : fills) {
        timestamps.append(timestamp_to_datetime(fill.timestamp));
        symbols.append(symbol_to_string(fill.symbol));
        quantities.append(fill.quantity);
        prices.append(fill.price);
        commissions.append(fill.commission);
        slippages.append(fill.slippage);
        order_ids.append(fill.order_id);
        fill_ids.append(fill.id);
        is_maker.append(fill.is_maker);
    }
    auto pandas = py::module_::import("pandas");
    auto df = pandas.attr("DataFrame")(py::dict(
        py::arg("timestamp") = timestamps,
        py::arg("symbol") = symbols,
        py::arg("quantity") = quantities,
        py::arg("price") = prices,
        py::arg("commission") = commissions,
        py::arg("slippage") = slippages,
        py::arg("order_id") = order_ids,
        py::arg("fill_id") = fill_ids,
        py::arg("is_maker") = is_maker));
    return df;
}

static double win_rate_from_equity(const metrics::EquityCurve& curve) {
    const auto& equities = curve.equities();
    if (equities.size() < 2) {
        return 0.0;
    }
    size_t wins = 0;
    size_t total = 0;
    for (size_t i = 1; i < equities.size(); ++i) {
        double ret = equities[i] - equities[i - 1];
        if (ret > 0) {
            ++wins;
        }
        ++total;
    }
    return total > 0 ? static_cast<double>(wins) / static_cast<double>(total) : 0.0;
}

static double profit_factor_from_equity(const metrics::EquityCurve& curve) {
    const auto& equities = curve.equities();
    if (equities.size() < 2) {
        return 0.0;
    }
    double gains = 0.0;
    double losses = 0.0;
    for (size_t i = 1; i < equities.size(); ++i) {
        double ret = equities[i] - equities[i - 1];
        if (ret >= 0) {
            gains += ret;
        } else {
            losses += -ret;
        }
    }
    return losses > 0 ? gains / losses : 0.0;
}

}  // namespace regimeflow::python

PYBIND11_MODULE(_core, m) {
    using namespace regimeflow;
    using namespace regimeflow::python;

    m.doc() = "RegimeFlow - Regime-adaptive backtesting framework";

    SymbolRegistry::instance().intern("");

    auto m_data = m.def_submodule("data", "Data handling");
    auto m_regime = m.def_submodule("regime", "Regime detection");
    auto m_engine = m.def_submodule("engine", "Backtesting engine");
    auto m_strategy = m.def_submodule("strategy", "Strategy definitions");
    auto m_metrics = m.def_submodule("metrics", "Performance metrics");
    auto m_risk = m.def_submodule("risk", "Risk management");
    auto m_walkforward = m.def_submodule("walkforward", "Walk-forward optimization");

    py::class_<Timestamp>(m, "Timestamp")
        .def(py::init<int64_t>())
        .def_property_readonly("value", &Timestamp::microseconds)
        .def("to_datetime", &timestamp_to_datetime)
        .def_static("from_datetime", &timestamp_from_datetime)
        .def("to_string", &Timestamp::to_string);

    py::class_<Config>(m, "Config")
        .def(py::init<>())
        .def(py::init([](const py::dict& dict) { return config_from_dict(dict); }),
             py::arg("data"))
        .def("has", &Config::has)
        .def("get", [](const Config& cfg, const std::string& key) -> py::object {
            const auto* value = cfg.get(key);
            if (!value) {
                return py::none();
            }
            return config_value_to_py(*value);
        })
        .def("get_path", [](const Config& cfg, const std::string& path) -> py::object {
            const auto* value = cfg.get_path(path);
            if (!value) {
                return py::none();
            }
            return config_value_to_py(*value);
        })
        .def("set", [](Config& cfg, const std::string& key, const py::object& value) {
            cfg.set(key, to_config_value(value));
        })
        .def("set_path", [](Config& cfg, const std::string& path, const py::object& value) {
            cfg.set_path(path, to_config_value(value));
        });

    m.def("load_config", [](const std::string& path) {
        return YamlConfigLoader::load_file(path);
    });

    py::enum_<engine::OrderSide>(m, "OrderSide")
        .value("BUY", engine::OrderSide::Buy)
        .value("SELL", engine::OrderSide::Sell);

    py::enum_<engine::OrderType>(m, "OrderType")
        .value("MARKET", engine::OrderType::Market)
        .value("LIMIT", engine::OrderType::Limit)
        .value("STOP", engine::OrderType::Stop)
        .value("STOP_LIMIT", engine::OrderType::StopLimit)
        .value("MARKET_ON_CLOSE", engine::OrderType::MarketOnClose)
        .value("MARKET_ON_OPEN", engine::OrderType::MarketOnOpen);

    py::enum_<engine::OrderStatus>(m, "OrderStatus")
        .value("CREATED", engine::OrderStatus::Created)
        .value("PENDING", engine::OrderStatus::Pending)
        .value("PARTIALLY_FILLED", engine::OrderStatus::PartiallyFilled)
        .value("FILLED", engine::OrderStatus::Filled)
        .value("CANCELLED", engine::OrderStatus::Cancelled)
        .value("REJECTED", engine::OrderStatus::Rejected)
        .value("INVALID", engine::OrderStatus::Invalid);

    py::enum_<engine::TimeInForce>(m, "TimeInForce")
        .value("DAY", engine::TimeInForce::Day)
        .value("GTC", engine::TimeInForce::GTC)
        .value("IOC", engine::TimeInForce::IOC)
        .value("FOK", engine::TimeInForce::FOK)
        .value("GTD", engine::TimeInForce::GTD);

    py::enum_<regime::RegimeType>(m, "RegimeType")
        .value("BULL", regime::RegimeType::Bull)
        .value("NEUTRAL", regime::RegimeType::Neutral)
        .value("BEAR", regime::RegimeType::Bear)
        .value("CRISIS", regime::RegimeType::Crisis);

    py::class_<regime::RegimeState>(m, "RegimeState")
        .def_readonly("regime", &regime::RegimeState::regime)
        .def_readonly("confidence", &regime::RegimeState::confidence)
        .def_readonly("probabilities", &regime::RegimeState::probabilities)
        .def_readonly("timestamp", &regime::RegimeState::timestamp);

    py::enum_<walkforward::WalkForwardConfig::WindowType>(m_walkforward, "WindowType")
        .value("ROLLING", walkforward::WalkForwardConfig::WindowType::Rolling)
        .value("ANCHORED", walkforward::WalkForwardConfig::WindowType::Anchored)
        .value("REGIME_AWARE", walkforward::WalkForwardConfig::WindowType::RegimeAware);

    py::enum_<walkforward::WalkForwardConfig::OptMethod>(m_walkforward, "OptMethod")
        .value("GRID", walkforward::WalkForwardConfig::OptMethod::Grid)
        .value("RANDOM", walkforward::WalkForwardConfig::OptMethod::Random)
        .value("BAYESIAN", walkforward::WalkForwardConfig::OptMethod::Bayesian);

    py::enum_<walkforward::ParameterDef::Type>(m_walkforward, "ParamType")
        .value("INT", walkforward::ParameterDef::Type::Int)
        .value("DOUBLE", walkforward::ParameterDef::Type::Double)
        .value("CATEGORICAL", walkforward::ParameterDef::Type::Categorical);

    py::enum_<walkforward::ParameterDef::Distribution>(m_walkforward, "ParamDistribution")
        .value("UNIFORM", walkforward::ParameterDef::Distribution::Uniform)
        .value("LOGUNIFORM", walkforward::ParameterDef::Distribution::LogUniform)
        .value("NORMAL", walkforward::ParameterDef::Distribution::Normal);

    py::class_<walkforward::ParameterDef>(m_walkforward, "ParameterDef")
        .def(py::init<>())
        .def(py::init([](py::kwargs kwargs) { return parameter_def_from_kwargs(kwargs); }))
        .def_readwrite("name", &walkforward::ParameterDef::name)
        .def_readwrite("type", &walkforward::ParameterDef::type)
        .def_readwrite("min_value", &walkforward::ParameterDef::min_value)
        .def_readwrite("max_value", &walkforward::ParameterDef::max_value)
        .def_readwrite("step", &walkforward::ParameterDef::step)
        .def_readwrite("categories", &walkforward::ParameterDef::categories)
        .def_readwrite("distribution", &walkforward::ParameterDef::distribution);

    py::class_<walkforward::WalkForwardConfig>(m_walkforward, "WalkForwardConfig")
        .def(py::init<>())
        .def(py::init([](py::kwargs kwargs) { return walkforward_config_from_kwargs(kwargs); }))
        .def_readwrite("window_type", &walkforward::WalkForwardConfig::window_type)
        .def_property("in_sample_days",
                      [](const walkforward::WalkForwardConfig& cfg) {
                          return static_cast<int>(cfg.in_sample_period.total_seconds() / 86400);
                      },
                      [](walkforward::WalkForwardConfig& cfg, int days) {
                          cfg.in_sample_period = Duration::days(days);
                      })
        .def_property("out_of_sample_days",
                      [](const walkforward::WalkForwardConfig& cfg) {
                          return static_cast<int>(cfg.out_of_sample_period.total_seconds() / 86400);
                      },
                      [](walkforward::WalkForwardConfig& cfg, int days) {
                          cfg.out_of_sample_period = Duration::days(days);
                      })
        .def_property("step_days",
                      [](const walkforward::WalkForwardConfig& cfg) {
                          return static_cast<int>(cfg.step_size.total_seconds() / 86400);
                      },
                      [](walkforward::WalkForwardConfig& cfg, int days) {
                          cfg.step_size = Duration::days(days);
                      })
        .def_readwrite("optimization_method", &walkforward::WalkForwardConfig::optimization_method)
        .def_readwrite("max_trials", &walkforward::WalkForwardConfig::max_trials)
        .def_readwrite("fitness_metric", &walkforward::WalkForwardConfig::fitness_metric)
        .def_readwrite("maximize", &walkforward::WalkForwardConfig::maximize)
        .def_readwrite("retrain_regime_each_window",
                       &walkforward::WalkForwardConfig::retrain_regime_each_window)
        .def_readwrite("optimize_per_regime", &walkforward::WalkForwardConfig::optimize_per_regime)
        .def_readwrite("disable_default_regime_training",
                       &walkforward::WalkForwardConfig::disable_default_regime_training)
        .def_readwrite("num_parallel_backtests", &walkforward::WalkForwardConfig::num_parallel_backtests)
        .def_readwrite("enable_overfitting_detection",
                       &walkforward::WalkForwardConfig::enable_overfitting_detection)
        .def_readwrite("max_is_oos_ratio", &walkforward::WalkForwardConfig::max_is_oos_ratio)
        .def_readwrite("initial_capital", &walkforward::WalkForwardConfig::initial_capital)
        .def_readwrite("bar_type", &walkforward::WalkForwardConfig::bar_type)
        .def_readwrite("periods_per_year", &walkforward::WalkForwardConfig::periods_per_year);

    py::class_<walkforward::WindowResult>(m_walkforward, "WindowResult")
        .def_property_readonly("in_sample_range", [](const walkforward::WindowResult& r) {
            return py::make_tuple(r.in_sample_range.start.to_string("%Y-%m-%d"),
                                  r.in_sample_range.end.to_string("%Y-%m-%d"));
        })
        .def_property_readonly("out_of_sample_range", [](const walkforward::WindowResult& r) {
            return py::make_tuple(r.out_of_sample_range.start.to_string("%Y-%m-%d"),
                                  r.out_of_sample_range.end.to_string("%Y-%m-%d"));
        })
        .def_readonly("optimal_params", &walkforward::WindowResult::optimal_params)
        .def_readonly("is_fitness", &walkforward::WindowResult::is_fitness)
        .def_readonly("is_results", &walkforward::WindowResult::is_results)
        .def_readonly("oos_fitness", &walkforward::WindowResult::oos_fitness)
        .def_readonly("oos_results", &walkforward::WindowResult::oos_results)
        .def_readonly("regime_distribution", &walkforward::WindowResult::regime_distribution)
        .def_readonly("efficiency_ratio", &walkforward::WindowResult::efficiency_ratio);

    py::class_<walkforward::WalkForwardResults>(m_walkforward, "WalkForwardResults")
        .def_readonly("windows", &walkforward::WalkForwardResults::windows)
        .def_readonly("stitched_oos_results", &walkforward::WalkForwardResults::stitched_oos_results)
        .def_readonly("param_evolution", &walkforward::WalkForwardResults::param_evolution)
        .def_readonly("param_stability_score", &walkforward::WalkForwardResults::param_stability_score)
        .def_readonly("avg_is_sharpe", &walkforward::WalkForwardResults::avg_is_sharpe)
        .def_readonly("avg_oos_sharpe", &walkforward::WalkForwardResults::avg_oos_sharpe)
        .def_readonly("overall_oos_sharpe", &walkforward::WalkForwardResults::overall_oos_sharpe)
        .def_readonly("avg_efficiency_ratio", &walkforward::WalkForwardResults::avg_efficiency_ratio)
        .def_readonly("potential_overfit", &walkforward::WalkForwardResults::potential_overfit)
        .def_readonly("overfit_diagnosis", &walkforward::WalkForwardResults::overfit_diagnosis)
        .def_readonly("oos_sharpe_by_regime", &walkforward::WalkForwardResults::oos_sharpe_by_regime)
        .def_readonly("regime_consistency_score",
                      &walkforward::WalkForwardResults::regime_consistency_score);

    py::class_<PyWalkForwardOptimizer>(m_walkforward, "WalkForwardOptimizer")
        .def(py::init<const walkforward::WalkForwardConfig&>())
        .def("optimize", &PyWalkForwardOptimizer::optimize,
             py::arg("params"),
             py::arg("strategy_factory"),
             py::arg("data_source"),
             py::arg("date_range"),
             py::arg("detector_config") = py::none());

    py::class_<regime::RegimeTransition>(m, "RegimeTransition")
        .def_readonly("from_regime", &regime::RegimeTransition::from)
        .def_readonly("to_regime", &regime::RegimeTransition::to)
        .def_readonly("timestamp", &regime::RegimeTransition::timestamp);

    PYBIND11_NUMPY_DTYPE(BarRow, timestamp, open, high, low, close, volume);

    py::class_<BarRow>(m_data, "BarRow", py::buffer_protocol())
        .def_readonly("timestamp", &BarRow::timestamp)
        .def_readonly("open", &BarRow::open)
        .def_readonly("high", &BarRow::high)
        .def_readonly("low", &BarRow::low)
        .def_readonly("close", &BarRow::close)
        .def_readonly("volume", &BarRow::volume);

    py::class_<engine::Order>(m, "Order")
        .def(py::init([](const std::string& symbol,
                         engine::OrderSide side,
                         engine::OrderType type,
                         double quantity,
                         std::optional<double> limit_price,
                         std::optional<double> stop_price) {
            engine::Order order;
            order.symbol = symbol_from_string(symbol);
            order.side = side;
            order.type = type;
            order.quantity = quantity;
            if (limit_price) order.limit_price = *limit_price;
            if (stop_price) order.stop_price = *stop_price;
            return order;
        }),
            py::arg("symbol"),
            py::arg("side"),
            py::arg("type"),
            py::arg("quantity"),
            py::arg("limit_price") = py::none(),
            py::arg("stop_price") = py::none())
        .def_readwrite("id", &engine::Order::id)
        .def_property("symbol",
                      [](const engine::Order& order) { return symbol_to_string(order.symbol); },
                      [](engine::Order& order, const std::string& symbol) {
                          order.symbol = symbol_from_string(symbol);
                      })
        .def_readwrite("side", &engine::Order::side)
        .def_readwrite("type", &engine::Order::type)
        .def_readwrite("tif", &engine::Order::tif)
        .def_readwrite("quantity", &engine::Order::quantity)
        .def_readwrite("limit_price", &engine::Order::limit_price)
        .def_readwrite("stop_price", &engine::Order::stop_price)
        .def_readonly("filled_quantity", &engine::Order::filled_quantity)
        .def_readonly("avg_fill_price", &engine::Order::avg_fill_price)
        .def_readwrite("status", &engine::Order::status);

    py::class_<engine::Fill>(m, "Fill")
        .def(py::init<>())
        .def_readwrite("id", &engine::Fill::id)
        .def_readwrite("order_id", &engine::Fill::order_id)
        .def_property("symbol",
                      [](const engine::Fill& fill) { return symbol_to_string(fill.symbol); },
                      [](engine::Fill& fill, const std::string& symbol) {
                          fill.symbol = symbol_from_string(symbol);
                      })
        .def_readwrite("quantity", &engine::Fill::quantity)
        .def_readwrite("price", &engine::Fill::price)
        .def_readwrite("timestamp", &engine::Fill::timestamp)
        .def_readwrite("commission", &engine::Fill::commission)
        .def_readwrite("slippage", &engine::Fill::slippage)
        .def_readwrite("is_maker", &engine::Fill::is_maker);

    py::class_<data::Bar>(m_data, "Bar")
        .def(py::init<>())
        .def_readwrite("timestamp", &data::Bar::timestamp)
        .def_property("symbol",
                      [](const data::Bar& bar) { return symbol_to_string(bar.symbol); },
                      [](data::Bar& bar, const std::string& symbol) {
                          bar.symbol = symbol_from_string(symbol);
                      })
        .def_readwrite("open", &data::Bar::open)
        .def_readwrite("high", &data::Bar::high)
        .def_readwrite("low", &data::Bar::low)
        .def_readwrite("close", &data::Bar::close)
        .def_readwrite("volume", &data::Bar::volume)
        .def_readwrite("trade_count", &data::Bar::trade_count)
        .def_readwrite("vwap", &data::Bar::vwap)
        .def("mid", &data::Bar::mid)
        .def("typical", &data::Bar::typical)
        .def("range", &data::Bar::range)
        .def("is_bullish", &data::Bar::is_bullish)
        .def("is_bearish", &data::Bar::is_bearish);

    py::class_<data::Tick>(m_data, "Tick")
        .def(py::init<>())
        .def_readwrite("timestamp", &data::Tick::timestamp)
        .def_property("symbol",
                      [](const data::Tick& tick) { return symbol_to_string(tick.symbol); },
                      [](data::Tick& tick, const std::string& symbol) {
                          tick.symbol = symbol_from_string(symbol);
                      })
        .def_readwrite("price", &data::Tick::price)
        .def_readwrite("quantity", &data::Tick::quantity)
        .def_readwrite("flags", &data::Tick::flags);

    py::class_<data::Quote>(m_data, "Quote")
        .def(py::init<>())
        .def_readwrite("timestamp", &data::Quote::timestamp)
        .def_property("symbol",
                      [](const data::Quote& quote) { return symbol_to_string(quote.symbol); },
                      [](data::Quote& quote, const std::string& symbol) {
                          quote.symbol = symbol_from_string(symbol);
                      })
        .def_readwrite("bid", &data::Quote::bid)
        .def_readwrite("ask", &data::Quote::ask)
        .def_readwrite("bid_size", &data::Quote::bid_size)
        .def_readwrite("ask_size", &data::Quote::ask_size)
        .def("mid", &data::Quote::mid)
        .def("spread", &data::Quote::spread)
        .def("spread_bps", &data::Quote::spread_bps);

    py::class_<data::BookLevel>(m_data, "BookLevel")
        .def(py::init<>())
        .def_readwrite("price", &data::BookLevel::price)
        .def_readwrite("quantity", &data::BookLevel::quantity)
        .def_readwrite("num_orders", &data::BookLevel::num_orders);

    py::class_<data::OrderBook>(m_data, "OrderBook")
        .def(py::init<>())
        .def_readwrite("timestamp", &data::OrderBook::timestamp)
        .def_property("symbol",
                      [](const data::OrderBook& book) { return symbol_to_string(book.symbol); },
                      [](data::OrderBook& book, const std::string& symbol) {
                          book.symbol = symbol_from_string(symbol);
                      })
        .def_property("bids",
                      [](const data::OrderBook& book) {
                          return std::vector<data::BookLevel>(book.bids.begin(), book.bids.end());
                      },
                      [](data::OrderBook& book, const std::vector<data::BookLevel>& bids) {
                          for (size_t i = 0; i < book.bids.size() && i < bids.size(); ++i) {
                              book.bids[i] = bids[i];
                          }
                      })
        .def_property("asks",
                      [](const data::OrderBook& book) {
                          return std::vector<data::BookLevel>(book.asks.begin(), book.asks.end());
                      },
                      [](data::OrderBook& book, const std::vector<data::BookLevel>& asks) {
                          for (size_t i = 0; i < book.asks.size() && i < asks.size(); ++i) {
                              book.asks[i] = asks[i];
                          }
                      });

    py::enum_<data::BarType>(m_data, "BarType")
        .value("TIME_1MIN", data::BarType::Time_1Min)
        .value("TIME_5MIN", data::BarType::Time_5Min)
        .value("TIME_15MIN", data::BarType::Time_15Min)
        .value("TIME_30MIN", data::BarType::Time_30Min)
        .value("TIME_1HOUR", data::BarType::Time_1Hour)
        .value("TIME_4HOUR", data::BarType::Time_4Hour)
        .value("TIME_1DAY", data::BarType::Time_1Day)
        .value("VOLUME", data::BarType::Volume)
        .value("TICK", data::BarType::Tick)
        .value("DOLLAR", data::BarType::Dollar);

    py::class_<data::AlpacaDataClient>(m_data, "AlpacaDataClient")
        .def(py::init([](const py::dict& cfg) {
            data::AlpacaDataClient::Config config;
            if (cfg.contains("api_key")) config.api_key = cfg["api_key"].cast<std::string>();
            if (cfg.contains("secret_key")) config.secret_key = cfg["secret_key"].cast<std::string>();
            if (cfg.contains("trading_base_url")) {
                config.trading_base_url = cfg["trading_base_url"].cast<std::string>();
            }
            if (cfg.contains("data_base_url")) {
                config.data_base_url = cfg["data_base_url"].cast<std::string>();
            }
            if (cfg.contains("timeout_seconds")) {
                config.timeout_seconds = cfg["timeout_seconds"].cast<int>();
            }
            return data::AlpacaDataClient(config);
        }))
        .def("list_assets", [](const data::AlpacaDataClient& client,
                               const std::string& status,
                               const std::string& asset_class) {
            auto res = client.list_assets(status, asset_class);
            if (res.is_err()) {
                throw std::runtime_error(res.error().to_string());
            }
            return res.value();
        }, py::arg("status") = "active", py::arg("asset_class") = "us_equity")
        .def("get_bars", [](const data::AlpacaDataClient& client,
                            const std::vector<std::string>& symbols,
                            const std::string& timeframe,
                            const std::string& start,
                            const std::string& end,
                            int limit,
                            const std::string& page_token) {
            auto res = client.get_bars(symbols, timeframe, start, end, limit, page_token);
            if (res.is_err()) {
                throw std::runtime_error(res.error().to_string());
            }
            return res.value();
        }, py::arg("symbols"), py::arg("timeframe"),
           py::arg("start") = "", py::arg("end") = "", py::arg("limit") = 0,
           py::arg("page_token") = "")
        .def("get_trades", [](const data::AlpacaDataClient& client,
                              const std::vector<std::string>& symbols,
                              const std::string& start,
                              const std::string& end,
                              int limit,
                              const std::string& page_token) {
            auto res = client.get_trades(symbols, start, end, limit, page_token);
            if (res.is_err()) {
                throw std::runtime_error(res.error().to_string());
            }
            return res.value();
        }, py::arg("symbols"), py::arg("start") = "", py::arg("end") = "",
           py::arg("limit") = 0, py::arg("page_token") = "")
        .def("get_snapshot", [](const data::AlpacaDataClient& client,
                                const std::string& symbol) {
            auto res = client.get_snapshot(symbol);
            if (res.is_err()) {
                throw std::runtime_error(res.error().to_string());
            }
            return res.value();
        }, py::arg("symbol"));

    py::class_<engine::Position>(m_engine, "Position")
        .def_property_readonly("symbol", [](const engine::Position& pos) {
            return symbol_to_string(pos.symbol);
        })
        .def_readonly("quantity", &engine::Position::quantity)
        .def_readonly("avg_cost", &engine::Position::avg_cost)
        .def_readonly("current_price", &engine::Position::current_price)
        .def_property_readonly("market_value", &engine::Position::market_value)
        .def_property_readonly("unrealized_pnl", &engine::Position::unrealized_pnl)
        .def_property_readonly("unrealized_pnl_pct", &engine::Position::unrealized_pnl_pct);

    py::class_<engine::Portfolio>(m_engine, "Portfolio")
        .def_property_readonly("cash", &engine::Portfolio::cash)
        .def_property_readonly("equity", &engine::Portfolio::equity)
        .def_property_readonly("gross_exposure", &engine::Portfolio::gross_exposure)
        .def_property_readonly("net_exposure", &engine::Portfolio::net_exposure)
        .def_property_readonly("leverage", &engine::Portfolio::leverage)
        .def("get_position", [](const engine::Portfolio& p, const std::string& symbol) {
            return p.get_position(symbol_from_string(symbol));
        })
        .def("get_all_positions", &engine::Portfolio::get_all_positions)
        .def("equity_curve", [](const engine::Portfolio& p) {
            return portfolio_equity_dataframe(p.equity_curve());
        });

    py::class_<BacktestConfig>(m_engine, "BacktestConfig")
        .def(py::init<>())
        .def_readwrite("data_source", &BacktestConfig::data_source)
        .def_readwrite("data_config", &BacktestConfig::data_config)
        .def_readwrite("symbols", &BacktestConfig::symbols)
        .def_readwrite("start_date", &BacktestConfig::start_date)
        .def_readwrite("end_date", &BacktestConfig::end_date)
        .def_readwrite("bar_type", &BacktestConfig::bar_type)
        .def_readwrite("initial_capital", &BacktestConfig::initial_capital)
        .def_readwrite("currency", &BacktestConfig::currency)
        .def_readwrite("regime_detector", &BacktestConfig::regime_detector)
        .def_readwrite("regime_params", &BacktestConfig::regime_params)
        .def_readwrite("plugins_search_paths", &BacktestConfig::plugins_search_paths)
        .def_readwrite("plugins_load", &BacktestConfig::plugins_load)
        .def_readwrite("slippage_model", &BacktestConfig::slippage_model)
        .def_readwrite("slippage_params", &BacktestConfig::slippage_params)
        .def_readwrite("commission_model", &BacktestConfig::commission_model)
        .def_readwrite("commission_params", &BacktestConfig::commission_params)
        .def_readwrite("risk_params", &BacktestConfig::risk_params)
        .def_readwrite("strategy_params", &BacktestConfig::strategy_params)
        .def_static("from_dict", &BacktestConfig::from_dict)
        .def_static("from_yaml", &BacktestConfig::from_yaml_file);

    py::class_<engine::BacktestResults>(m_engine, "BacktestResults")
        .def_property_readonly("total_return", [](const engine::BacktestResults& r) { return r.total_return; })
        .def_property_readonly("max_drawdown", [](const engine::BacktestResults& r) { return r.max_drawdown; })
        .def_property_readonly("sharpe_ratio", [](const engine::BacktestResults& r) {
            auto stats = metrics::compute_stats(r.metrics.equity_curve(), 252.0);
            return stats.sharpe;
        })
        .def_property_readonly("sortino_ratio", [](const engine::BacktestResults& r) {
            auto stats = metrics::compute_stats(r.metrics.equity_curve(), 252.0);
            return stats.sortino;
        })
        .def_property_readonly("win_rate", [](const engine::BacktestResults& r) {
            return win_rate_from_equity(r.metrics.equity_curve());
        })
        .def_property_readonly("profit_factor", [](const engine::BacktestResults& r) {
            return profit_factor_from_equity(r.metrics.equity_curve());
        })
        .def_property_readonly("num_trades", [](const engine::BacktestResults& r) {
            return static_cast<int64_t>(r.fills.size());
        })
        .def("equity_curve", [](const engine::BacktestResults& r) {
            return equity_curve_dataframe(r.metrics.equity_curve());
        })
        .def("trades", [](const engine::BacktestResults& r) {
            return fills_dataframe(r.fills);
        })
        .def("report_csv", [](const engine::BacktestResults& r) {
            auto report = metrics::build_report(r.metrics, r.fills);
            return metrics::ReportWriter::to_csv(report);
        })
        .def("report_json", [](const engine::BacktestResults& r) {
            auto report = metrics::build_report(r.metrics, r.fills);
            return metrics::ReportWriter::to_json(report);
        })
        .def("performance_summary", [](const engine::BacktestResults& r) {
            auto report = metrics::build_report(r.metrics, r.fills);
            return performance_summary_to_dict(report.performance_summary);
        })
        .def("performance_stats", [](const engine::BacktestResults& r) {
            auto report = metrics::build_report(r.metrics, r.fills);
            return performance_stats_to_dict(report.performance);
        })
        .def("regime_performance", [](const engine::BacktestResults& r) {
            auto report = metrics::build_report(r.metrics, r.fills);
            return regime_performance_to_dict(report.regime_performance);
        })
        .def("transition_metrics", [](const engine::BacktestResults& r) {
            auto report = metrics::build_report(r.metrics, r.fills);
            return transition_stats_to_dict(report.transitions);
        })
        .def("regime_metrics", [](const engine::BacktestResults& r) {
            py::dict out;
            const auto& results = r.metrics.regime_attribution().results();
            for (const auto& [regime_type, metrics] : results) {
                py::dict entry;
                entry["return"] = metrics.total_return;
                entry["sharpe"] = metrics.sharpe;
                entry["time_pct"] = metrics.time_pct;
                entry["max_drawdown"] = metrics.max_drawdown;
                entry["observations"] = metrics.observations;
                out[py::str(regime_type_name(regime_type))] = entry;
            }
            return out;
        })
        .def("regime_history", [](const engine::BacktestResults& r) {
            return r.regime_history;
        });

    py::class_<PyBacktestEngine>(m_engine, "BacktestEngine")
        .def(py::init<const BacktestConfig&>())
        .def("run", [](PyBacktestEngine& engine, py::object strategy) {
            return engine.run(std::move(strategy));
        })
        .def("run_parallel", [](PyBacktestEngine& engine,
                                 const std::vector<py::dict>& param_sets,
                                 py::object factory,
                                 int num_threads) {
            return engine.run_parallel(param_sets, factory, num_threads);
        }, py::arg("param_sets"), py::arg("strategy_factory"), py::arg("num_threads") = -1)
        .def_property_readonly("portfolio", &PyBacktestEngine::portfolio,
                                py::return_value_policy::reference_internal)
        .def_property_readonly("current_regime", &PyBacktestEngine::current_regime,
                                py::return_value_policy::reference_internal)
        .def("get_close_prices", &PyBacktestEngine::get_close_prices,
             py::arg("symbol"),
             py::arg("start") = py::none(),
             py::arg("end") = py::none())
        .def("get_bars_array", &PyBacktestEngine::get_bars_array,
             py::arg("symbol"),
             py::arg("start") = py::none(),
             py::arg("end") = py::none());

    py::class_<strategy::StrategyContext>(m_strategy, "StrategyContext")
        .def("submit_order", [](strategy::StrategyContext& ctx, engine::Order order) {
            auto result = ctx.submit_order(std::move(order));
            if (result.is_err()) {
                throw std::runtime_error(result.error().to_string());
            }
            return result.value();
        })
        .def("cancel_order", [](strategy::StrategyContext& ctx, engine::OrderId id) {
            auto result = ctx.cancel_order(id);
            if (result.is_err()) {
                throw std::runtime_error(result.error().to_string());
            }
        })
        .def("portfolio", [](strategy::StrategyContext& ctx) -> engine::Portfolio& {
            return ctx.portfolio();
        }, py::return_value_policy::reference_internal)
        .def("get_position", [](const strategy::StrategyContext& ctx, const std::string& symbol) {
            auto pos = ctx.portfolio().get_position(symbol_from_string(symbol));
            return pos ? pos->quantity : 0.0;
        })
        .def("current_regime", &strategy::StrategyContext::current_regime)
        .def("current_time", &strategy::StrategyContext::current_time)
        .def("get_latest_bar", &strategy::StrategyContext::latest_bar)
        .def("get_latest_quote", &strategy::StrategyContext::latest_quote)
        .def("get_latest_book", &strategy::StrategyContext::latest_order_book)
        .def("get_bars", [](const strategy::StrategyContext& ctx, const std::string& symbol,
                            int n) {
            return ctx.recent_bars(symbol_from_string(symbol), static_cast<size_t>(n));
        })
        .def("schedule_timer", &strategy::StrategyContext::schedule_timer)
        .def("cancel_timer", &strategy::StrategyContext::cancel_timer);

    py::class_<strategy::Strategy, PyStrategyTrampoline>(m_strategy, "Strategy")
        .def(py::init<>())
        .def("initialize", &strategy::Strategy::initialize)
        .def("on_start", &strategy::Strategy::on_start)
        .def("on_stop", &strategy::Strategy::on_stop)
        .def("on_bar", &strategy::Strategy::on_bar)
        .def("on_tick", &strategy::Strategy::on_tick)
        .def("on_quote", &strategy::Strategy::on_quote)
        .def("on_order_book", &strategy::Strategy::on_order_book)
        .def("on_order_update", &strategy::Strategy::on_order_update)
        .def("on_fill", &strategy::Strategy::on_fill)
        .def("on_regime_change", &strategy::Strategy::on_regime_change)
        .def("on_end_of_day", &strategy::Strategy::on_end_of_day)
        .def("on_timer", &strategy::Strategy::on_timer)
        .def_property_readonly("ctx", [](strategy::Strategy& s) {
            return s.context();
        }, py::return_value_policy::reference_internal);

    m.def("register_strategy", [](const std::string& name, py::object strategy_class) {
        strategy::StrategyFactory::instance().register_creator(
            name, [strategy_class](const Config&) {
                py::gil_scoped_acquire acquire;
                py::object instance = strategy_class();
                return std::unique_ptr<strategy::Strategy>(new PythonStrategyAdapter(instance));
            });
    });

    (void)m_regime;
    (void)m_metrics;
    (void)m_risk;
}
