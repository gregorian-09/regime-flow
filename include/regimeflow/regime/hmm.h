/**
 * @file hmm.h
 * @brief RegimeFlow regimeflow hmm declarations.
 */

#pragma once

#include "regimeflow/regime/features.h"
#include "regimeflow/regime/kalman_filter.h"
#include "regimeflow/regime/regime_detector.h"

#include <vector>

namespace regimeflow::regime {

/**
 * @brief Gaussian emission parameters for HMM states.
 */
struct GaussianParams {
    FeatureVector mean;
    FeatureVector variance;
};

/**
 * @brief Hidden Markov Model regime detector.
 */
class HMMRegimeDetector final : public RegimeDetector {
public:
    /**
     * @brief Construct HMM detector.
     * @param states Number of hidden states.
     * @param window Feature window size.
     */
    explicit HMMRegimeDetector(int states = 4, int window = 20);

    /**
     * @brief Update detector with bar data.
     */
    RegimeState on_bar(const data::Bar& bar) override;
    /**
     * @brief Update detector with tick data.
     */
    RegimeState on_tick(const data::Tick& tick) override;
    /**
     * @brief Update detector with order book data.
     */
    RegimeState on_book(const data::OrderBook& book) override;
    /**
     * @brief Train the HMM using feature vectors.
     */
    void train(const std::vector<FeatureVector>& data) override;

    /**
     * @brief Train HMM parameters with Baum-Welch.
     * @param data Feature vectors.
     * @param max_iter Maximum iterations.
     * @param tol Convergence tolerance.
     */
    void baum_welch(const std::vector<FeatureVector>& data, int max_iter = 50,
                    double tol = 1e-4);
    /**
     * @brief Compute log-likelihood for a dataset.
     * @param data Feature vectors.
     * @return Log-likelihood.
     */
    double log_likelihood(const std::vector<FeatureVector>& data) const;

    /**
     * @brief Set transition probabilities.
     * @param matrix Transition matrix.
     */
    void set_transition_matrix(const std::vector<std::vector<double>>& matrix);
    /**
     * @brief Set emission parameters.
     * @param params Emission Gaussian params.
     */
    void set_emission_params(std::vector<GaussianParams> params);
    /**
     * @brief Set feature list.
     * @param features Feature types.
     */
    void set_features(std::vector<FeatureType> features);
    /**
     * @brief Enable/disable feature normalization.
     * @param normalize True to normalize.
     */
    void set_normalize_features(bool normalize);
    /**
     * @brief Set normalization mode.
     * @param mode Normalization mode.
     */
    void set_normalization_mode(NormalizationMode mode);
    /**
     * @brief Save model to disk.
     */
    void save(const std::string& path) const override;
    /**
     * @brief Load model from disk.
     */
    void load(const std::string& path) override;
    /**
     * @brief Configure model parameters.
     */
    void configure(const Config& config) override;
    /**
     * @brief Number of hidden states.
     */
    int num_states() const override { return states_; }
    /**
     * @brief State names for display.
     */
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
