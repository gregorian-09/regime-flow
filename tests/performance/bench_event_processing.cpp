#include "regimeflow/events/event_queue.h"
#include "regimeflow/events/event.h"
#include "regimeflow/data/bar.h"

#include <chrono>
#include <iostream>

int main() {
    constexpr int kEvents = 500000;
    regimeflow::events::EventQueue queue;
    auto symbol = regimeflow::SymbolRegistry::instance().intern("BENCH");

    regimeflow::data::Bar bar;
    bar.symbol = symbol;
    bar.open = bar.high = bar.low = bar.close = 1.0;
    bar.volume = 1;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kEvents; ++i) {
        bar.timestamp = regimeflow::Timestamp(static_cast<int64_t>(i));
        queue.push(regimeflow::events::make_market_event(bar));
    }
    int popped = 0;
    while (auto evt = queue.pop()) {
        (void)evt;
        ++popped;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    double eps = static_cast<double>(popped) / elapsed.count();
    std::cout << "Event processing: " << eps << " events/sec" << std::endl;
    return 0;
}
