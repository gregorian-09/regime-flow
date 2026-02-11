#include "regimeflow/engine/regime_tracker.h"

namespace regimeflow::engine {

RegimeTracker::RegimeTracker(std::unique_ptr<regime::RegimeDetector> detector)
    : detector_(std::move(detector)) {}

void RegimeTracker::record_state(const regime::RegimeState& state) {
    history_.push_back(state);
    if (history_size_ > 0 && history_.size() > history_size_) {
        history_.pop_front();
    }
}

void RegimeTracker::register_transition_callback(
    std::function<void(const regime::RegimeTransition&)> callback) {
    if (callback) {
        callbacks_.push_back(std::move(callback));
    }
}

void RegimeTracker::notify_transition(const regime::RegimeTransition& transition) {
    for (const auto& cb : callbacks_) {
        cb(transition);
    }
}

std::optional<regime::RegimeTransition> RegimeTracker::on_bar(const data::Bar& bar) {
    if (!detector_) {
        return std::nullopt;
    }
    regime::RegimeState next = detector_->on_bar(bar);
    if (!has_state_) {
        current_state_ = next;
        has_state_ = true;
        record_state(current_state_);
        return std::nullopt;
    }
    if (next.regime != current_state_.regime) {
        regime::RegimeTransition transition;
        transition.from = current_state_.regime;
        transition.to = next.regime;
        transition.timestamp = next.timestamp;
        transition.confidence = next.confidence;
        transition.duration_in_from =
            static_cast<double>((next.timestamp - current_state_.timestamp).total_seconds());
        current_state_ = next;
        record_state(current_state_);
        notify_transition(transition);
        return transition;
    }
    current_state_ = next;
    record_state(current_state_);
    return std::nullopt;
}

std::optional<regime::RegimeTransition> RegimeTracker::on_tick(const data::Tick& tick) {
    if (!detector_) {
        return std::nullopt;
    }
    regime::RegimeState next = detector_->on_tick(tick);
    if (!has_state_) {
        current_state_ = next;
        has_state_ = true;
        record_state(current_state_);
        return std::nullopt;
    }
    if (next.regime != current_state_.regime) {
        regime::RegimeTransition transition;
        transition.from = current_state_.regime;
        transition.to = next.regime;
        transition.timestamp = next.timestamp;
        transition.confidence = next.confidence;
        transition.duration_in_from =
            static_cast<double>((next.timestamp - current_state_.timestamp).total_seconds());
        current_state_ = next;
        record_state(current_state_);
        notify_transition(transition);
        return transition;
    }
    current_state_ = next;
    record_state(current_state_);
    return std::nullopt;
}

}  // namespace regimeflow::engine
