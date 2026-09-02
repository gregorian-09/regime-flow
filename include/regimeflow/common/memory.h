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
            auto* base = reinterpret_cast<std::byte*>(block.data.get());
            void* ptr = static_cast<void*>(base + offset_);
            size_t space = block.size - offset_;
            if (std::align(alignment, bytes, ptr, space) == nullptr) {
                return nullptr;
            }

            const auto* aligned = static_cast<const std::byte*>(ptr);
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
        explicit PoolAllocator(size_t capacity = 1024)
            : initial_capacity_(std::max<size_t>(capacity, 1)) {
            reserve(initial_capacity_);
        }

        /**
         * @brief Allocate an object from the pool.
         * @return Pointer to an available object.
         */
        T* allocate() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (free_.empty()) {
                if (!chunks_.empty()
                    && chunk_size_ > std::numeric_limits<size_t>::max() / 2) {
                    throw std::bad_alloc();
                }
                reserve(chunks_.empty() ? initial_capacity_ : chunk_size_ * 2);
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

        /**
         * @brief Release spare chunks when every allocation has been returned.
         * @return True when memory was released; false if callers still own objects.
         *
         * The pool deliberately never relocates live objects.  Call this at a
         * quiescent lifecycle boundary such as queue clear or shutdown.
         */
        [[nodiscard]] bool release_unused() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (chunks_.size() <= 1 || free_.size() != total_capacity_) {
                return false;
            }

            chunks_.resize(1);
            free_.clear();
            const size_t retained_capacity = initial_capacity_;
            for (size_t i = 0; i < retained_capacity; ++i) {
                free_.push_back(&chunks_.front()[i]);
            }
            total_capacity_ = retained_capacity;
            chunk_size_ = retained_capacity;
            return true;
        }

        /**
         * @brief Return the number of objects currently retained by the pool.
         */
        [[nodiscard]] size_t retained_capacity() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return total_capacity_;
        }

    private:
        void reserve(size_t capacity) {
            size_t count = capacity;
            if (count > std::numeric_limits<size_t>::max() - total_capacity_) {
                throw std::bad_alloc();
            }
            auto block = std::make_unique<T[]>(count);
            for (size_t i = 0; i < count; ++i) {
                free_.push_back(&block[i]);
            }
            chunks_.push_back(std::move(block));
            chunk_size_ = count;
            total_capacity_ += count;
        }

        mutable std::mutex mutex_;
        std::vector<std::unique_ptr<T[]>> chunks_;
        std::vector<T*> free_;
        const size_t initial_capacity_ = 1024;
        size_t chunk_size_ = 0;
        size_t total_capacity_ = 0;
    };
}  // namespace regimeflow::common
