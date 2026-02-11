#include "regimeflow/common/memory.h"

#include <chrono>
#include <iostream>
#include <vector>

namespace {

template<typename F>
double run_stage(const char* name, F&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << name << ": " << elapsed.count() << "s" << std::endl;
    return elapsed.count();
}

}  // namespace

int main() {
    constexpr size_t kItems = 500000;

    run_stage("Stage 1 (baseline new/delete)", [] {
        std::vector<int*> pointers;
        pointers.reserve(kItems);
        for (size_t i = 0; i < kItems; ++i) {
            pointers.push_back(new int(static_cast<int>(i)));
        }
        for (auto* ptr : pointers) {
            delete ptr;
        }
    });

    run_stage("Stage 2 (PoolAllocator)", [] {
        regimeflow::common::PoolAllocator<int> pool(4096);
        std::vector<int*> pointers;
        pointers.reserve(kItems);
        for (size_t i = 0; i < kItems; ++i) {
            auto* ptr = pool.allocate();
            new (ptr) int(static_cast<int>(i));
            pointers.push_back(ptr);
        }
        for (auto* ptr : pointers) {
            pool.deallocate(ptr);
        }
    });

    run_stage("Stage 3 (MonotonicArena)", [] {
        regimeflow::common::MonotonicArena arena(1024 * 1024 * 8);
        std::vector<int*> pointers;
        pointers.reserve(kItems);
        for (size_t i = 0; i < kItems; ++i) {
            auto* ptr = static_cast<int*>(arena.allocate(sizeof(int), alignof(int)));
            if (!ptr) {
                break;
            }
            new (ptr) int(static_cast<int>(i));
            pointers.push_back(ptr);
        }
        arena.reset();
    });

    return 0;
}
