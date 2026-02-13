// Example custom regime detector plugin.

#include "custom_regime_detector.h"

#include "regimeflow/common/result.h"
#include "regimeflow/common/time.h"
#include "regimeflow/plugins/registry.h"

#include <cmath>

namespace custom_regime {

CustomFeatureBuilder::CustomFeatureBuilder(int window) : window_(window) {}

regimeflow::regime::FeatureVector CustomFeatureBuilder::on_bar(
    const regimeflow::data::Bar& bar) {
    if (last_close_ > 0.0) {
        double ret = (bar.close - last_close_) / last_close_;
        returns_.push_back(ret);
        if (static_cast<int>(returns_.size()) > window_) {
            returns_.pop_front();
        }
    }
    last_close_ = bar.close;
    if (bar.close > peak_) {
        peak_ = bar.close;
    }
    double trend_20 = returns_.empty() ? 0.0 : returns_.back();
    double vol_20 = compute_volatility();
    double drawdown_60 = compute_drawdown();
    double custom_skew = compute_skew();
    return {trend_20, vol_20, drawdown_60, custom_skew};
}

double CustomFeatureBuilder::compute_volatility() const {
    if (returns_.empty()) return 0.0;
    double mean = 0.0;
    for (double r : returns_) mean += r;
    mean /= static_cast<double>(returns_.size());
    double var = 0.0;
    for (double r : returns_) {
        double d = r - mean;
        var += d * d;
    }
    var /= static_cast<double>(returns_.size());
    return std::sqrt(var);
}

double CustomFeatureBuilder::compute_drawdown() const {
    if (peak_ <= 0.0 || last_close_ <= 0.0) return 0.0;
    return (peak_ - last_close_) / peak_;
}

double CustomFeatureBuilder::compute_skew() const {
    if (returns_.size() < 3) return 0.0;
    double mean = 0.0;
    for (double r : returns_) mean += r;
    mean /= static_cast<double>(returns_.size());
    double m2 = 0.0;
    double m3 = 0.0;
    for (double r : returns_) {
        double d = r - mean;
        m2 += d * d;
        m3 += d * d * d;
    }
    if (m2 == 0.0) return 0.0;
    double variance = m2 / static_cast<double>(returns_.size());
    double stddev = std::sqrt(variance);
    return (m3 / static_cast<double>(returns_.size())) / (stddev * stddev * stddev);
}

CustomRegimeDetector::CustomRegimeDetector() : features_(60) {}

regimeflow::regime::RegimeState CustomRegimeDetector::on_bar(
    const regimeflow::data::Bar& bar) {
    auto features = features_.on_bar(bar);
    const double trend = features[0];
    const double vol = features[1];
    const double dd = features[2];

    regimeflow::regime::RegimeState state;
    state.timestamp = bar.timestamp;
    state.state_count = 3;
    if (dd > vol_threshold_) {
        state.regime = regimeflow::regime::RegimeType::Crisis;
        state.confidence = 0.8;
        state.probabilities = {0.05, 0.15, 0.8, 0.0};
    } else if (trend > trend_threshold_) {
        state.regime = regimeflow::regime::RegimeType::Bull;
        state.confidence = 0.7;
        state.probabilities = {0.7, 0.2, 0.1, 0.0};
    } else if (trend < -trend_threshold_) {
        state.regime = regimeflow::regime::RegimeType::Bear;
        state.confidence = 0.7;
        state.probabilities = {0.1, 0.2, 0.7, 0.0};
    } else {
        state.regime = regimeflow::regime::RegimeType::Neutral;
        state.confidence = 0.6;
        state.probabilities = {0.2, 0.6, 0.2, 0.0};
    }
    return state;
}

regimeflow::regime::RegimeState CustomRegimeDetector::on_tick(
    const regimeflow::data::Tick& tick) {
    regimeflow::data::Bar bar{};
    bar.timestamp = tick.timestamp;
    bar.symbol = tick.symbol;
    bar.open = tick.price;
    bar.high = tick.price;
    bar.low = tick.price;
    bar.close = tick.price;
    bar.volume = static_cast<regimeflow::Volume>(tick.quantity);
    return on_bar(bar);
}

void CustomRegimeDetector::configure(const regimeflow::Config& config) {
    if (auto v = config.get_as<int64_t>("window")) {
        features_ = CustomFeatureBuilder(static_cast<int>(*v));
    }
    if (auto v = config.get_as<double>("trend_threshold")) {
        trend_threshold_ = *v;
    }
    if (auto v = config.get_as<double>("vol_threshold")) {
        vol_threshold_ = *v;
    }
}

std::vector<std::string> CustomRegimeDetector::state_names() const {
    return {"BULL", "NEUTRAL", "BEAR", "CRISIS"};
}

regimeflow::plugins::PluginInfo CustomRegimeDetectorPlugin::info() const {
    return {"custom_regime", "0.1.0",
            "Custom regime detector with bespoke features", "RegimeFlow", {}};
}

regimeflow::Result<void> CustomRegimeDetectorPlugin::on_initialize(
    const regimeflow::Config& config) {
    config_ = config;
    return regimeflow::Ok();
}

std::unique_ptr<regimeflow::regime::RegimeDetector>
CustomRegimeDetectorPlugin::create_detector() {
    auto detector = std::make_unique<CustomRegimeDetector>();
    detector->configure(config_);
    return detector;
}

}  // namespace custom_regime

extern "C" {

REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new custom_regime::CustomRegimeDetectorPlugin();
}

REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

REGIMEFLOW_EXPORT const char* plugin_type() {
    return "regime_detector";
}

REGIMEFLOW_EXPORT const char* plugin_name() {
    return "custom_regime";
}

REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}

}  // extern "C"
