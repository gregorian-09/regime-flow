#pragma once

#include "regimeflow/regime/regime_detector.h"

#include <memory>
#include <vector>

namespace regimeflow::regime {

class EnsembleRegimeDetector final : public RegimeDetector {
public:
    enum class VotingMethod {
        WeightedAverage,
        Majority,
        ConfidenceWeighted,
        Bayesian
    };

    void add_detector(std::unique_ptr<RegimeDetector> detector, double weight = 1.0);
    void set_voting_method(VotingMethod method) { voting_method_ = method; }
    void save(const std::string& path) const override;
    void load(const std::string& path) override;
    void configure(const Config& config) override;
    int num_states() const override;
    std::vector<std::string> state_names() const override;

    RegimeState on_bar(const data::Bar& bar) override;
    RegimeState on_tick(const data::Tick& tick) override;
    RegimeState on_book(const data::OrderBook& book) override;
    void train(const std::vector<FeatureVector>& data) override;

private:
    RegimeState combine(const std::vector<RegimeState>& states, Timestamp timestamp) const;

    std::vector<std::unique_ptr<RegimeDetector>> detectors_;
    std::vector<double> weights_;
    VotingMethod voting_method_ = VotingMethod::WeightedAverage;
};

}  // namespace regimeflow::regime
