#pragma once

#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/regime/types.h"

#include <deque>
#include <functional>
#include <optional>
#include <memory>

namespace regimeflow::engine {

class RegimeTracker {
public:
    explicit RegimeTracker(std::unique_ptr<regime::RegimeDetector> detector);
    RegimeTracker(RegimeTracker&&) noexcept = default;
    RegimeTracker& operator=(RegimeTracker&&) noexcept = default;
    RegimeTracker(const RegimeTracker&) = delete;
    RegimeTracker& operator=(const RegimeTracker&) = delete;

    std::optional<regime::RegimeTransition> on_bar(const data::Bar& bar);
    std::optional<regime::RegimeTransition> on_tick(const data::Tick& tick);

    const regime::RegimeState& current_state() const { return current_state_; }
    const std::deque<regime::RegimeState>& history() const { return history_; }
    void set_history_size(size_t size) { history_size_ = size; }
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
