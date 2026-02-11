#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

namespace regimeflow::common {

class MonotonicArena {
public:
    explicit MonotonicArena(size_t block_size = 1 << 20)
        : block_size_(block_size) {
        blocks_.emplace_back(std::make_unique<uint8_t[]>(block_size_));
    }

    void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t)) {
        size_t current = offset_;
        size_t aligned = (current + alignment - 1) & ~(alignment - 1);
        if (aligned + bytes > block_size_) {
            blocks_.emplace_back(std::make_unique<uint8_t[]>(block_size_));
            offset_ = 0;
            aligned = 0;
        }
        void* ptr = blocks_.back().get() + aligned;
        offset_ = aligned + bytes;
        return ptr;
    }

    void reset() {
        if (!blocks_.empty()) {
            blocks_.resize(1);
        }
        offset_ = 0;
    }

private:
    size_t block_size_;
    size_t offset_ = 0;
    std::vector<std::unique_ptr<uint8_t[]>> blocks_;
};

template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(size_t capacity = 1024) {
        reserve(capacity);
    }

    T* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (free_.empty()) {
            reserve(chunks_.empty() ? 1024 : chunks_.size() * 2 * chunk_size_);
        }
        T* ptr = free_.back();
        free_.pop_back();
        return ptr;
    }

    void deallocate(T* ptr) {
        if (!ptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        free_.push_back(ptr);
    }

private:
    void reserve(size_t capacity) {
        size_t count = capacity;
        auto block = std::make_unique<T[]>(count);
        for (size_t i = 0; i < count; ++i) {
            free_.push_back(&block[i]);
        }
        chunks_.push_back(std::move(block));
        chunk_size_ = count;
    }

    std::mutex mutex_;
    std::vector<std::unique_ptr<T[]>> chunks_;
    std::vector<T*> free_;
    size_t chunk_size_ = 0;
};

}  // namespace regimeflow::common
