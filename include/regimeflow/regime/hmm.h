#pragma once

#include "regimeflow/regime/features.h"
#include "regimeflow/regime/kalman_filter.h"
#include "regimeflow/regime/regime_detector.h"

#include <vector>

namespace regimeflow::regime {

struct GaussianParams {
    FeatureVector mean;
    FeatureVector variance;
};

class HMMRegimeDetector final : public RegimeDetector {
public:
    explicit HMMRegimeDetector(int states = 4, int window = 20);

    RegimeState on_bar(const data::Bar& bar) override;
    RegimeState on_tick(const data::Tick& tick) override;
    RegimeState on_book(const data::OrderBook& book) override;
    void train(const std::vector<FeatureVector>& data) override;

    void baum_welch(const std::vector<FeatureVector>& data, int max_iter = 50,
                    double tol = 1e-4);
    double log_likelihood(const std::vector<FeatureVector>& data) const;

    void set_transition_matrix(const std::vector<std::vector<double>>& matrix);
    void set_emission_params(std::vector<GaussianParams> params);
    void set_features(std::vector<FeatureType> features);
    void set_normalize_features(bool normalize);
    void set_normalization_mode(NormalizationMode mode);
    void save(const std::string& path) const override;
    void load(const std::string& path) override;
    void configure(const Config& config) override;
    int num_states() const override { return states_; }
    std::vector<std::string> state_names() const override;

private:
    FeatureVector extract_features(const data::Bar& bar);
    FeatureVector extract_features(const data::Tick& tick);
    RegimeState detect(const FeatureVector& features, Timestamp timestamp);

    void initialize_from_data(const std::vector<FeatureVector>& data);
    std::vector<std::vector<double>> forward_log(const std::vector<FeatureVector>& data) const;
    std::vector<std::vector<double>> backward_log(const std::vector<FeatureVector>& data) const;

    int states_ = 4;
    int window_ = 20;
    std::vector<std::vector<double>> transition_;
    std::vector<GaussianParams> emissions_;
    std::vector<double> probabilities_;
    std::vector<double> initial_;
    bool enable_kalman_ = false;
    double kalman_process_noise_ = 1e-3;
    double kalman_measurement_noise_ = 1e-2;
    std::vector<KalmanFilter1D> kalman_filters_;
    FeatureExtractor extractor_;
};

}  // namespace regimeflow::regime
