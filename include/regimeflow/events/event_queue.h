/**
 * @file event_queue.h
 * @brief RegimeFlow regimeflow event queue declarations.
 */

#pragma once

#include "regimeflow/events/event.h"
#include "regimeflow/common/memory.h"

#include <atomic>
#include <optional>
#include <queue>
#include <vector>

namespace regimeflow::events {

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
 * Producers push into a lock-free pending list, which is drained on pop/peek.
 */
class EventQueue {
public:
    /**
     * @brief Enqueue an event.
     * @param event Event to enqueue.
     */
    void push(Event event) {
        event.sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        Node* node = pool_.allocate();
        new (node) Node{std::move(event), nullptr};
        Node* prev = pending_.exchange(node, std::memory_order_acq_rel);
        node->next = prev;
    }

    /**
     * @brief Pop the next event in priority order.
     * @return Optional event, empty if none.
     */
    std::optional<Event> pop() {
        drain_pending();
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
        drain_pending();
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
        drain_pending();
        return queue_.empty();
    }

    /**
     * @brief Get the number of queued events.
     * @return Queue size.
     */
    size_t size() {
        drain_pending();
        return queue_.size();
    }

    /**
     * @brief Clear all queued events.
     */
    void clear() {
        drain_pending();
        queue_ = std::priority_queue<Event, std::vector<Event>, EventComparator>();
    }

    /**
     * @brief Destroy the queue and free pooled nodes.
     */
    ~EventQueue() { clear(); }

private:
    /**
     * @brief Internal node used for the pending list.
     */
    struct Node {
        Event event;
        Node* next = nullptr;
    };

    void drain_pending() {
        Node* list = pending_.exchange(nullptr, std::memory_order_acq_rel);
        while (list) {
            Node* next = list->next;
            queue_.push(std::move(list->event));
            list->~Node();
            pool_.deallocate(list);
            list = next;
        }
    }

    std::priority_queue<Event, std::vector<Event>, EventComparator> queue_;
    std::atomic<Node*> pending_{nullptr};
    std::atomic<uint64_t> next_sequence_{0};
    common::PoolAllocator<Node> pool_{1024};
};

}  // namespace regimeflow::events
