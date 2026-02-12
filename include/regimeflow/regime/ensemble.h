/**
 * @file ensemble.h
 * @brief RegimeFlow regimeflow ensemble declarations.
 */

#pragma once

#include "regimeflow/regime/regime_detector.h"

#include <memory>
#include <vector>

namespace regimeflow::regime {

/**
 * @brief Ensemble detector combining multiple detectors.
 */
class EnsembleRegimeDetector final : public RegimeDetector {
public:
    /**
     * @brief Ensemble voting method.
     */
    enum class VotingMethod {
        WeightedAverage,
        Majority,
        ConfidenceWeighted,
        Bayesian
    };

    /**
     * @brief Add a detector to the ensemble.
     * @param detector Detector instance.
     * @param weight Weight for voting.
     */
    void add_detector(std::unique_ptr<RegimeDetector> detector, double weight = 1.0);
    /**
     * @brief Set the voting method.
     * @param method Voting method.
     */
    void set_voting_method(VotingMethod method) { voting_method_ = method; }
    /**
     * @brief Save ensemble configuration and models.
     */
    void save(const std::string& path) const override;
    /**
     * @brief Load ensemble configuration and models.
     */
    void load(const std::string& path) override;
    /**
     * @brief Configure the ensemble.
     */
    void configure(const Config& config) override;
    /**
     * @brief Number of states across detectors.
     */
    int num_states() const override;
    /**
     * @brief State names across detectors.
     */
    std::vector<std::string> state_names() const override;

    /**
     * @brief Update ensemble with bar input.
     */
    RegimeState on_bar(const data::Bar& bar) override;
    /**
     * @brief Update ensemble with tick input.
     */
    RegimeState on_tick(const data::Tick& tick) override;
    /**
     * @brief Update ensemble with order book input.
     */
    RegimeState on_book(const data::OrderBook& book) override;
    /**
     * @brief Train all detectors with shared features.
     */
    void train(const std::vector<FeatureVector>& data) override;

private:
    RegimeState combine(const std::vector<RegimeState>& states, Timestamp timestamp) const;

    std::vector<std::unique_ptr<RegimeDetector>> detectors_;
    std::vector<double> weights_;
    VotingMethod voting_method_ = VotingMethod::WeightedAverage;
};

}  // namespace regimeflow::regime
