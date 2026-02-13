#include "transformer_regime_detector.h"

#include "regimeflow/common/result.h"
#include "regimeflow/common/time.h"
#include "regimeflow/plugins/registry.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace transformer_regime {

namespace {

regimeflow::regime::RegimeType parse_regime(const std::string& value) {
    if (value == "bull") return regimeflow::regime::RegimeType::Bull;
    if (value == "bear") return regimeflow::regime::RegimeType::Bear;
    if (value == "crisis") return regimeflow::regime::RegimeType::Crisis;
    return regimeflow::regime::RegimeType::Neutral;
}

std::vector<std::string> split_line(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        fields.push_back(item);
    }
    return fields;
}

}  // namespace

CsvTransformerDetector::CsvTransformerDetector(std::string path)
    : path_(std::move(path)) {
    load_csv();
}

void CsvTransformerDetector::configure(const regimeflow::Config& config) {
    if (auto v = config.get_as<std::string>("signals_path")) {
        path_ = *v;
        load_csv();
    }
}

std::vector<std::string> CsvTransformerDetector::state_names() const {
    return {"BULL", "NEUTRAL", "BEAR", "CRISIS"};
}

regimeflow::regime::RegimeState CsvTransformerDetector::on_bar(const regimeflow::data::Bar& bar) {
    return state_for_timestamp(bar.timestamp);
}

regimeflow::regime::RegimeState CsvTransformerDetector::on_tick(const regimeflow::data::Tick& tick) {
    return state_for_timestamp(tick.timestamp);
}

void CsvTransformerDetector::load_csv() {
    rows_.clear();
    cursor_ = 0;

    std::ifstream in(path_);
    if (!in) {
        return;
    }
    std::string line;
    bool header = true;
    while (std::getline(in, line)) {
        if (header) {
            header = false;
            continue;
        }
        if (line.empty()) continue;
        auto fields = split_line(line);
        if (fields.size() < 7) continue;

        RegimeRow row;
        row.timestamp = regimeflow::Timestamp::from_string(fields[0], "%Y-%m-%d %H:%M:%S");
        row.regime = parse_regime(fields[1]);
        row.confidence = std::stod(fields[2]);
        row.probs = {std::stod(fields[3]), std::stod(fields[4]), std::stod(fields[5]), std::stod(fields[6])};
        rows_.push_back(row);
    }

    std::sort(rows_.begin(), rows_.end(),
              [](const RegimeRow& a, const RegimeRow& b) { return a.timestamp < b.timestamp; });
}

regimeflow::regime::RegimeState CsvTransformerDetector::state_for_timestamp(
    const regimeflow::Timestamp& ts) const {
    regimeflow::regime::RegimeState state;
    if (rows_.empty()) {
        state.timestamp = ts;
        state.state_count = 4;
        state.regime = regimeflow::regime::RegimeType::Neutral;
        state.confidence = 0.0;
        state.probabilities = {0.0, 1.0, 0.0, 0.0};
        return state;
    }

    size_t idx = cursor_;
    if (idx >= rows_.size()) idx = rows_.size() - 1;
    while (idx + 1 < rows_.size() && rows_[idx + 1].timestamp <= ts) {
        ++idx;
    }

    const auto& row = rows_[idx];
    state.timestamp = ts;
    state.state_count = 4;
    state.regime = row.regime;
    state.confidence = row.confidence;
    state.probabilities = {row.probs[0], row.probs[1], row.probs[2], row.probs[3]};
    return state;
}

regimeflow::plugins::PluginInfo TransformerRegimePlugin::info() const {
    return {"transformer_regime", "0.1.0",
            "Regime detector reading transformer signals from CSV", "RegimeFlow", {}};
}

regimeflow::Result<void> TransformerRegimePlugin::on_initialize(const regimeflow::Config& config) {
    config_ = config;
    return regimeflow::Ok();
}

std::unique_ptr<regimeflow::regime::RegimeDetector> TransformerRegimePlugin::create_detector() {
    auto path = config_.get_as<std::string>("signals_path").value_or("examples/python_transformer_regime/regime_signals.csv");
    auto detector = std::make_unique<CsvTransformerDetector>(path);
    detector->configure(config_);
    return detector;
}

}  // namespace transformer_regime

extern "C" {

REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new transformer_regime::TransformerRegimePlugin();
}

REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

REGIMEFLOW_EXPORT const char* plugin_type() {
    return "regime_detector";
}

REGIMEFLOW_EXPORT const char* plugin_name() {
    return "transformer_regime";
}

REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}

}
