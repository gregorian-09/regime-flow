#pragma once

#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/plugins/interfaces.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace transformer_regime {

struct RegimeRow {
    regimeflow::Timestamp timestamp;
    regimeflow::regime::RegimeType regime = regimeflow::regime::RegimeType::Neutral;
    double confidence = 0.0;
    std::array<double, 4> probs{0.0, 0.0, 0.0, 0.0};
};

class CsvTransformerDetector final : public regimeflow::regime::RegimeDetector {
public:
    explicit CsvTransformerDetector(std::string path);

    regimeflow::regime::RegimeState on_bar(const regimeflow::data::Bar& bar) override;
    regimeflow::regime::RegimeState on_tick(const regimeflow::data::Tick& tick) override;
    void configure(const regimeflow::Config& config) override;
    std::vector<std::string> state_names() const override;

private:
    void load_csv();
    regimeflow::regime::RegimeState state_for_timestamp(const regimeflow::Timestamp& ts) const;

    std::string path_;
    std::vector<RegimeRow> rows_;
    size_t cursor_ = 0;
};

class TransformerRegimePlugin final : public regimeflow::plugins::RegimeDetectorPlugin {
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
