#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/engine/backtest_results.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/regime/types.h"
#include "regimeflow/strategy/strategy.h"
#include "regimeflow/data/bar.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <variant>
#include <vector>

namespace regimeflow::walkforward {

struct WalkForwardConfig {
    enum class WindowType { Rolling, Anchored, RegimeAware };
    WindowType window_type = WindowType::Rolling;

    Duration in_sample_period = Duration::months(12);
    Duration out_of_sample_period = Duration::months(3);
    Duration step_size = Duration::months(3);

    enum class OptMethod { Grid, Random, Bayesian };
    OptMethod optimization_method = OptMethod::Grid;
    int max_trials = 100;

    std::string fitness_metric = "sharpe";
    bool maximize = true;

    bool retrain_regime_each_window = true;
    bool optimize_per_regime = false;
    bool disable_default_regime_training = false;

    int num_parallel_backtests = -1;

    bool enable_overfitting_detection = true;
    double max_is_oos_ratio = 2.0;

    double initial_capital = 0.0;
    data::BarType bar_type = data::BarType::Time_1Day;
    double periods_per_year = 252.0;
};

struct ParameterDef {
    std::string name;
    enum class Type { Int, Double, Categorical };
    Type type = Type::Double;

    double min_value = 0;
    double max_value = 0;
    double step = 0;

    std::vector<std::variant<int, double, std::string>> categories;

    enum class Distribution { Uniform, LogUniform, Normal };
    Distribution distribution = Distribution::Uniform;
};

using ParameterSet = std::map<std::string, std::variant<int, double, std::string>>;

struct WindowResult {
    TimeRange in_sample_range;
    TimeRange out_of_sample_range;

    ParameterSet optimal_params;
    double is_fitness = 0.0;
    engine::BacktestResults is_results;

    double oos_fitness = 0.0;
    engine::BacktestResults oos_results;

    std::map<regime::RegimeType, double> regime_distribution;

    double efficiency_ratio = 0.0;
};

struct WalkForwardResults {
    std::vector<WindowResult> windows;
    engine::BacktestResults stitched_oos_results;

    std::map<std::string, std::vector<double>> param_evolution;
    std::map<std::string, double> param_stability_score;

    double avg_is_sharpe = 0.0;
    double avg_oos_sharpe = 0.0;
    double overall_oos_sharpe = 0.0;

    double avg_efficiency_ratio = 0.0;
    bool potential_overfit = false;
    std::string overfit_diagnosis;

    std::map<regime::RegimeType, double> oos_sharpe_by_regime;
    double regime_consistency_score = 0.0;
};

class WalkForwardOptimizer {
public:
    struct RegimeTrainingContext {
        data::DataSource* data_source = nullptr;
        TimeRange training_range;
        data::BarType bar_type = data::BarType::Time_1Day;
        std::vector<SymbolId> symbols;
        regime::RegimeDetector* detector = nullptr;
    };

    using RegimeTrainingHook = std::function<bool(const RegimeTrainingContext&)>;
    using RegimeTrainingCallback = std::function<void(const RegimeTrainingContext&)>;

    explicit WalkForwardOptimizer(const WalkForwardConfig& config);

    WalkForwardResults optimize(
        const std::vector<ParameterDef>& params,
        std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)> strategy_factory,
        data::DataSource* data_source,
        const TimeRange& full_range,
        std::function<std::unique_ptr<regime::RegimeDetector>()> detector_factory = {});

    void on_window_complete(std::function<void(const WindowResult&)> callback);
    void on_trial_complete(std::function<void(const ParameterSet&, double)> callback);
    void on_regime_train(RegimeTrainingHook callback);
    void on_regime_trained(RegimeTrainingCallback callback);

    void cancel();

private:
    struct TrialOutcome {
        double fitness = 0.0;
        engine::BacktestResults results;
    };

    std::vector<std::pair<TimeRange, TimeRange>> generate_windows(TimeRange full_range) const;
    std::vector<std::pair<TimeRange, TimeRange>> generate_regime_windows(
        data::DataSource* data_source,
        std::function<std::unique_ptr<regime::RegimeDetector>()> detector_factory,
        const TimeRange& full_range) const;
    double compute_fitness(const engine::BacktestResults& results) const;
    std::vector<ParameterSet> build_grid(const std::vector<ParameterDef>& params) const;
    std::vector<ParameterSet> build_random_trials(const std::vector<ParameterDef>& params,
                                                  int max_trials,
                                                  std::mt19937& rng) const;
    std::vector<TrialOutcome> evaluate_trials(
        const std::vector<ParameterSet>& trials,
        std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)>& factory,
        data::DataSource* data,
        const TimeRange& range,
        const std::function<std::unique_ptr<regime::RegimeDetector>()>& detector_factory,
        bool attach_detector) const;
    engine::BacktestResults run_backtest(
        const ParameterSet& params,
        std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)>& factory,
        data::DataSource* data,
        const TimeRange& range,
        const TimeRange& training_range,
        const std::function<std::unique_ptr<regime::RegimeDetector>()>& detector_factory,
        bool attach_detector) const;
    std::map<regime::RegimeType, double> extract_regime_distribution(
        const engine::BacktestResults& results) const;
    engine::BacktestResults stitch_oos_results(const std::vector<WindowResult>& windows) const;
    void analyze_overfitting(WalkForwardResults& results) const;
    void analyze_param_stability(WalkForwardResults& results) const;
    void analyze_oos_performance(WalkForwardResults& results) const;

    WalkForwardConfig config_;
    std::vector<std::function<void(const WindowResult&)>> window_callbacks_;
    std::vector<std::function<void(const ParameterSet&, double)>> trial_callbacks_;
    std::vector<RegimeTrainingHook> regime_train_hooks_;
    std::vector<RegimeTrainingCallback> regime_trained_callbacks_;
    std::atomic<bool> cancelled_{false};
};

}  // namespace regimeflow::walkforward
