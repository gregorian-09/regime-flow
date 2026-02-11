#include "regimeflow/regime/constant_detector.h"

#include <fstream>

namespace regimeflow::regime {

RegimeState ConstantRegimeDetector::on_bar(const data::Bar& bar) {
    RegimeState state;
    state.regime = regime_;
    state.confidence = 1.0;
    state.timestamp = bar.timestamp;
    state.probabilities = {0, 0, 0, 0};
    state.state_count = 4;
    state.probabilities_all.assign(4, 0.0);
    auto idx = static_cast<size_t>(std::min(static_cast<int>(regime_), 3));
    state.probabilities[idx] = 1.0;
    state.probabilities_all[idx] = 1.0;
    return state;
}

RegimeState ConstantRegimeDetector::on_tick(const data::Tick& tick) {
    RegimeState state;
    state.regime = regime_;
    state.confidence = 1.0;
    state.timestamp = tick.timestamp;
    state.probabilities = {0, 0, 0, 0};
    state.state_count = 4;
    state.probabilities_all.assign(4, 0.0);
    auto idx = static_cast<size_t>(std::min(static_cast<int>(regime_), 3));
    state.probabilities[idx] = 1.0;
    state.probabilities_all[idx] = 1.0;
    return state;
}

void ConstantRegimeDetector::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return;
    }
    out << static_cast<int>(regime_) << "\n";
}

void ConstantRegimeDetector::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return;
    }
    int value = 0;
    if (in >> value) {
        regime_ = static_cast<RegimeType>(value);
    }
}

void ConstantRegimeDetector::configure(const Config& config) {
    auto name = config.get_as<std::string>("regime").value_or("neutral");
    if (name == "bull") {
        regime_ = RegimeType::Bull;
    } else if (name == "bear") {
        regime_ = RegimeType::Bear;
    } else if (name == "crisis") {
        regime_ = RegimeType::Crisis;
    } else {
        regime_ = RegimeType::Neutral;
    }
}

}  // namespace regimeflow::regime
