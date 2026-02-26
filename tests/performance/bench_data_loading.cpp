#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/common/time.h"

#include <chrono>
#include <iostream>

int main() {
    constexpr int kBars = 500000;
    const auto source = std::make_shared<regimeflow::data::MemoryDataSource>();
    auto symbol = regimeflow::SymbolRegistry::instance().intern("BENCH");

    std::vector<regimeflow::data::Bar> bars;
    bars.reserve(kBars);
    for (int i = 0; i < kBars; ++i) {
        regimeflow::data::Bar bar;
        bar.symbol = symbol;
        bar.timestamp = regimeflow::Timestamp(static_cast<int64_t>(i));
        bar.open = bar.high = bar.low = bar.close = 1.0;
        bar.volume = 1;
        bars.push_back(bar);
    }
    source->add_bars(symbol, bars);

    const regimeflow::TimeRange range{regimeflow::Timestamp(0), regimeflow::Timestamp(kBars)};

    const auto start = std::chrono::high_resolution_clock::now();
    const auto iter = source->create_iterator({symbol}, range, regimeflow::data::BarType::Time_1Min);
    int count = 0;
    while (iter->has_next()) {
        iter->next();
        ++count;
    }
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end - start;

    const double eps = static_cast<double>(count) / elapsed.count();
    std::cout << "Data loading: " << eps << " bars/sec" << std::endl;
    return 0;
}
