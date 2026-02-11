#include "regimeflow/live/event_bus.h"

namespace regimeflow::live {

EventBus::EventBus() = default;

EventBus::~EventBus() {
    stop();
}

void EventBus::start() {
    if (running_.exchange(true)) {
        return;
    }
    dispatcher_ = std::thread(&EventBus::dispatch_loop, this);
}

void EventBus::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    queue_cv_.notify_all();
    if (dispatcher_.joinable()) {
        dispatcher_.join();
    }
    drain_pending();
}

EventBus::SubscriptionId EventBus::subscribe(LiveTopic topic, Callback callback) {
    std::lock_guard<std::mutex> lock(sub_mutex_);
    SubscriptionId id = next_id_++;
    subscribers_[id] = {topic, std::move(callback)};
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    std::lock_guard<std::mutex> lock(sub_mutex_);
    subscribers_.erase(id);
}

void EventBus::publish(LiveMessage message) {
    Node* node = pool_.allocate();
    new (node) Node{std::move(message), nullptr};
    Node* prev = pending_.exchange(node, std::memory_order_acq_rel);
    node->next = prev;
    queue_cv_.notify_one();
}

void EventBus::drain_pending() {
    Node* list = pending_.exchange(nullptr, std::memory_order_acq_rel);
    while (list) {
        Node* next = list->next;
        queue_.push(std::move(list->message));
        list->~Node();
        pool_.deallocate(list);
        list = next;
    }
}

void EventBus::dispatch_loop() {
    while (running_) {
        LiveMessage message;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return pending_.load(std::memory_order_acquire) != nullptr || !queue_.empty()
                    || !running_;
            });
            drain_pending();
            if (!running_ && queue_.empty()) {
                break;
            }
            message = std::move(queue_.front());
            queue_.pop();
        }

        std::vector<Callback> callbacks;
        {
            std::lock_guard<std::mutex> lock(sub_mutex_);
            for (const auto& [id, entry] : subscribers_) {
                if (entry.first == message.topic) {
                    callbacks.push_back(entry.second);
                }
            }
        }

        for (const auto& cb : callbacks) {
            cb(message);
        }
    }
}

}  // namespace regimeflow::live
