#include "regimeflow/common/memory.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
    template<typename F>
    double run_stage(const char* name, F&& func) {
        const auto start = std::chrono::high_resolution_clock::now();
        func();
        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        std::cout << name << ": " << elapsed.count() << "s" << '\n';
        return elapsed.count();
    }
}  // namespace

int main() {
    constexpr size_t kItems = 500000;

    const auto baseline_seconds = run_stage("Stage 1 (baseline new/delete)", [] {
        std::vector<int*> pointers;
        pointers.reserve(kItems);
        for (size_t i = 0; i < kItems; ++i) {
            pointers.push_back(new int(static_cast<int>(i)));
        }
        if (pointers.size() != kItems) {
            std::abort();
        }
        for (const auto* ptr : pointers) {
            delete ptr;
        }
    });

    const auto pool_seconds = run_stage("Stage 2 (PoolAllocator)", [] {
        regimeflow::common::PoolAllocator<int> pool(4096);
        std::vector<int*> pointers;
        pointers.reserve(kItems);
        long long checksum = 0;
        for (size_t i = 0; i < kItems; ++i) {
            auto* ptr = pool.allocate();
            new (ptr) int(static_cast<int>(i));
            checksum += *ptr;
            pointers.push_back(ptr);
        }
        const auto expected = static_cast<long long>(kItems - 1) * static_cast<long long>(kItems) / 2;
        if (pointers.size() != kItems || checksum != expected) {
            std::abort();
        }
        for (auto* ptr : pointers) {
            pool.deallocate(ptr);
        }
    });

    const auto arena_seconds = run_stage("Stage 3 (MonotonicArena)", [] {
        regimeflow::common::MonotonicArena arena(1024 * 1024 * 8);
        std::vector<int*> pointers;
        pointers.reserve(kItems);
        long long checksum = 0;
        for (size_t i = 0; i < kItems; ++i) {
            auto* ptr = static_cast<int*>(arena.allocate(sizeof(int), alignof(int)));
            if (!ptr) {
                break;
            }
            new (ptr) int(static_cast<int>(i));
            checksum += *ptr;
            pointers.push_back(ptr);
        }
        const auto expected = static_cast<long long>(kItems - 1) * static_cast<long long>(kItems) / 2;
        if (pointers.size() != kItems || checksum != expected) {
            std::abort();
        }
        arena.reset();
    });

    if (baseline_seconds <= 0.0 || pool_seconds <= 0.0 || arena_seconds <= 0.0) {
        std::cerr << "Allocator benchmark failed sanity checks" << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
