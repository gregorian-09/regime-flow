#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace regimeflow::common {

template<typename T>
class MpscQueue {
public:
    MpscQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~MpscQueue() {
        T value;
        while (pop(value)) {
        }
        Node* node = head_.load(std::memory_order_relaxed);
        delete node;
    }

    MpscQueue(const MpscQueue&) = delete;
    MpscQueue& operator=(const MpscQueue&) = delete;

    void push(const T& value) {
        Node* node = new Node(value);
        Node* prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
    }

    void push(T&& value) {
        Node* node = new Node(std::move(value));
        Node* prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
    }

    bool pop(T& out) {
        Node* head = head_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        if (!next) {
            return false;
        }
        out = std::move(next->value);
        head_.store(next, std::memory_order_release);
        delete head;
        return true;
    }

    bool empty() const {
        Node* head = head_.load(std::memory_order_acquire);
        return head->next.load(std::memory_order_acquire) == nullptr;
    }

private:
    struct Node {
        T value{};
        std::atomic<Node*> next{nullptr};

        Node() = default;
        explicit Node(T v) : value(std::move(v)) {}
    };

    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
};

}  // namespace regimeflow::common
