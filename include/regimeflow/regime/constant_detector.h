#pragma once

#include "regimeflow/regime/regime_detector.h"

namespace regimeflow::regime {

class ConstantRegimeDetector final : public RegimeDetector {
public:
    explicit ConstantRegimeDetector(RegimeType regime) : regime_(regime) {}

    RegimeState on_bar(const data::Bar& bar) override;
    RegimeState on_tick(const data::Tick& tick) override;
    void save(const std::string& path) const override;
    void load(const std::string& path) override;
    void configure(const Config& config) override;
    int num_states() const override { return 1; }
    std::vector<std::string> state_names() const override { return {"Constant"}; }

private:
    RegimeType regime_;
};

}  // namespace regimeflow::regime
