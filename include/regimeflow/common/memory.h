/**
 * @file memory.h
 * @brief RegimeFlow regimeflow memory declarations.
 */

#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

namespace regimeflow::common
{
    /**
     * @brief Simple monotonic arena allocator.
     *
     * @details Allocations are fast and only freed by resetting the arena.
     * Useful for batch-style workloads like backtests and parsing.
     */
    class MonotonicArena {
    public:
        /**
         * @brief Construct the arena with a block size.
         * @param block_size Bytes per block.
         */
        explicit MonotonicArena(size_t block_size = 1 << 20)
            : block_size_(block_size) {
            add_block(block_size_);
        }

        /**
         * @brief Allocate a block of memory from the arena.
         * @param bytes Number of bytes to allocate.
         * @param alignment Requested alignment (defaults to max_align_t).
         * @return Pointer to allocated memory.
         */
        void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t)) {
            bytes = std::max<size_t>(bytes, 1);
            alignment = normalize_alignment(alignment);
            if (bytes > std::numeric_limits<size_t>::max() - alignment) {
                throw std::bad_alloc();
            }

            if (void* ptr = try_allocate(bytes, alignment)) {
                return ptr;
            }

            add_block(std::max(block_size_, bytes + alignment - 1));
            if (void* ptr = try_allocate(bytes, alignment)) {
                return ptr;
            }
            throw std::bad_alloc();
        }

        /**
         * @brief Reset the arena, freeing all allocations.
         */
        void reset() {
            if (!blocks_.empty()) {
                blocks_.resize(1);
            }
            offset_ = 0;
        }

    private:
        struct Block {
            std::unique_ptr<uint8_t[]> data;
            size_t size = 0;
        };

        static size_t normalize_alignment(size_t alignment) {
            if (alignment == 0) {
                return alignof(std::max_align_t);
            }
            if (!std::has_single_bit(alignment)) {
                constexpr size_t max_power_of_two = size_t{1} << (std::numeric_limits<size_t>::digits - 1);
                if (alignment > max_power_of_two) {
                    throw std::bad_alloc();
                }
                return std::bit_ceil(alignment);
            }
            return alignment;
        }

        void add_block(size_t size) {
            blocks_.push_back(Block{std::make_unique<uint8_t[]>(size), size});
            offset_ = 0;
        }

        void* try_allocate(size_t bytes, size_t alignment) {
            Block& block = blocks_.back();
            void* ptr = block.data.get() + offset_;
            size_t space = block.size - offset_;
            if (std::align(alignment, bytes, ptr, space) == nullptr) {
                return nullptr;
            }

            const auto* base = block.data.get();
            const auto* aligned = static_cast<const uint8_t*>(ptr);
            offset_ = static_cast<size_t>(aligned - base) + bytes;
            return ptr;
        }

        size_t block_size_;
        size_t offset_ = 0;
        std::vector<Block> blocks_;
    };

    template<typename T>
    /**
     * @brief Thread-safe object pool allocator.
     * @tparam T Object type.
     *
     * @details Keeps a free list of objects and grows in chunks.
     */
    class PoolAllocator {
    public:
        /**
         * @brief Construct the pool with an initial capacity.
         * @param capacity Number of objects to pre-allocate.
         */
        explicit PoolAllocator(size_t capacity = 1024) {
            reserve(capacity);
        }

        /**
         * @brief Allocate an object from the pool.
         * @return Pointer to an available object.
         */
        T* allocate() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (free_.empty()) {
                reserve(chunks_.empty() ? 1024 : chunks_.size() * 2 * chunk_size_);
            }
            T* ptr = free_.back();
            free_.pop_back();
            return ptr;
        }

        /**
         * @brief Return an object to the pool.
         * @param ptr Pointer to the object to recycle.
         */
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
