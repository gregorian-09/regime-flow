#pragma once

#include "regimeflow/regime/types.h"

#include <deque>
#include <functional>
#include <map>
#include <vector>

namespace regimeflow::regime {

class RegimeStateManager {
public:
    explicit RegimeStateManager(size_t history_size = 256);

    void update(const RegimeState& state);

    RegimeType current_regime() const;
    double time_in_current_regime() const;
    std::vector<RegimeTransition> recent_transitions(size_t n) const;

    std::map<RegimeType, double> regime_frequencies() const;
    double avg_regime_duration(RegimeType regime) const;
    std::vector<std::vector<double>> empirical_transition_matrix() const;

    void register_transition_callback(
        std::function<void(const RegimeTransition&)> callback);

private:
    size_t to_index(RegimeType regime) const;
    void record_transition(const RegimeTransition& transition);

    RegimeState current_state_{};
    bool has_state_ = false;
    size_t history_size_ = 256;
    std::deque<RegimeTransition> transition_history_;
    std::vector<std::function<void(const RegimeTransition&)>> callbacks_;
};

}  // namespace regimeflow::regime
