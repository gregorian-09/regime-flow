/**
 * @file regime_tracker.h
 * @brief RegimeFlow regimeflow regime tracker declarations.
 */

#pragma once

#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/regime/types.h"

#include <deque>
#include <functional>
#include <optional>
#include <memory>

namespace regimeflow::engine {

/**
 * @brief Tracks regime state transitions using a detector.
 */
class RegimeTracker {
public:
    /**
     * @brief Construct with a regime detector.
     * @param detector Detector instance.
     */
    explicit RegimeTracker(std::unique_ptr<regime::RegimeDetector> detector);
    RegimeTracker(RegimeTracker&&) noexcept = default;
    RegimeTracker& operator=(RegimeTracker&&) noexcept = default;
    RegimeTracker(const RegimeTracker&) = delete;
    RegimeTracker& operator=(const RegimeTracker&) = delete;

    /**
     * @brief Feed a bar and optionally produce a transition.
     * @param bar Market bar.
     * @return Transition if regime changed.
     */
    std::optional<regime::RegimeTransition> on_bar(const data::Bar& bar);
    /**
     * @brief Feed a tick and optionally produce a transition.
     * @param tick Market tick.
     * @return Transition if regime changed.
     */
    std::optional<regime::RegimeTransition> on_tick(const data::Tick& tick);

    /**
     * @brief Current regime state.
     */
    const regime::RegimeState& current_state() const { return current_state_; }
    /**
     * @brief History of previous regime states.
     */
    const std::deque<regime::RegimeState>& history() const { return history_; }
    /**
     * @brief Set the maximum history size.
     * @param size Maximum number of stored states.
     */
    void set_history_size(size_t size) { history_size_ = size; }
    /**
     * @brief Register a callback for transitions.
     * @param callback Callback invoked on regime change.
     */
    void register_transition_callback(
        std::function<void(const regime::RegimeTransition&)> callback);

private:
    void record_state(const regime::RegimeState& state);
    void notify_transition(const regime::RegimeTransition& transition);

    std::unique_ptr<regime::RegimeDetector> detector_;
    regime::RegimeState current_state_;
    std::deque<regime::RegimeState> history_;
    std::vector<std::function<void(const regime::RegimeTransition&)>> callbacks_;
    size_t history_size_ = 256;
    bool has_state_ = false;
};

}  // namespace regimeflow::engine
