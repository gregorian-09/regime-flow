#include "regimeflow/engine/event_loop.h"

namespace regimeflow::engine {

EventLoop::EventLoop(events::EventQueue* queue) : queue_(queue) {}

void EventLoop::set_dispatcher(events::EventDispatcher* dispatcher) {
    dispatcher_ = dispatcher;
}

void EventLoop::add_pre_hook(Hook hook) {
    pre_hooks_.push_back(std::move(hook));
}

void EventLoop::add_post_hook(Hook hook) {
    post_hooks_.push_back(std::move(hook));
}

void EventLoop::set_progress_callback(ProgressCallback callback) {
    progress_callback_ = std::move(callback);
}

void EventLoop::run() {
    if (!queue_ || !dispatcher_) {
        return;
    }
    running_ = true;
    processed_ = 0;
    while (running_) {
        auto next = queue_->pop();
        if (!next) {
            break;
        }
        const auto& event = *next;
        current_time_ = event.timestamp;
        for (const auto& hook : pre_hooks_) {
            hook(event);
        }
        dispatcher_->dispatch(event);
        for (const auto& hook : post_hooks_) {
            hook(event);
        }
        ++processed_;
        if (progress_callback_) {
            progress_callback_(processed_, queue_->size());
        }
    }
    running_ = false;
}

void EventLoop::run_until(Timestamp end_time) {
    if (!queue_ || !dispatcher_) {
        return;
    }
    running_ = true;
    while (running_) {
        auto next = queue_->peek();
        if (!next || next->timestamp > end_time) {
            break;
        }
        if (!step()) {
            break;
        }
    }
    running_ = false;
}

bool EventLoop::step() {
    if (!queue_ || !dispatcher_) {
        return false;
    }
    auto next = queue_->pop();
    if (!next) {
        return false;
    }
    const auto& event = *next;
    current_time_ = event.timestamp;
    for (const auto& hook : pre_hooks_) {
        hook(event);
    }
    dispatcher_->dispatch(event);
    for (const auto& hook : post_hooks_) {
        hook(event);
    }
    ++processed_;
    if (progress_callback_) {
        progress_callback_(processed_, queue_->size());
    }
    return true;
}

void EventLoop::stop() {
    running_ = false;
}

}  // namespace regimeflow::engine
