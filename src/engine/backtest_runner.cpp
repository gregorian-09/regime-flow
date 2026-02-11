#include "regimeflow/engine/backtest_runner.h"

#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/engine/engine_factory.h"
#include "regimeflow/strategy/strategy_factory.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace regimeflow::engine {

BacktestRunner::BacktestRunner(BacktestEngine* engine, data::DataSource* data_source)
    : engine_(engine), data_source_(data_source) {}

BacktestResults BacktestRunner::run(std::unique_ptr<strategy::Strategy> strategy,
                                    const TimeRange& range,
                                    const std::vector<SymbolId>& symbols,
                                    data::BarType bar_type) {
    if (!engine_ || !data_source_) {
        return {};
    }

    auto bar_iterator = data_source_->create_iterator(symbols, range, bar_type);
    auto tick_iterator = data_source_->create_tick_iterator(symbols, range);
    auto book_iterator = data_source_->create_book_iterator(symbols, range);
    engine_->load_data(std::move(bar_iterator), std::move(tick_iterator),
                       std::move(book_iterator));
    engine_->set_strategy(std::move(strategy));
    engine_->run();

    return engine_->results();
}

namespace {

BacktestResults run_spec(const BacktestRunSpec& spec) {
    auto engine = EngineFactory::create(spec.engine_config);
    if (!engine) {
        throw std::runtime_error("Failed to create backtest engine");
    }
    auto data_source = data::DataSourceFactory::create(spec.data_config);
    if (!data_source) {
        throw std::runtime_error("Failed to create data source");
    }
    auto strategy = strategy::StrategyFactory::instance().create(spec.strategy_config);
    if (!strategy) {
        throw std::runtime_error("Failed to create strategy");
    }

    std::vector<SymbolId> symbol_ids;
    symbol_ids.reserve(spec.symbols.size());
    for (const auto& sym : spec.symbols) {
        symbol_ids.push_back(SymbolRegistry::instance().intern(sym));
    }

    BacktestRunner runner(engine.get(), data_source.get());
    return runner.run(std::move(strategy), spec.range, symbol_ids, spec.bar_type);
}

}  // namespace

std::vector<BacktestResults> BacktestRunner::run_parallel(
    const std::vector<BacktestRunSpec>& runs,
    int num_threads) {
    if (runs.empty()) {
        return {};
    }
    if (num_threads <= 0) {
        num_threads = static_cast<int>(std::thread::hardware_concurrency());
        if (num_threads <= 0) {
            num_threads = 1;
        }
    }

    std::vector<BacktestResults> results(runs.size());
    std::atomic<size_t> index{0};
    std::exception_ptr failure;
    std::mutex failure_mutex;

    auto worker = [&]() {
        while (true) {
            size_t i = index.fetch_add(1, std::memory_order_relaxed);
            if (i >= runs.size()) {
                break;
            }
            try {
                results[i] = run_spec(runs[i]);
            } catch (...) {
                std::lock_guard<std::mutex> lock(failure_mutex);
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(num_threads));
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }
    if (failure) {
        std::rethrow_exception(failure);
    }
    return results;
}

}  // namespace regimeflow::engine
