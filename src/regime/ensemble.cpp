#include "regimeflow/regime/ensemble.h"
#include "regimeflow/regime/regime_factory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <numeric>
#include <string>

namespace regimeflow::regime {

void EnsembleRegimeDetector::add_detector(std::unique_ptr<RegimeDetector> detector, double weight) {
    if (!detector || weight <= 0) {
        return;
    }
    detectors_.push_back(std::move(detector));
    weights_.push_back(weight);
}

RegimeState EnsembleRegimeDetector::on_bar(const data::Bar& bar) {
    std::vector<RegimeState> states;
    states.reserve(detectors_.size());
    for (const auto& detector : detectors_) {
        states.push_back(detector->on_bar(bar));
    }
    return combine(states, bar.timestamp);
}

RegimeState EnsembleRegimeDetector::on_tick(const data::Tick& tick) {
    std::vector<RegimeState> states;
    states.reserve(detectors_.size());
    for (const auto& detector : detectors_) {
        states.push_back(detector->on_tick(tick));
    }
    return combine(states, tick.timestamp);
}

RegimeState EnsembleRegimeDetector::on_book(const data::OrderBook& book) {
    std::vector<RegimeState> states;
    states.reserve(detectors_.size());
    for (const auto& detector : detectors_) {
        states.push_back(detector->on_book(book));
    }
    return combine(states, book.timestamp);
}

void EnsembleRegimeDetector::train(const std::vector<FeatureVector>& data) {
    for (auto& detector : detectors_) {
        detector->train(data);
    }
}

void EnsembleRegimeDetector::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return;
    }
    out << "voting " << static_cast<int>(voting_method_) << "\n";
    out << "detectors " << detectors_.size() << "\n";
    for (size_t i = 0; i < detectors_.size(); ++i) {
        out << "weight " << (i < weights_.size() ? weights_[i] : 1.0) << "\n";
        out << "states " << detectors_[i]->num_states() << "\n";
    }
}

void EnsembleRegimeDetector::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return;
    }
    std::string token;
    size_t count = 0;
    while (in >> token) {
        if (token == "voting") {
            int value = 0;
            in >> value;
            voting_method_ = static_cast<VotingMethod>(value);
        } else if (token == "detectors") {
            in >> count;
            break;
        }
    }
    (void)count;
}

void EnsembleRegimeDetector::configure(const Config& config) {
    if (auto voting = config.get_as<std::string>("ensemble.voting_method")) {
        if (*voting == "majority") {
            voting_method_ = VotingMethod::Majority;
        } else if (*voting == "confidence_weighted") {
            voting_method_ = VotingMethod::ConfidenceWeighted;
        } else if (*voting == "bayesian") {
            voting_method_ = VotingMethod::Bayesian;
        } else {
            voting_method_ = VotingMethod::WeightedAverage;
        }
    }
    if (auto detectors = config.get_as<ConfigValue::Array>("ensemble.detectors")) {
        detectors_.clear();
        weights_.clear();
        for (const auto& entry : *detectors) {
            const auto* obj = entry.get_if<ConfigValue::Object>();
            if (!obj) {
                continue;
            }
            Config det_cfg(*obj);
            auto det = RegimeFactory::create_detector(det_cfg);
            double weight = det_cfg.get_as<double>("weight").value_or(1.0);
            if (det) {
                add_detector(std::move(det), weight);
            }
        }
    }
}

int EnsembleRegimeDetector::num_states() const {
    if (detectors_.empty()) {
        return 0;
    }
    return detectors_[0]->num_states();
}

std::vector<std::string> EnsembleRegimeDetector::state_names() const {
    if (detectors_.empty()) {
        return {};
    }
    return detectors_[0]->state_names();
}

RegimeState EnsembleRegimeDetector::combine(const std::vector<RegimeState>& states,
                                            Timestamp timestamp) const {
    RegimeState result;
    result.timestamp = timestamp;
    result.probabilities = {0, 0, 0, 0};

    if (states.empty()) {
        return result;
    }

    const size_t prob_size = result.probabilities.size();

    auto normalize_probs = [&](std::vector<double>& probs) {
        double sum = std::accumulate(probs.begin(), probs.end(), 0.0);
        if (sum <= 0.0) {
            double uniform = probs.empty() ? 0.0 : 1.0 / probs.size();
            std::fill(probs.begin(), probs.end(), uniform);
            return;
        }
        for (double& p : probs) {
            p /= sum;
        }
    };

    if (voting_method_ == VotingMethod::Majority) {
        std::array<double, 4> counts{0, 0, 0, 0};
        for (const auto& state : states) {
            size_t idx = static_cast<size_t>(std::min(static_cast<int>(state.regime), 3));
            counts[idx] += 1.0;
        }
        for (size_t i = 0; i < prob_size; ++i) {
            result.probabilities[i] = counts[i];
        }
        std::vector<double> probs(result.probabilities.begin(), result.probabilities.end());
        normalize_probs(probs);
        std::copy(probs.begin(), probs.end(), result.probabilities.begin());
    } else if (voting_method_ == VotingMethod::ConfidenceWeighted) {
        for (size_t i = 0; i < states.size(); ++i) {
            double w = i < weights_.size() ? weights_[i] : 1.0;
            double conf = states[i].confidence > 0 ? states[i].confidence : 1.0;
            if (!states[i].probabilities_all.empty()) {
                for (size_t j = 0; j < prob_size && j < states[i].probabilities_all.size(); ++j) {
                    result.probabilities[j] += states[i].probabilities_all[j] * w * conf;
                }
            } else {
                for (size_t j = 0; j < prob_size; ++j) {
                    result.probabilities[j] += states[i].probabilities[j] * w * conf;
                }
            }
        }
        std::vector<double> probs(result.probabilities.begin(), result.probabilities.end());
        normalize_probs(probs);
        std::copy(probs.begin(), probs.end(), result.probabilities.begin());
    } else if (voting_method_ == VotingMethod::Bayesian) {
        std::vector<double> logp(prob_size, 0.0);
        for (size_t i = 0; i < states.size(); ++i) {
            double w = i < weights_.size() ? weights_[i] : 1.0;
            for (size_t j = 0; j < prob_size; ++j) {
                double p = states[i].probabilities[j];
                if (!states[i].probabilities_all.empty() && j < states[i].probabilities_all.size()) {
                    p = states[i].probabilities_all[j];
                }
                p = std::max(p, 1e-12);
                logp[j] += std::log(p) * w;
            }
        }
        double maxv = *std::max_element(logp.begin(), logp.end());
        double sum = 0.0;
        for (double& v : logp) {
            v = std::exp(v - maxv);
            sum += v;
        }
        for (size_t j = 0; j < prob_size; ++j) {
            result.probabilities[j] = sum > 0.0 ? (logp[j] / sum) : (1.0 / prob_size);
        }
    } else {
        double total_weight = std::accumulate(weights_.begin(), weights_.end(), 0.0);
        if (total_weight == 0) {
            total_weight = 1.0;
        }
        for (size_t i = 0; i < states.size(); ++i) {
            double w = i < weights_.size() ? weights_[i] : 1.0;
            for (size_t j = 0; j < prob_size; ++j) {
                result.probabilities[j] += states[i].probabilities[j] * w;
            }
        }
        for (double& p : result.probabilities) {
            p /= total_weight;
        }
    }

    auto max_it = std::max_element(result.probabilities.begin(), result.probabilities.end());
    int idx = static_cast<int>(std::distance(result.probabilities.begin(), max_it));
    result.regime = static_cast<RegimeType>(std::min(idx, 3));
    result.confidence = *max_it;
    result.state_count = prob_size;
    result.probabilities_all.assign(result.probabilities.begin(), result.probabilities.end());
    return result;
}

}  // namespace regimeflow::regime
