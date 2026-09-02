/**
 * @file event_queue.h
 * @brief RegimeFlow regimeflow event queue declarations.
 */

#pragma once

#include "regimeflow/events/event.h"
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

namespace regimeflow::events
{
    /**
     * @brief Priority comparator for events (time, priority, sequence).
     */
    struct EventComparator {
        bool operator()(const Event& a, const Event& b) const {
            if (a.timestamp != b.timestamp) {
                return a.timestamp > b.timestamp;
            }
            if (a.priority != b.priority) {
                return a.priority > b.priority;
            }
            return a.sequence > b.sequence;
        }
    };

    /**
     * @brief Concurrent event queue with deterministic ordering.
     *
     * @details Events are prioritized by timestamp, then priority, then sequence.
     * Producers and consumers synchronize directly on the priority queue.  This
     * deliberately favors safe memory reclamation over a lock-free linked list:
     * an MPSC list requires a reclamation scheme before detached nodes can be
     * returned to an allocator.
     */
    class EventQueue {
    public:
        /**
         * @brief Enqueue an event.
         * @param event Event to enqueue.
         */
        void push(Event event) {
            event.sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(queue_mutex_);
            queue_.push(std::move(event));
        }

        /**
         * @brief Pop the next event in priority order.
         * @return Optional event, empty if none.
         */
        std::optional<Event> pop() {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (queue_.empty()) {
                return std::nullopt;
            }
            Event event = queue_.top();
            queue_.pop();
            return event;
        }

        /**
         * @brief Peek the next event in priority order.
         * @return Optional event, empty if none.
         */
        std::optional<Event> peek() {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (queue_.empty()) {
                return std::nullopt;
            }
            return queue_.top();
        }

        /**
         * @brief Check if the queue is empty.
         * @return True if empty.
         */
        bool empty() {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            return queue_.empty();
        }

        /**
         * @brief Get the number of queued events.
         * @return Queue size.
         */
        size_t size() {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            return queue_.size();
        }

        /**
         * @brief Clear all queued events.
         */
        void clear() {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            queue_ = std::priority_queue<Event, std::vector<Event>, EventComparator>();
        }

        /**
         * @brief Destroy the queue and release queued events.
         */
        ~EventQueue() { clear(); }

    private:
        std::mutex queue_mutex_;
        std::priority_queue<Event, std::vector<Event>, EventComparator> queue_;
        std::atomic<uint64_t> next_sequence_{0};
    };
}  // namespace regimeflow::events
