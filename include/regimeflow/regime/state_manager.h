/**
 * @file state_manager.h
 * @brief RegimeFlow regimeflow state manager declarations.
 */

#pragma once

#include "regimeflow/regime/types.h"

#include <deque>
#include <functional>
#include <map>
#include <vector>

namespace regimeflow::regime {

/**
 * @brief Tracks regime state history and transitions.
 */
class RegimeStateManager {
public:
    /**
     * @brief Construct with history size.
     * @param history_size Max transitions to keep.
     */
    explicit RegimeStateManager(size_t history_size = 256);

    /**
     * @brief Update with a new regime state.
     * @param state New regime state.
     */
    void update(const RegimeState& state);

    /**
     * @brief Current regime type.
     */
    RegimeType current_regime() const;
    /**
     * @brief Time spent in the current regime.
     */
    double time_in_current_regime() const;
    /**
     * @brief Recent regime transitions.
     * @param n Number of transitions.
     * @return Vector of transitions.
     */
    std::vector<RegimeTransition> recent_transitions(size_t n) const;

    /**
     * @brief Regime frequency distribution.
     */
    std::map<RegimeType, double> regime_frequencies() const;
    /**
     * @brief Average duration of a regime.
     * @param regime Regime type.
     */
    double avg_regime_duration(RegimeType regime) const;
    /**
     * @brief Empirical transition matrix from history.
     */
    std::vector<std::vector<double>> empirical_transition_matrix() const;

    /**
     * @brief Register a transition callback.
     * @param callback Callback invoked on transitions.
     */
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
