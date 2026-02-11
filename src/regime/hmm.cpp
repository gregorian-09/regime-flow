#include "regimeflow/regime/hmm.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>

namespace regimeflow::regime {

namespace {

double log_sum_exp(const std::vector<double>& values) {
    if (values.empty()) {
        return -std::numeric_limits<double>::infinity();
    }
    double maxv = *std::max_element(values.begin(), values.end());
    double sum = 0.0;
    for (double v : values) {
        sum += std::exp(v - maxv);
    }
    return maxv + std::log(sum);
}

double log_gaussian(const FeatureVector& x, const GaussianParams& params) {
    constexpr double kTwoPi = 6.2831853071795864769;
    double logp = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        double mean = params.mean[i];
        double var = params.variance[i];
        if (var <= 0) {
            var = 1e-6;
        }
        double diff = x[i] - mean;
        logp += -0.5 * (std::log(kTwoPi * var) + (diff * diff) / var);
    }
    return logp;
}

std::vector<double> softmax(const std::vector<double>& logp) {
    double maxv = *std::max_element(logp.begin(), logp.end());
    std::vector<double> exps(logp.size());
    double sum = 0.0;
    for (size_t i = 0; i < logp.size(); ++i) {
        exps[i] = std::exp(logp[i] - maxv);
        sum += exps[i];
    }
    if (sum == 0) {
        return std::vector<double>(logp.size(), 1.0 / logp.size());
    }
    for (double& v : exps) {
        v /= sum;
    }
    return exps;
}

}  // namespace

HMMRegimeDetector::HMMRegimeDetector(int states, int window)
    : states_(states), window_(window), extractor_(window) {
    transition_.assign(states_, std::vector<double>(states_, 1.0 / states_));
    emissions_.resize(states_);
    for (int i = 0; i < states_; ++i) {
        emissions_[i].mean = {0.0, 0.01};
        emissions_[i].variance = {1e-6, 1e-4};
    }
    probabilities_.assign(states_, 1.0 / states_);
    initial_.assign(states_, 1.0 / states_);
    kalman_filters_.assign(states_, KalmanFilter1D(kalman_process_noise_, kalman_measurement_noise_));
}

RegimeState HMMRegimeDetector::on_bar(const data::Bar& bar) {
    return detect(extract_features(bar), bar.timestamp);
}

RegimeState HMMRegimeDetector::on_tick(const data::Tick& tick) {
    return detect(extract_features(tick), tick.timestamp);
}

RegimeState HMMRegimeDetector::on_book(const data::OrderBook& book) {
    return detect(extractor_.on_book(book), book.timestamp);
}

void HMMRegimeDetector::train(const std::vector<FeatureVector>& data) {
    baum_welch(data, 50, 1e-4);
}

void HMMRegimeDetector::baum_welch(const std::vector<FeatureVector>& data, int max_iter,
                                   double tol) {
    if (data.empty()) {
        return;
    }
    initialize_from_data(data);

    double prev_ll = -std::numeric_limits<double>::infinity();
    for (int iter = 0; iter < max_iter; ++iter) {
        auto log_alpha = forward_log(data);
        auto log_beta = backward_log(data);

        const int T = static_cast<int>(data.size());
        std::vector<std::vector<double>> gamma(T, std::vector<double>(states_, 0.0));
        std::vector<std::vector<std::vector<double>>> xi(
            T - 1, std::vector<std::vector<double>>(states_, std::vector<double>(states_, 0.0)));

        for (int t = 0; t < T; ++t) {
            std::vector<double> log_gamma(states_, 0.0);
            for (int i = 0; i < states_; ++i) {
                log_gamma[i] = log_alpha[t][i] + log_beta[t][i];
            }
            double log_norm = log_sum_exp(log_gamma);
            for (int i = 0; i < states_; ++i) {
                gamma[t][i] = std::exp(log_gamma[i] - log_norm);
            }
        }

        for (int t = 0; t < T - 1; ++t) {
            std::vector<double> log_xi_flat;
            log_xi_flat.reserve(states_ * states_);
            for (int i = 0; i < states_; ++i) {
                for (int j = 0; j < states_; ++j) {
                    double val = log_alpha[t][i]
                                 + std::log(std::max(transition_[i][j], 1e-12))
                                 + log_gaussian(data[t + 1], emissions_[j])
                                 + log_beta[t + 1][j];
                    log_xi_flat.push_back(val);
                }
            }
            double log_norm = log_sum_exp(log_xi_flat);
            size_t idx = 0;
            for (int i = 0; i < states_; ++i) {
                for (int j = 0; j < states_; ++j) {
                    xi[t][i][j] = std::exp(log_xi_flat[idx++] - log_norm);
                }
            }
        }

        for (int i = 0; i < states_; ++i) {
            initial_[i] = gamma[0][i];
        }

        for (int i = 0; i < states_; ++i) {
            double denom = 0.0;
            for (int t = 0; t < T - 1; ++t) {
                denom += gamma[t][i];
            }
            for (int j = 0; j < states_; ++j) {
                double numer = 0.0;
                for (int t = 0; t < T - 1; ++t) {
                    numer += xi[t][i][j];
                }
                double value = denom > 0.0 ? numer / denom : (1.0 / states_);
                transition_[i][j] = std::max(value, 1e-6);
            }
            double row_sum = 0.0;
            for (int j = 0; j < states_; ++j) {
                row_sum += transition_[i][j];
            }
            for (int j = 0; j < states_; ++j) {
                transition_[i][j] /= row_sum;
            }
        }

        const int dim = static_cast<int>(data.front().size());
        for (int i = 0; i < states_; ++i) {
            emissions_[i].mean.assign(dim, 0.0);
            emissions_[i].variance.assign(dim, 0.0);
        }

        std::vector<double> gamma_sum(states_, 0.0);
        for (int t = 0; t < T; ++t) {
            for (int i = 0; i < states_; ++i) {
                gamma_sum[i] += gamma[t][i];
                for (int d = 0; d < dim; ++d) {
                    emissions_[i].mean[d] += gamma[t][i] * data[t][d];
                }
            }
        }
        for (int i = 0; i < states_; ++i) {
            if (gamma_sum[i] == 0.0) {
                gamma_sum[i] = 1.0;
            }
            for (int d = 0; d < dim; ++d) {
                emissions_[i].mean[d] /= gamma_sum[i];
            }
        }
        for (int t = 0; t < T; ++t) {
            for (int i = 0; i < states_; ++i) {
                for (int d = 0; d < dim; ++d) {
                    double diff = data[t][d] - emissions_[i].mean[d];
                    emissions_[i].variance[d] += gamma[t][i] * diff * diff;
                }
            }
        }
        for (int i = 0; i < states_; ++i) {
            for (int d = 0; d < dim; ++d) {
                emissions_[i].variance[d] /= gamma_sum[i];
                emissions_[i].variance[d] = std::max(emissions_[i].variance[d], 1e-6);
            }
        }

        double ll = log_likelihood(data);
        if (iter > 0 && std::abs(ll - prev_ll) < tol) {
            break;
        }
        prev_ll = ll;
    }
}

double HMMRegimeDetector::log_likelihood(const std::vector<FeatureVector>& data) const {
    if (data.empty()) {
        return 0.0;
    }
    auto log_alpha = forward_log(data);
    return log_sum_exp(log_alpha.back());
}

void HMMRegimeDetector::set_transition_matrix(const std::vector<std::vector<double>>& matrix) {
    transition_ = matrix;
}

void HMMRegimeDetector::set_emission_params(std::vector<GaussianParams> params) {
    emissions_ = std::move(params);
}

void HMMRegimeDetector::set_features(std::vector<FeatureType> features) {
    extractor_.set_features(std::move(features));
}

void HMMRegimeDetector::set_normalize_features(bool normalize) {
    extractor_.set_normalize(normalize);
}

void HMMRegimeDetector::set_normalization_mode(NormalizationMode mode) {
    extractor_.set_normalization_mode(mode);
}

void HMMRegimeDetector::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return;
    }
    out << "states " << states_ << "\n";
    out << "window " << window_ << "\n";
    out << "normalization " << static_cast<int>(extractor_.normalization_mode()) << "\n";
    out << "features " << extractor_.features().size() << "\n";
    for (auto feature : extractor_.features()) {
        out << static_cast<int>(feature) << " ";
    }
    out << "\n";
    out << "initial ";
    for (double v : initial_) out << v << " ";
    out << "\n";
    out << "transition\n";
    for (int i = 0; i < states_; ++i) {
        for (int j = 0; j < states_; ++j) {
            out << transition_[i][j] << " ";
        }
        out << "\n";
    }
    out << "emissions " << emissions_.size() << "\n";
    for (const auto& params : emissions_) {
        out << "mean ";
        for (double v : params.mean) out << v << " ";
        out << "\n";
        out << "variance ";
        for (double v : params.variance) out << v << " ";
        out << "\n";
    }
}

void HMMRegimeDetector::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return;
    }
    std::string token;
    while (in >> token) {
        if (token == "states") {
            in >> states_;
            transition_.assign(states_, std::vector<double>(states_, 1.0 / states_));
            probabilities_.assign(states_, 1.0 / states_);
            initial_.assign(states_, 1.0 / states_);
        } else if (token == "window") {
            in >> window_;
            extractor_.set_window(window_);
        } else if (token == "normalization") {
            int mode = 0;
            in >> mode;
            extractor_.set_normalization_mode(static_cast<NormalizationMode>(mode));
        } else if (token == "features") {
            size_t count = 0;
            in >> count;
            std::vector<FeatureType> features;
            features.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                int value = 0;
                in >> value;
                features.push_back(static_cast<FeatureType>(value));
            }
            extractor_.set_features(std::move(features));
        } else if (token == "initial") {
            initial_.assign(states_, 0.0);
            for (int i = 0; i < states_; ++i) {
                in >> initial_[i];
            }
        } else if (token == "transition") {
            transition_.assign(states_, std::vector<double>(states_, 0.0));
            for (int i = 0; i < states_; ++i) {
                for (int j = 0; j < states_; ++j) {
                    in >> transition_[i][j];
                }
            }
        } else if (token == "emissions") {
            size_t count = 0;
            in >> count;
            emissions_.clear();
            emissions_.reserve(count);
            in >> std::ws;
            for (size_t i = 0; i < count; ++i) {
                GaussianParams params;
                in >> token; // mean
                std::getline(in >> std::ws, token);
                std::istringstream mean_stream(token);
                double value = 0.0;
                while (mean_stream >> value) {
                    params.mean.push_back(value);
                }
                in >> token; // variance
                std::getline(in >> std::ws, token);
                std::istringstream var_stream(token);
                while (var_stream >> value) {
                    params.variance.push_back(value);
                }
                emissions_.push_back(std::move(params));
            }
        }
    }
}

void HMMRegimeDetector::configure(const Config& config) {
    int states = static_cast<int>(config.get_as<int64_t>("hmm.states").value_or(states_));
    int window = static_cast<int>(config.get_as<int64_t>("hmm.window").value_or(window_));
    if (states != states_) {
        states_ = states;
        transition_.assign(states_, std::vector<double>(states_, 1.0 / states_));
        probabilities_.assign(states_, 1.0 / states_);
        initial_.assign(states_, 1.0 / states_);
    }
    if (window != window_) {
        window_ = window;
        extractor_.set_window(window_);
    }
    if (auto normalize = config.get_as<bool>("hmm.normalize_features")) {
        extractor_.set_normalize(*normalize);
    }
    if (auto norm_mode = config.get_as<std::string>("hmm.normalization")) {
        if (*norm_mode == "zscore") {
            extractor_.set_normalization_mode(NormalizationMode::ZScore);
        } else if (*norm_mode == "minmax") {
            extractor_.set_normalization_mode(NormalizationMode::MinMax);
        } else if (*norm_mode == "robust") {
            extractor_.set_normalization_mode(NormalizationMode::Robust);
        } else {
            extractor_.set_normalization_mode(NormalizationMode::None);
        }
    }
    if (auto v = config.get_as<bool>("hmm.kalman_enabled")) {
        enable_kalman_ = *v;
    }
    if (auto v = config.get_as<double>("hmm.kalman_process_noise")) {
        kalman_process_noise_ = *v;
    }
    if (auto v = config.get_as<double>("hmm.kalman_measurement_noise")) {
        kalman_measurement_noise_ = *v;
    }
    if (enable_kalman_) {
        kalman_filters_.assign(states_,
                               KalmanFilter1D(kalman_process_noise_, kalman_measurement_noise_));
    } else {
        kalman_filters_.clear();
    }
}

std::vector<std::string> HMMRegimeDetector::state_names() const {
    std::vector<std::string> names;
    names.reserve(states_);
    if (states_ == 4) {
        names = {"Bull", "Neutral", "Bear", "Crisis"};
        return names;
    }
    for (int i = 0; i < states_; ++i) {
        names.push_back("State" + std::to_string(i));
    }
    return names;
}

FeatureVector HMMRegimeDetector::extract_features(const data::Bar& bar) {
    return extractor_.on_bar(bar);
}

FeatureVector HMMRegimeDetector::extract_features(const data::Tick& tick) {
    data::Bar bar;
    bar.timestamp = tick.timestamp;
    bar.symbol = tick.symbol;
    bar.open = tick.price;
    bar.high = tick.price;
    bar.low = tick.price;
    bar.close = tick.price;
    bar.volume = static_cast<Volume>(tick.quantity);
    return extractor_.on_bar(bar);
}

RegimeState HMMRegimeDetector::detect(const FeatureVector& features, Timestamp timestamp) {
    std::vector<double> logp(states_, 0.0);
    for (int i = 0; i < states_; ++i) {
        double prior = 0.0;
        for (int j = 0; j < states_; ++j) {
            prior += probabilities_[j] * transition_[j][i];
        }
        if (prior <= 0) {
            prior = 1e-12;
        }
        double emission = log_gaussian(features, emissions_[i]);
        logp[i] = std::log(prior) + emission;
    }

    probabilities_ = softmax(logp);
    if (enable_kalman_) {
        if (static_cast<int>(kalman_filters_.size()) != states_) {
            kalman_filters_.assign(states_,
                                   KalmanFilter1D(kalman_process_noise_, kalman_measurement_noise_));
        }
        double sum = 0.0;
        for (int i = 0; i < states_; ++i) {
            probabilities_[i] = std::max(0.0, kalman_filters_[i].update(probabilities_[i]));
            sum += probabilities_[i];
        }
        if (sum > 0.0) {
            for (double& p : probabilities_) {
                p /= sum;
            }
        }
    }
    auto max_it = std::max_element(probabilities_.begin(), probabilities_.end());
    int idx = static_cast<int>(std::distance(probabilities_.begin(), max_it));

    RegimeState state;
    state.timestamp = timestamp;
    state.confidence = *max_it;
    state.regime = static_cast<RegimeType>(std::min(idx, 3));
    state.probabilities = {0, 0, 0, 0};
    state.probabilities_all = probabilities_;
    state.state_count = probabilities_.size();
    for (int i = 0; i < std::min(4, states_); ++i) {
        state.probabilities[i] = probabilities_[i];
    }
    return state;
}

void HMMRegimeDetector::initialize_from_data(const std::vector<FeatureVector>& data) {
    if (data.empty()) {
        return;
    }
    const int dim = static_cast<int>(data.front().size());
    emissions_.resize(states_);
    for (int i = 0; i < states_; ++i) {
        emissions_[i].mean.assign(dim, 0.0);
        emissions_[i].variance.assign(dim, 0.0);
    }
    FeatureVector mean(dim, 0.0);
    FeatureVector var(dim, 0.0);
    for (const auto& obs : data) {
        for (int d = 0; d < dim; ++d) {
            mean[d] += obs[d];
        }
    }
    for (int d = 0; d < dim; ++d) {
        mean[d] /= static_cast<double>(data.size());
    }
    for (const auto& obs : data) {
        for (int d = 0; d < dim; ++d) {
            double diff = obs[d] - mean[d];
            var[d] += diff * diff;
        }
    }
    for (int d = 0; d < dim; ++d) {
        var[d] = std::max(var[d] / static_cast<double>(data.size()), 1e-6);
    }
    for (int i = 0; i < states_; ++i) {
        emissions_[i].mean = mean;
        emissions_[i].variance = var;
    }
    probabilities_.assign(states_, 1.0 / states_);
    initial_.assign(states_, 1.0 / states_);
}

std::vector<std::vector<double>> HMMRegimeDetector::forward_log(
    const std::vector<FeatureVector>& data) const {
    const int T = static_cast<int>(data.size());
    std::vector<std::vector<double>> log_alpha(T, std::vector<double>(states_, 0.0));
    for (int i = 0; i < states_; ++i) {
        log_alpha[0][i] = std::log(std::max(initial_[i], 1e-12))
                          + log_gaussian(data[0], emissions_[i]);
    }
    for (int t = 1; t < T; ++t) {
        for (int j = 0; j < states_; ++j) {
            std::vector<double> acc(states_);
            for (int i = 0; i < states_; ++i) {
                acc[i] = log_alpha[t - 1][i] + std::log(std::max(transition_[i][j], 1e-12));
            }
            log_alpha[t][j] = log_sum_exp(acc) + log_gaussian(data[t], emissions_[j]);
        }
    }
    return log_alpha;
}

std::vector<std::vector<double>> HMMRegimeDetector::backward_log(
    const std::vector<FeatureVector>& data) const {
    const int T = static_cast<int>(data.size());
    std::vector<std::vector<double>> log_beta(T, std::vector<double>(states_, 0.0));
    for (int i = 0; i < states_; ++i) {
        log_beta[T - 1][i] = 0.0;
    }
    for (int t = T - 2; t >= 0; --t) {
        for (int i = 0; i < states_; ++i) {
            std::vector<double> acc(states_);
            for (int j = 0; j < states_; ++j) {
                acc[j] = std::log(std::max(transition_[i][j], 1e-12))
                         + log_gaussian(data[t + 1], emissions_[j])
                         + log_beta[t + 1][j];
            }
            log_beta[t][i] = log_sum_exp(acc);
        }
    }
    return log_beta;
}

}  // namespace regimeflow::regime
