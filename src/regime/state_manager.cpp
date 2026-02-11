#include "regimeflow/regime/state_manager.h"

#include <algorithm>

namespace regimeflow::regime {

RegimeStateManager::RegimeStateManager(size_t history_size) : history_size_(history_size) {}

void RegimeStateManager::update(const RegimeState& state) {
    if (!has_state_) {
        current_state_ = state;
        has_state_ = true;
        return;
    }
    if (state.regime != current_state_.regime) {
        RegimeTransition transition;
        transition.from = current_state_.regime;
        transition.to = state.regime;
        transition.timestamp = state.timestamp;
        transition.confidence = state.confidence;
        transition.duration_in_from =
            static_cast<double>((state.timestamp - current_state_.timestamp).total_seconds());
        record_transition(transition);
        for (const auto& cb : callbacks_) {
            cb(transition);
        }
    }
    current_state_ = state;
}

RegimeType RegimeStateManager::current_regime() const {
    return current_state_.regime;
}

double RegimeStateManager::time_in_current_regime() const {
    if (!has_state_) {
        return 0.0;
    }
    auto duration = Timestamp::now() - current_state_.timestamp;
    return static_cast<double>(duration.total_seconds());
}

std::vector<RegimeTransition> RegimeStateManager::recent_transitions(size_t n) const {
    std::vector<RegimeTransition> out;
    if (n == 0 || transition_history_.empty()) {
        return out;
    }
    size_t start = transition_history_.size() > n ? transition_history_.size() - n : 0;
    for (size_t i = start; i < transition_history_.size(); ++i) {
        out.push_back(transition_history_[i]);
    }
    return out;
}

std::map<RegimeType, double> RegimeStateManager::regime_frequencies() const {
    std::map<RegimeType, double> freq;
    if (transition_history_.empty()) {
        if (has_state_) {
            freq[current_state_.regime] = 1.0;
        }
        return freq;
    }
    std::map<RegimeType, double> counts;
    for (const auto& transition : transition_history_) {
        counts[transition.from] += 1.0;
        counts[transition.to] += 1.0;
    }
    if (has_state_) {
        counts[current_state_.regime] += 1.0;
    }
    double total = 0.0;
    for (const auto& [_, count] : counts) {
        total += count;
    }
    if (total == 0.0) {
        return freq;
    }
    for (const auto& [regime, count] : counts) {
        freq[regime] = count / total;
    }
    return freq;
}

double RegimeStateManager::avg_regime_duration(RegimeType regime) const {
    double total = 0.0;
    int count = 0;
    for (const auto& transition : transition_history_) {
        if (transition.from == regime) {
            total += transition.duration_in_from;
            ++count;
        }
    }
    if (has_state_ && current_state_.regime == regime) {
        total += time_in_current_regime();
        ++count;
    }
    if (count == 0) {
        return 0.0;
    }
    return total / static_cast<double>(count);
}

std::vector<std::vector<double>> RegimeStateManager::empirical_transition_matrix() const {
    const size_t size = 4;
    std::vector<std::vector<double>> matrix(size, std::vector<double>(size, 0.0));
    std::vector<double> row_sum(size, 0.0);
    for (const auto& transition : transition_history_) {
        size_t from = to_index(transition.from);
        size_t to = to_index(transition.to);
        if (from >= size || to >= size) {
            continue;
        }
        matrix[from][to] += 1.0;
        row_sum[from] += 1.0;
    }
    for (size_t i = 0; i < size; ++i) {
        if (row_sum[i] == 0.0) {
            continue;
        }
        for (size_t j = 0; j < size; ++j) {
            matrix[i][j] /= row_sum[i];
        }
    }
    return matrix;
}

void RegimeStateManager::register_transition_callback(
    std::function<void(const RegimeTransition&)> callback) {
    if (callback) {
        callbacks_.push_back(std::move(callback));
    }
}

size_t RegimeStateManager::to_index(RegimeType regime) const {
    switch (regime) {
        case RegimeType::Bull: return 0;
        case RegimeType::Neutral: return 1;
        case RegimeType::Bear: return 2;
        case RegimeType::Crisis: return 3;
        default: return 3;
    }
}

void RegimeStateManager::record_transition(const RegimeTransition& transition) {
    transition_history_.push_back(transition);
    if (history_size_ > 0 && transition_history_.size() > history_size_) {
        transition_history_.pop_front();
    }
}

}  // namespace regimeflow::regime
