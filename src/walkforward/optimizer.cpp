#include "regimeflow/walkforward/optimizer.h"

#include "regimeflow/engine/backtest_runner.h"
#include "regimeflow/metrics/performance_metrics.h"
#include "regimeflow/regime/features.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <random>
#include <thread>

namespace {

double clamp_value(double v, double min_v, double max_v) {
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

double apply_step(double value, double min_v, double max_v, double step) {
    if (step <= 0.0) {
        return clamp_value(value, min_v, max_v);
    }
    double steps = std::round((value - min_v) / step);
    return clamp_value(min_v + steps * step, min_v, max_v);
}

std::variant<int, double, std::string> sample_param_value(
    const regimeflow::walkforward::ParameterDef& param,
    std::mt19937& rng) {
    if (param.type == regimeflow::walkforward::ParameterDef::Type::Categorical) {
        if (param.categories.empty()) {
            return std::string{};
        }
        std::uniform_int_distribution<size_t> pick(0, param.categories.size() - 1);
        return param.categories[pick(rng)];
    }

    double min_v = param.min_value;
    double max_v = param.max_value;
    if (max_v < min_v) {
        std::swap(min_v, max_v);
    }

    double value = 0.0;
    switch (param.distribution) {
        case regimeflow::walkforward::ParameterDef::Distribution::LogUniform: {
            double safe_min = std::max(min_v, 1e-12);
            std::uniform_real_distribution<double> dist(std::log(safe_min), std::log(max_v));
            value = std::exp(dist(rng));
            break;
        }
        case regimeflow::walkforward::ParameterDef::Distribution::Normal: {
            double mean = 0.5 * (min_v + max_v);
            double stddev = (max_v - min_v) / 6.0;
            if (stddev <= 0.0) {
                value = mean;
            } else {
                std::normal_distribution<double> dist(mean, stddev);
                value = dist(rng);
            }
            break;
        }
        case regimeflow::walkforward::ParameterDef::Distribution::Uniform:
        default: {
            std::uniform_real_distribution<double> dist(min_v, max_v);
            value = dist(rng);
            break;
        }
    }

    value = apply_step(value, min_v, max_v, param.step);
    if (param.type == regimeflow::walkforward::ParameterDef::Type::Int) {
        return static_cast<int>(std::llround(value));
    }
    return value;
}

}  // namespace

namespace regimeflow::walkforward {

WalkForwardOptimizer::WalkForwardOptimizer(const WalkForwardConfig& config) : config_(config) {}

WalkForwardResults WalkForwardOptimizer::optimize(
    const std::vector<ParameterDef>& params,
    std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)> strategy_factory,
    data::DataSource* data_source,
    const TimeRange& full_range,
    std::function<std::unique_ptr<regime::RegimeDetector>()> detector_factory) {
    WalkForwardResults results;
    auto windows = (config_.window_type == WalkForwardConfig::WindowType::RegimeAware)
        ? generate_regime_windows(data_source, detector_factory, full_range)
        : generate_windows(full_range);
    results.windows.reserve(windows.size());

    std::mt19937 rng(1337);

    for (const auto& [is_range, oos_range] : windows) {
        if (cancelled_.load(std::memory_order_relaxed)) {
            break;
        }
        WindowResult window;
        window.in_sample_range = is_range;
        window.out_of_sample_range = oos_range;
        double best_fitness = config_.maximize ? -1e300 : 1e300;
        size_t best_index = 0;
        ParameterSet best_params;
        engine::BacktestResults best_is_results;

        std::vector<ParameterSet> trials;
        if (config_.optimization_method == WalkForwardConfig::OptMethod::Grid) {
            trials = build_grid(params);
        } else if (config_.optimization_method == WalkForwardConfig::OptMethod::Random) {
            trials = build_random_trials(params, config_.max_trials, rng);
        }
        if (trials.empty()) {
            trials.push_back(ParameterSet{});
        }

        if (config_.optimization_method == WalkForwardConfig::OptMethod::Bayesian) {
            int total_trials = std::max(1, config_.max_trials);
            int warmup = std::min(total_trials, 10);
            for (int i = 0; i < total_trials; ++i) {
                if (cancelled_.load(std::memory_order_relaxed)) {
                    break;
                }
                ParameterSet param_set;
                if (i < warmup || best_params.empty()) {
                    param_set = build_random_trials(params, 1, rng).front();
                } else {
                    ParameterSet candidate;
                    for (const auto& param : params) {
                        if (param.type == ParameterDef::Type::Categorical) {
                            std::uniform_real_distribution<double> pick(0.0, 1.0);
                            if (pick(rng) < 0.7) {
                                auto it = best_params.find(param.name);
                                if (it != best_params.end()) {
                                    candidate[param.name] = it->second;
                                } else {
                                    candidate[param.name] = sample_param_value(param, rng);
                                }
                            } else {
                                candidate[param.name] = sample_param_value(param, rng);
                            }
                        } else {
                            auto it = best_params.find(param.name);
                            double base = param.min_value;
                            if (it != best_params.end()) {
                                if (auto* v = std::get_if<int>(&it->second)) {
                                    base = static_cast<double>(*v);
                                } else if (auto* v2 = std::get_if<double>(&it->second)) {
                                    base = *v2;
                                }
                            }
                            double stddev = (param.max_value - param.min_value) / 5.0;
                            std::normal_distribution<double> dist(base, stddev > 0.0 ? stddev : 1.0);
                            double value = dist(rng);
                            value = apply_step(value, param.min_value, param.max_value, param.step);
                            if (param.type == ParameterDef::Type::Int) {
                                candidate[param.name] = static_cast<int>(std::llround(value));
                            } else {
                                candidate[param.name] = value;
                            }
                        }
                    }
                    param_set = std::move(candidate);
                }

                auto is_results = run_backtest(param_set, strategy_factory, data_source, is_range,
                                               is_range, detector_factory,
                                               config_.retrain_regime_each_window);
                double fitness = compute_fitness(is_results);
                for (const auto& cb : trial_callbacks_) {
                    cb(param_set, fitness);
                }
                bool better = config_.maximize ? (fitness > best_fitness) : (fitness < best_fitness);
                if (better) {
                    best_fitness = fitness;
                    best_params = param_set;
                    best_is_results = std::move(is_results);
                }
            }
        } else {
            auto outcomes = evaluate_trials(trials, strategy_factory, data_source, is_range,
                                            detector_factory, config_.retrain_regime_each_window);
            for (size_t i = 0; i < outcomes.size(); ++i) {
                double fitness = outcomes[i].fitness;
                bool better = config_.maximize ? (fitness > best_fitness) : (fitness < best_fitness);
                if (better || (fitness == best_fitness && i < best_index)) {
                    best_fitness = fitness;
                    best_index = i;
                }
            }
            if (!outcomes.empty()) {
                best_params = trials[best_index];
                best_is_results = std::move(outcomes[best_index].results);
            }
        }

        window.optimal_params = best_params;
        window.is_fitness = best_fitness;
        window.is_results = std::move(best_is_results);
        window.regime_distribution = extract_regime_distribution(window.is_results);

        window.oos_results = run_backtest(best_params, strategy_factory, data_source, oos_range,
                                          is_range, detector_factory,
                                          config_.retrain_regime_each_window);
        window.oos_fitness = compute_fitness(window.oos_results);
        if (window.is_fitness != 0) {
            window.efficiency_ratio = window.oos_fitness / window.is_fitness;
        }

        results.windows.push_back(window);

        for (const auto& cb : window_callbacks_) {
            cb(results.windows.back());
        }
    }

    results.stitched_oos_results = stitch_oos_results(results.windows);
    analyze_oos_performance(results);
    analyze_param_stability(results);
    if (config_.enable_overfitting_detection) {
        analyze_overfitting(results);
    }

    return results;
}

void WalkForwardOptimizer::on_window_complete(std::function<void(const WindowResult&)> callback) {
    window_callbacks_.push_back(std::move(callback));
}

void WalkForwardOptimizer::on_trial_complete(
    std::function<void(const ParameterSet&, double)> callback) {
    trial_callbacks_.push_back(std::move(callback));
}

void WalkForwardOptimizer::on_regime_train(RegimeTrainingHook callback) {
    regime_train_hooks_.push_back(std::move(callback));
}

void WalkForwardOptimizer::on_regime_trained(RegimeTrainingCallback callback) {
    regime_trained_callbacks_.push_back(std::move(callback));
}

void WalkForwardOptimizer::cancel() {
    cancelled_.store(true, std::memory_order_relaxed);
}

std::vector<std::pair<TimeRange, TimeRange>> WalkForwardOptimizer::generate_windows(
    TimeRange full_range) const {
    std::vector<std::pair<TimeRange, TimeRange>> windows;

    Timestamp cursor = full_range.start;
    Timestamp end = full_range.end;

    while (cursor < end) {
        TimeRange is_range;
        TimeRange oos_range;

        if (config_.window_type == WalkForwardConfig::WindowType::Anchored) {
            is_range.start = full_range.start;
        } else {
            is_range.start = cursor;
        }
        is_range.end = is_range.start + config_.in_sample_period;
        oos_range.start = is_range.end;
        oos_range.end = oos_range.start + config_.out_of_sample_period;

        if (oos_range.start >= end) {
            break;
        }
        if (oos_range.end > end) {
            oos_range.end = end;
        }

        windows.emplace_back(is_range, oos_range);
        cursor = cursor + config_.step_size;
    }

    return windows;
}

std::vector<std::pair<TimeRange, TimeRange>> WalkForwardOptimizer::generate_regime_windows(
    data::DataSource* data_source,
    std::function<std::unique_ptr<regime::RegimeDetector>()> detector_factory,
    const TimeRange& full_range) const {
    if (!data_source) {
        return generate_windows(full_range);
    }
    if (!detector_factory) {
        return generate_windows(full_range);
    }

    std::vector<SymbolId> symbols;
    for (const auto& info : data_source->get_available_symbols()) {
        symbols.push_back(info.id);
    }
    if (symbols.empty()) {
        return generate_windows(full_range);
    }

    auto bars = data_source->get_bars(symbols.front(), full_range, config_.bar_type);
    if (bars.empty()) {
        return generate_windows(full_range);
    }

    auto detector = detector_factory();
    if (!detector) {
        return generate_windows(full_range);
    }

    struct LabeledBar {
        Timestamp ts;
        regime::RegimeType regime;
    };
    std::vector<LabeledBar> labeled;
    labeled.reserve(bars.size());
    std::map<regime::RegimeType, bool> regime_present;
    for (const auto& bar : bars) {
        auto state = detector->on_bar(bar);
        labeled.push_back({bar.timestamp, state.regime});
        regime_present[state.regime] = true;
    }

    std::vector<regime::RegimeType> required;
    required.reserve(regime_present.size());
    for (const auto& [regime_type, present] : regime_present) {
        if (present) {
            required.push_back(regime_type);
        }
    }
    if (required.empty()) {
        return generate_windows(full_range);
    }

    std::vector<std::pair<TimeRange, TimeRange>> windows;
    auto base_windows = generate_windows(full_range);
    for (const auto& [base_is, base_oos] : base_windows) {
        TimeRange is_range = base_is;
        TimeRange oos_range = base_oos;

        size_t start_index = 0;
        while (start_index < labeled.size() && labeled[start_index].ts < is_range.start) {
            ++start_index;
        }
        size_t end_index = start_index;
        while (end_index < labeled.size() && labeled[end_index].ts < is_range.end) {
            ++end_index;
        }

        std::map<regime::RegimeType, bool> seen;
        for (size_t i = start_index; i < end_index; ++i) {
            seen[labeled[i].regime] = true;
        }
        auto missing_regimes = [&]() {
            for (auto regime_type : required) {
                if (seen.find(regime_type) == seen.end()) {
                    return true;
                }
            }
            return false;
        };

        while (missing_regimes() && start_index > 0) {
            --start_index;
            seen[labeled[start_index].regime] = true;
        }
        if (start_index < labeled.size()) {
            is_range.start = labeled[start_index].ts;
        }

        if (oos_range.start >= full_range.end) {
            break;
        }
        if (oos_range.end > full_range.end) {
            oos_range.end = full_range.end;
        }
        windows.emplace_back(is_range, oos_range);
    }

    return windows;
}

double WalkForwardOptimizer::compute_fitness(const engine::BacktestResults& results) const {
    if (config_.optimize_per_regime || config_.window_type == WalkForwardConfig::WindowType::RegimeAware) {
        const auto& regimes = results.metrics.regime_attribution().results();
        if (!regimes.empty()) {
            double weighted = 0.0;
            double weight_sum = 0.0;
            for (const auto& [regime, perf] : regimes) {
                double metric = 0.0;
                if (config_.fitness_metric == "return") {
                    metric = perf.total_return;
                } else if (config_.fitness_metric == "drawdown") {
                    metric = perf.max_drawdown;
                } else if (config_.fitness_metric == "calmar") {
                    metric = perf.max_drawdown > 0.0 ? perf.total_return / perf.max_drawdown : 0.0;
                } else if (config_.fitness_metric == "sortino" || config_.fitness_metric == "sharpe") {
                    metric = perf.sharpe;
                } else {
                    metric = perf.sharpe;
                }
                weighted += metric * perf.time_pct;
                weight_sum += perf.time_pct;
            }
            if (weight_sum > 0.0) {
                return weighted / weight_sum;
            }
        }
    }

    auto stats = metrics::compute_stats(results.metrics.equity_curve(), config_.periods_per_year);
    if (config_.fitness_metric == "sharpe") {
        return stats.sharpe;
    }
    if (config_.fitness_metric == "sortino") {
        return stats.sortino;
    }
    if (config_.fitness_metric == "calmar") {
        return stats.calmar;
    }
    if (config_.fitness_metric == "return") {
        return stats.total_return;
    }
    if (config_.fitness_metric == "drawdown") {
        return stats.max_drawdown;
    }
    return stats.sharpe;
}

std::vector<ParameterSet> WalkForwardOptimizer::build_grid(
    const std::vector<ParameterDef>& params) const {
    std::vector<ParameterSet> grid;
    grid.push_back(ParameterSet{});

    for (const auto& param : params) {
        std::vector<ParameterSet> next;
        if (param.type == ParameterDef::Type::Categorical) {
            for (const auto& base : grid) {
                for (const auto& value : param.categories) {
                    auto copy = base;
                    copy[param.name] = value;
                    next.push_back(std::move(copy));
                }
            }
        } else {
            double min_v = param.min_value;
            double max_v = param.max_value;
            if (max_v < min_v) {
                std::swap(min_v, max_v);
            }
            double step = param.step != 0 ? param.step : 1.0;
            for (const auto& base : grid) {
                for (int idx = 0;; ++idx) {
                    double value = min_v + static_cast<double>(idx) * step;
                    if (value > max_v + 1e-12) {
                        break;
                    }
                    auto copy = base;
                    if (param.type == ParameterDef::Type::Int) {
                        copy[param.name] = static_cast<int>(value);
                    } else {
                        copy[param.name] = value;
                    }
                    next.push_back(std::move(copy));
                }
            }
        }
        grid = std::move(next);
        if (grid.empty()) {
            grid.push_back(ParameterSet{});
        }
    }

    return grid;
}

std::vector<ParameterSet> WalkForwardOptimizer::build_random_trials(
    const std::vector<ParameterDef>& params,
    int max_trials,
    std::mt19937& rng) const {
    std::vector<ParameterSet> trials;
    if (max_trials <= 0) {
        return trials;
    }
    trials.reserve(static_cast<size_t>(max_trials));
    for (int i = 0; i < max_trials; ++i) {
        ParameterSet param_set;
        for (const auto& param : params) {
            param_set[param.name] = sample_param_value(param, rng);
        }
        trials.push_back(std::move(param_set));
    }
    return trials;
}

std::vector<WalkForwardOptimizer::TrialOutcome> WalkForwardOptimizer::evaluate_trials(
    const std::vector<ParameterSet>& trials,
    std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)>& factory,
    data::DataSource* data,
    const TimeRange& range,
    const std::function<std::unique_ptr<regime::RegimeDetector>()>& detector_factory,
    bool attach_detector) const {
    std::vector<TrialOutcome> outcomes(trials.size());
    if (trials.empty()) {
        return outcomes;
    }

    int num_threads = config_.num_parallel_backtests;
    if (num_threads <= 0) {
        num_threads = static_cast<int>(std::thread::hardware_concurrency());
        if (num_threads <= 0) {
            num_threads = 1;
        }
    }

    std::atomic<size_t> index{0};
    std::exception_ptr failure;
    std::mutex failure_mutex;

    auto worker = [&]() {
        while (true) {
            size_t i = index.fetch_add(1, std::memory_order_relaxed);
            if (i >= trials.size()) {
                break;
            }
            try {
                auto results = run_backtest(trials[i], factory, data, range, range,
                                            detector_factory, attach_detector);
                double fitness = compute_fitness(results);
                outcomes[i].fitness = fitness;
                outcomes[i].results = std::move(results);
            } catch (...) {
                std::lock_guard<std::mutex> lock(failure_mutex);
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }
    if (failure) {
        std::rethrow_exception(failure);
    }

    for (size_t i = 0; i < trials.size(); ++i) {
        for (const auto& cb : trial_callbacks_) {
            cb(trials[i], outcomes[i].fitness);
        }
    }

    return outcomes;
}

engine::BacktestResults WalkForwardOptimizer::run_backtest(
    const ParameterSet& params,
    std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)>& factory,
    data::DataSource* data,
    const TimeRange& range,
    const TimeRange& training_range,
    const std::function<std::unique_ptr<regime::RegimeDetector>()>& detector_factory,
    bool attach_detector) const {
    if (!data) {
        return {};
    }
    std::vector<SymbolId> symbols;
    for (const auto& info : data->get_available_symbols()) {
        symbols.push_back(info.id);
    }
    auto strategy = factory(params);
    if (!strategy) {
        return {};
    }
    engine::BacktestEngine engine(config_.initial_capital);
    if (attach_detector && detector_factory) {
        auto detector = detector_factory();
        if (detector && config_.retrain_regime_each_window) {
            RegimeTrainingContext ctx;
            ctx.data_source = data;
            ctx.training_range = training_range;
            ctx.bar_type = config_.bar_type;
            ctx.symbols = symbols;
            ctx.detector = detector.get();

            bool handled = false;
            for (const auto& hook : regime_train_hooks_) {
                handled = hook(ctx) || handled;
            }
            if (!handled && !config_.disable_default_regime_training) {
                regime::FeatureExtractor extractor;
                std::vector<regime::FeatureVector> features;
                if (!symbols.empty()) {
                    auto iterator = data->create_iterator(symbols, training_range, config_.bar_type);
                    if (iterator) {
                        while (iterator->has_next()) {
                            auto bar = iterator->next();
                            auto feature = extractor.on_bar(bar);
                            if (!feature.empty()) {
                                features.push_back(std::move(feature));
                            }
                        }
                    }
                }
                if (!features.empty()) {
                    detector->train(features);
                }
            }
            for (const auto& cb : regime_trained_callbacks_) {
                cb(ctx);
            }
        }
        engine.set_regime_detector(std::move(detector));
    }
    engine::BacktestRunner runner(&engine, data);
    return runner.run(std::move(strategy), range, symbols, config_.bar_type);
}

std::map<regime::RegimeType, double> WalkForwardOptimizer::extract_regime_distribution(
    const engine::BacktestResults& results) const {
    std::map<regime::RegimeType, double> dist;
    const auto& regimes = results.metrics.regime_attribution().results();
    for (const auto& [regime, perf] : regimes) {
        dist[regime] = perf.time_pct;
    }
    return dist;
}

engine::BacktestResults WalkForwardOptimizer::stitch_oos_results(
    const std::vector<WindowResult>& windows) const {
    engine::BacktestResults stitched;
    bool initialized = false;
    double total_return = 0.0;
    double max_drawdown = 0.0;

    for (const auto& window : windows) {
        const auto& curve = window.oos_results.metrics.equity_curve();
        if (!initialized && !curve.equities().empty()) {
            stitched.metrics = window.oos_results.metrics;
            initialized = true;
        }
        total_return += window.oos_results.total_return;
        if (window.oos_results.max_drawdown > max_drawdown) {
            max_drawdown = window.oos_results.max_drawdown;
        }
    }

    stitched.total_return = total_return;
    stitched.max_drawdown = max_drawdown;
    return stitched;
}

void WalkForwardOptimizer::analyze_overfitting(WalkForwardResults& results) const {
    double ratio_sum = 0.0;
    int count = 0;
    for (const auto& window : results.windows) {
        if (window.is_fitness != 0) {
            ratio_sum += window.oos_fitness / window.is_fitness;
            count += 1;
        }
    }
    results.avg_efficiency_ratio = count > 0 ? ratio_sum / count : 0.0;
    results.potential_overfit = results.avg_efficiency_ratio < (1.0 / config_.max_is_oos_ratio);
    if (results.potential_overfit) {
        results.overfit_diagnosis = "OOS/IS efficiency ratio below threshold";
    }
}

void WalkForwardOptimizer::analyze_param_stability(WalkForwardResults& results) const {
    for (const auto& window : results.windows) {
        for (const auto& [name, value] : window.optimal_params) {
            if (auto* v = std::get_if<int>(&value)) {
                results.param_evolution[name].push_back(static_cast<double>(*v));
            } else if (auto* v2 = std::get_if<double>(&value)) {
                results.param_evolution[name].push_back(*v2);
            }
        }
    }
    for (const auto& [name, values] : results.param_evolution) {
        if (values.empty()) {
            results.param_stability_score[name] = 0.0;
            continue;
        }
        double mean = 0.0;
        for (double v : values) mean += v;
        mean /= static_cast<double>(values.size());
        double variance = 0.0;
        for (double v : values) {
            double diff = v - mean;
            variance += diff * diff;
        }
        variance /= static_cast<double>(values.size());
        results.param_stability_score[name] = variance;
    }
}

void WalkForwardOptimizer::analyze_oos_performance(WalkForwardResults& results) const {
    double is_sum = 0.0;
    double oos_sum = 0.0;
    int count = 0;
    for (const auto& window : results.windows) {
        is_sum += window.is_fitness;
        oos_sum += window.oos_fitness;
        count += 1;
    }
    results.avg_is_sharpe = count > 0 ? is_sum / count : 0.0;
    results.avg_oos_sharpe = count > 0 ? oos_sum / count : 0.0;
    results.overall_oos_sharpe = compute_fitness(results.stitched_oos_results);

    std::map<regime::RegimeType, double> weighted_sum;
    std::map<regime::RegimeType, double> weight_sum;
    for (const auto& window : results.windows) {
        const auto& regimes = window.oos_results.metrics.regime_attribution().results();
        for (const auto& [regime, perf] : regimes) {
            weighted_sum[regime] += perf.sharpe * perf.time_pct;
            weight_sum[regime] += perf.time_pct;
        }
    }
    results.oos_sharpe_by_regime.clear();
    std::vector<double> regime_values;
    for (const auto& [regime, total] : weighted_sum) {
        double denom = weight_sum[regime];
        double value = denom > 0.0 ? total / denom : 0.0;
        results.oos_sharpe_by_regime[regime] = value;
        regime_values.push_back(value);
    }
    if (regime_values.size() <= 1) {
        results.regime_consistency_score = 0.0;
    } else {
        double mean = 0.0;
        for (double v : regime_values) {
            mean += v;
        }
        mean /= static_cast<double>(regime_values.size());
        double variance = 0.0;
        for (double v : regime_values) {
            double diff = v - mean;
            variance += diff * diff;
        }
        variance /= static_cast<double>(regime_values.size());
        double stddev = std::sqrt(variance);
        results.regime_consistency_score = 1.0 / (1.0 + stddev);
    }
}

}  // namespace regimeflow::walkforward
