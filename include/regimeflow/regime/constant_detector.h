/**
 * @file constant_detector.h
 * @brief RegimeFlow regimeflow constant detector declarations.
 */

#pragma once

#include "regimeflow/regime/regime_detector.h"

namespace regimeflow::regime {

/**
 * @brief Regime detector that always returns a fixed regime.
 */
class ConstantRegimeDetector final : public RegimeDetector {
public:
    /**
     * @brief Construct with a fixed regime.
     * @param regime Regime to always report.
     */
    explicit ConstantRegimeDetector(RegimeType regime) : regime_(regime) {}

    /**
     * @brief Return the fixed regime for bar input.
     */
    RegimeState on_bar(const data::Bar& bar) override;
    /**
     * @brief Return the fixed regime for tick input.
     */
    RegimeState on_tick(const data::Tick& tick) override;
    /**
     * @brief Save the detector state.
     */
    void save(const std::string& path) const override;
    /**
     * @brief Load the detector state.
     */
    void load(const std::string& path) override;
    /**
     * @brief Configure the detector.
     */
    void configure(const Config& config) override;
    /**
     * @brief Number of states (always 1).
     */
    int num_states() const override { return 1; }
    /**
     * @brief State names.
     */
    std::vector<std::string> state_names() const override { return {"Constant"}; }

private:
    RegimeType regime_;
};

}  // namespace regimeflow::regime
