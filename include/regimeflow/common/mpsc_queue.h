/**
 * @file mpsc_queue.h
 * @brief RegimeFlow regimeflow mpsc queue declarations.
 */

#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace regimeflow::common {

/**
 * @brief Lock-free multi-producer single-consumer queue.
 * @tparam T Item type.
 *
 * @details Uses a linked-node Michael-Scott style queue. Producers may call
 * push concurrently, while a single consumer calls pop.
 */
template<typename T>
class MpscQueue {
public:
    /**
     * @brief Construct an empty queue with a dummy node.
     */
    MpscQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    /**
     * @brief Drain and destroy the queue.
     */
    ~MpscQueue() {
        T value;
        while (pop(value)) {
        }
        Node* node = head_.load(std::memory_order_relaxed);
        delete node;
    }

    MpscQueue(const MpscQueue&) = delete;
    MpscQueue& operator=(const MpscQueue&) = delete;

    /**
     * @brief Enqueue by copy.
     * @param value Item to enqueue.
     */
    void push(const T& value) {
        Node* node = new Node(value);
        Node* prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
    }

    /**
     * @brief Enqueue by move.
     * @param value Item to enqueue.
     */
    void push(T&& value) {
        Node* node = new Node(std::move(value));
        Node* prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
    }

    /**
     * @brief Dequeue an item if available.
     * @param out Output value.
     * @return True if an item was popped.
     */
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

    /**
     * @brief Check if the queue is empty.
     * @return True if empty.
     */
    bool empty() const {
        Node* head = head_.load(std::memory_order_acquire);
        return head->next.load(std::memory_order_acquire) == nullptr;
    }

private:
    /**
     * @brief Internal node for the linked queue.
     */
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
