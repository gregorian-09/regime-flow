// Example custom regime detector plugin.

#pragma once

#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/regime/regime_detector.h"

#include <deque>
#include <string>

namespace custom_regime {

class CustomFeatureBuilder {
public:
    explicit CustomFeatureBuilder(int window = 60);

    // Update rolling statistics and return feature vector:
    // [trend_20, vol_20, drawdown_60, custom_skew]
    regimeflow::regime::FeatureVector on_bar(const regimeflow::data::Bar& bar);

private:
    double compute_volatility() const;
    double compute_drawdown() const;
    double compute_skew() const;

    int window_ = 60;
    double last_close_ = 0.0;
    double peak_ = 0.0;
    std::deque<double> returns_;
};

class CustomRegimeDetector final : public regimeflow::regime::RegimeDetector {
public:
    CustomRegimeDetector();

    regimeflow::regime::RegimeState on_bar(const regimeflow::data::Bar& bar) override;
    regimeflow::regime::RegimeState on_tick(const regimeflow::data::Tick& tick) override;
    void configure(const regimeflow::Config& config) override;
    int num_states() const override { return 3; }
    std::vector<std::string> state_names() const override;

private:
    CustomFeatureBuilder features_;
    double trend_threshold_ = 0.02;
    double vol_threshold_ = 0.015;
};

class CustomRegimeDetectorPlugin final : public regimeflow::plugins::RegimeDetectorPlugin {
public:
    regimeflow::plugins::PluginInfo info() const override;
    regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override;
    std::unique_ptr<regimeflow::regime::RegimeDetector> create_detector() override;

private:
    regimeflow::Config config_;
};

}  // namespace custom_regime
