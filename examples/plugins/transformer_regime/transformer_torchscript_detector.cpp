#include "transformer_torchscript_detector.h"

#include "regimeflow/common/result.h"

#include <algorithm>
#include <array>

#include <torch/script.h>
#include <torch/torch.h>

namespace transformer_regime {

namespace {

regimeflow::regime::RegimeType idx_to_regime(int idx) {
    switch (idx) {
        case 0: return regimeflow::regime::RegimeType::Bull;
        case 1: return regimeflow::regime::RegimeType::Neutral;
        case 2: return regimeflow::regime::RegimeType::Bear;
        case 3: return regimeflow::regime::RegimeType::Crisis;
        default: return regimeflow::regime::RegimeType::Neutral;
    }
}

}  // namespace

TorchscriptRegimeDetector::TorchscriptRegimeDetector() = default;

void TorchscriptRegimeDetector::configure(const regimeflow::Config& config) {
    if (auto v = config.get_as<std::string>("model_path")) {
        model_path_ = *v;
    }
    if (auto v = config.get_as<int64_t>("feature_window")) {
        window_ = std::max<int64_t>(30, *v);
    }
    if (auto v = config.get_as<int64_t>("feature_dim")) {
        feature_dim_ = std::max<int64_t>(4, *v);
    }
    load_model();
}

void TorchscriptRegimeDetector::load_model() {
    if (model_path_.empty()) {
        ready_ = false;
        return;
    }
    try {
        module_ = std::make_shared<torch::jit::script::Module>(torch::jit::load(model_path_));
        module_->eval();
        ready_ = true;
    } catch (...) {
        ready_ = false;
    }
}

std::vector<std::string> TorchscriptRegimeDetector::state_names() const {
    return {"BULL", "NEUTRAL", "BEAR", "CRISIS"};
}

void TorchscriptRegimeDetector::push_features(const regimeflow::data::Bar& bar) {
    std::vector<float> feat;
    feat.reserve(static_cast<size_t>(feature_dim_));
    feat.push_back(static_cast<float>(bar.close));
    feat.push_back(static_cast<float>(bar.high - bar.low));
    feat.push_back(static_cast<float>(bar.volume));
    feat.push_back(static_cast<float>(bar.open));
    feat.push_back(static_cast<float>(bar.close - bar.open));
    feat.push_back(static_cast<float>(bar.vwap));
    feat.push_back(static_cast<float>(bar.trade_count));
    feat.push_back(1.0f);

    if (feat.size() > static_cast<size_t>(feature_dim_)) {
        feat.resize(static_cast<size_t>(feature_dim_));
    } else {
        while (feat.size() < static_cast<size_t>(feature_dim_)) {
            feat.push_back(0.0f);
        }
    }

    features_.push_back(std::move(feat));
    if (features_.size() > static_cast<size_t>(window_)) {
        features_.erase(features_.begin());
    }
}

regimeflow::regime::RegimeState TorchscriptRegimeDetector::state_for_timestamp(
    const regimeflow::Timestamp& ts) {
    regimeflow::regime::RegimeState state;
    state.timestamp = ts;
    state.state_count = 4;

    if (!ready_ || features_.size() < static_cast<size_t>(window_)) {
        state.regime = regimeflow::regime::RegimeType::Neutral;
        state.confidence = 0.0;
        state.probabilities = {0.0, 1.0, 0.0, 0.0};
        return state;
    }

    auto tensor = torch::zeros({1, window_, feature_dim_});
    for (int64_t i = 0; i < window_; ++i) {
        for (int64_t j = 0; j < feature_dim_; ++j) {
            tensor[0][i][j] = features_[static_cast<size_t>(i)][static_cast<size_t>(j)];
        }
    }

    try {
        torch::NoGradGuard guard;
        auto output = module_->forward({tensor}).toTensor();
        auto probs = torch::softmax(output, 1).squeeze(0);
        std::array<double, 4> p{
            probs[0].item<double>(),
            probs[1].item<double>(),
            probs[2].item<double>(),
            probs[3].item<double>()
        };
        int idx = 0;
        double best = p[0];
        for (int i = 1; i < 4; ++i) {
            if (p[i] > best) {
                best = p[i];
                idx = i;
            }
        }
        state.regime = idx_to_regime(idx);
        state.confidence = best;
        state.probabilities = {p[0], p[1], p[2], p[3]};
    } catch (...) {
        state.regime = regimeflow::regime::RegimeType::Neutral;
        state.confidence = 0.0;
        state.probabilities = {0.0, 1.0, 0.0, 0.0};
    }

    return state;
}

regimeflow::regime::RegimeState TorchscriptRegimeDetector::on_bar(const regimeflow::data::Bar& bar) {
    push_features(bar);
    return state_for_timestamp(bar.timestamp);
}

regimeflow::regime::RegimeState TorchscriptRegimeDetector::on_tick(const regimeflow::data::Tick& tick) {
    regimeflow::data::Bar bar{};
    bar.timestamp = tick.timestamp;
    bar.symbol = tick.symbol;
    bar.open = tick.price;
    bar.high = tick.price;
    bar.low = tick.price;
    bar.close = tick.price;
    bar.volume = static_cast<regimeflow::Volume>(tick.quantity);
    push_features(bar);
    return state_for_timestamp(tick.timestamp);
}

regimeflow::plugins::PluginInfo TransformerTorchscriptPlugin::info() const {
    return {"transformer_torchscript", "0.1.0",
            "Regime detector backed by TorchScript transformer", "RegimeFlow", {}};
}

regimeflow::Result<void> TransformerTorchscriptPlugin::on_initialize(const regimeflow::Config& config) {
    config_ = config;
    return regimeflow::Ok();
}

std::unique_ptr<regimeflow::regime::RegimeDetector> TransformerTorchscriptPlugin::create_detector() {
    auto detector = std::make_unique<TorchscriptRegimeDetector>();
    detector->configure(config_);
    return detector;
}

}  // namespace transformer_regime

extern "C" {

REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new transformer_regime::TransformerTorchscriptPlugin();
}

REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

REGIMEFLOW_EXPORT const char* plugin_type() {
    return "regime_detector";
}

REGIMEFLOW_EXPORT const char* plugin_name() {
    return "transformer_torchscript";
}

REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}

}
