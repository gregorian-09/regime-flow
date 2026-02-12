/**
 * @file spsc_queue.h
 * @brief RegimeFlow regimeflow spsc queue declarations.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace regimeflow::common {

/**
 * @brief Lock-free single-producer single-consumer ring buffer.
 * @tparam T Item type.
 * @tparam Capacity Fixed ring capacity.
 */
template<typename T, size_t Capacity>
class SpscQueue {
public:
    /**
     * @brief Enqueue an item.
     * @param value Item to enqueue.
     * @return True if enqueued, false if queue is full.
     */
    bool push(const T& value) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next = increment(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[head] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    /**
     * @brief Dequeue an item.
     * @param out Output value.
     * @return True if an item was popped, false if queue is empty.
     */
    bool pop(T& out) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        out = buffer_[tail];
        tail_.store(increment(tail), std::memory_order_release);
        return true;
    }

    /**
     * @brief Check if the queue is empty.
     * @return True if empty.
     */
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t increment(size_t value) {
        return (value + 1) % Capacity;
    }

    std::array<T, Capacity> buffer_{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

}  // namespace regimeflow::common
