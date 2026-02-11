#pragma once

#include "regimeflow/events/event.h"
#include "regimeflow/common/memory.h"

#include <atomic>
#include <optional>
#include <queue>
#include <vector>

namespace regimeflow::events {

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

class EventQueue {
public:
    void push(Event event) {
        event.sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        Node* node = pool_.allocate();
        new (node) Node{std::move(event), nullptr};
        Node* prev = pending_.exchange(node, std::memory_order_acq_rel);
        node->next = prev;
    }

    std::optional<Event> pop() {
        drain_pending();
        if (queue_.empty()) {
            return std::nullopt;
        }
        Event event = queue_.top();
        queue_.pop();
        return event;
    }

    std::optional<Event> peek() {
        drain_pending();
        if (queue_.empty()) {
            return std::nullopt;
        }
        return queue_.top();
    }

    bool empty() {
        drain_pending();
        return queue_.empty();
    }

    size_t size() {
        drain_pending();
        return queue_.size();
    }

    void clear() {
        drain_pending();
        queue_ = std::priority_queue<Event, std::vector<Event>, EventComparator>();
    }

    ~EventQueue() { clear(); }

private:
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
