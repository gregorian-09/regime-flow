#include "regimeflow/live/event_bus.h"

#include <ranges>

namespace regimeflow::live
{
    EventBus::EventBus() = default;

    EventBus::~EventBus() {
        stop();
    }

    void EventBus::start() {
        std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
        if (running_.load()) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(publisher_mutex_);
            accepting_ = true;
        }
        running_ = true;
        dispatcher_ = std::thread(&EventBus::dispatch_loop, this);
    }

    void EventBus::stop() {
        std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
        {
            std::unique_lock<std::mutex> lock(publisher_mutex_);
            accepting_ = false;
            publisher_cv_.wait(lock, [this] { return active_publishers_ == 0; });
        }
        if (!running_.exchange(false)) {
            return;
        }
        queue_cv_.notify_all();
        if (dispatcher_.joinable()) {
            dispatcher_.join();
        }
    }

    EventBus::SubscriptionId EventBus::subscribe(LiveTopic topic, Callback callback) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        const SubscriptionId id = next_id_++;
        subscribers_[id] = {topic, std::move(callback)};
        return id;
    }

    void EventBus::unsubscribe(SubscriptionId id) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        subscribers_.erase(id);
    }

    void EventBus::publish(LiveMessage message) {
        static_cast<void>(try_publish(std::move(message)));
    }

    bool EventBus::try_publish(LiveMessage message) {
        {
            std::lock_guard<std::mutex> lock(publisher_mutex_);
            if (!accepting_) {
                return false;
            }
            ++active_publishers_;
        }
        const auto finish_publish = [this] {
            std::lock_guard<std::mutex> lock(publisher_mutex_);
            --active_publishers_;
            if (active_publishers_ == 0) {
                publisher_cv_.notify_all();
            }
        };
        try {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                queue_.push(std::move(message));
            }
            finish_publish();
            queue_cv_.notify_one();
            return true;
        } catch (...) {
            finish_publish();
            throw;
        }
    }

    void EventBus::dispatch_loop() {
        while (true) {
            LiveMessage message;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] {
                    return !queue_.empty() || !running_;
                });
                if (!running_ && queue_.empty()) {
                    break;
                }
                message = std::move(queue_.front());
                queue_.pop();
            }

            std::vector<Callback> callbacks;
            {
                std::lock_guard<std::mutex> lock(sub_mutex_);
                for (const auto& [fst, snd] : subscribers_ | std::views::values) {
                    if (fst == message.topic) {
                        callbacks.push_back(snd);
                    }
                }
            }

            for (const auto& cb : callbacks) {
                cb(message);
            }
        }
    }
}  // namespace regimeflow::live
