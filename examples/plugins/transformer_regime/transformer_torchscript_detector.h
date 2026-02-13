#pragma once

#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/plugins/interfaces.h"

#include <memory>
#include <string>
#include <vector>

#include <torch/script.h>

namespace transformer_regime {

class TorchscriptRegimeDetector final : public regimeflow::regime::RegimeDetector {
public:
    TorchscriptRegimeDetector();

    regimeflow::regime::RegimeState on_bar(const regimeflow::data::Bar& bar) override;
    regimeflow::regime::RegimeState on_tick(const regimeflow::data::Tick& tick) override;
    void configure(const regimeflow::Config& config) override;
    std::vector<std::string> state_names() const override;

private:
    void load_model();
    regimeflow::regime::RegimeState state_for_timestamp(const regimeflow::Timestamp& ts);
    void push_features(const regimeflow::data::Bar& bar);

    std::string model_path_;
    int64_t window_ = 120;
    int64_t feature_dim_ = 8;

    std::vector<std::vector<float>> features_;

    bool ready_ = false;
    std::shared_ptr<torch::jit::script::Module> module_;
};

class TransformerTorchscriptPlugin final : public regimeflow::plugins::RegimeDetectorPlugin {
public:
    regimeflow::plugins::PluginInfo info() const override;
    regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override;
    std::unique_ptr<regimeflow::regime::RegimeDetector> create_detector() override;

private:
    regimeflow::Config config_;
};

}  // namespace transformer_regime

extern "C" {

REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin();
REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin);
REGIMEFLOW_EXPORT const char* plugin_type();
REGIMEFLOW_EXPORT const char* plugin_name();
REGIMEFLOW_EXPORT const char* regimeflow_abi_version();

}
