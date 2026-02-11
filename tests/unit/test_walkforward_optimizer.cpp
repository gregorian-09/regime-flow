#include <gtest/gtest.h>

#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/strategy/strategy.h"
#include "regimeflow/walkforward/optimizer.h"

namespace regimeflow::test {

class QtyStrategy final : public strategy::Strategy {
public:
    explicit QtyStrategy(double qty) : qty_(qty) {}

    void initialize(strategy::StrategyContext&) override {}

    void on_bar(const data::Bar& bar) override {
        if (sent_) {
            return;
        }
        engine::Order order = engine::Order::market(bar.symbol, engine::OrderSide::Buy, qty_);
        ctx_->submit_order(std::move(order));
        sent_ = true;
    }

private:
    double qty_ = 1.0;
    bool sent_ = false;
};

class DummyRegimeDetector final : public regime::RegimeDetector {
public:
    regime::RegimeState on_bar(const data::Bar& bar) override {
        regime::RegimeState state;
        state.timestamp = bar.timestamp;
        return state;
    }
    regime::RegimeState on_tick(const data::Tick& tick) override {
        regime::RegimeState state;
        state.timestamp = tick.timestamp;
        return state;
    }
};

static std::vector<data::Bar> build_bars(SymbolId symbol, int days, double start_price) {
    std::vector<data::Bar> bars;
    bars.reserve(static_cast<size_t>(days));
    auto base = Timestamp::from_date(2020, 1, 1);
    for (int i = 0; i < days; ++i) {
        data::Bar bar{};
        bar.symbol = symbol;
        bar.timestamp = base + Duration::days(i);
        double price = start_price + static_cast<double>(i);
        bar.open = price;
        bar.high = price;
        bar.low = price;
        bar.close = price;
        bar.volume = 100;
        bars.push_back(bar);
    }
    return bars;
}

TEST(WalkForwardOptimizer, GridStitchesOOS) {
    SymbolRegistry::instance().intern("DUMMY");
    auto symbol = SymbolRegistry::instance().intern("WFO");
    ASSERT_NE(symbol, 0u);
    data::MemoryDataSource source;
    data::SymbolInfo info{};
    info.id = symbol;
    info.ticker = "WFO";
    source.add_symbol_info(info);
    source.add_bars(symbol, build_bars(symbol, 120, 100.0));

    walkforward::WalkForwardConfig cfg;
    cfg.window_type = walkforward::WalkForwardConfig::WindowType::Rolling;
    cfg.in_sample_period = Duration::days(30);
    cfg.out_of_sample_period = Duration::days(15);
    cfg.step_size = Duration::days(15);
    cfg.optimization_method = walkforward::WalkForwardConfig::OptMethod::Grid;
    cfg.fitness_metric = "return";
    cfg.num_parallel_backtests = 2;
    cfg.initial_capital = 100000.0;

    walkforward::ParameterDef def;
    def.name = "qty";
    def.type = walkforward::ParameterDef::Type::Int;
    def.min_value = 1;
    def.max_value = 3;
    def.step = 1;

    walkforward::WalkForwardOptimizer optimizer(cfg);
    TimeRange range;
    range.start = Timestamp::from_date(2020, 1, 1);
    range.end = Timestamp::from_date(2020, 4, 29);
    auto results = optimizer.optimize(
        {def},
        [](const walkforward::ParameterSet& params) {
            auto it = params.find("qty");
            double qty = 1.0;
            if (it != params.end()) {
                if (auto* v = std::get_if<int>(&it->second)) {
                    qty = static_cast<double>(*v);
                } else if (auto* v2 = std::get_if<double>(&it->second)) {
                    qty = *v2;
                }
            }
            return std::make_unique<QtyStrategy>(qty);
        },
        &source,
        range);

    ASSERT_GE(results.windows.size(), 2u);
    auto it = results.windows.front().optimal_params.find("qty");
    ASSERT_TRUE(it != results.windows.front().optimal_params.end());
    EXPECT_EQ(std::get<int>(it->second), 3);
    EXPECT_GT(results.stitched_oos_results.total_return, 0.0);
}

TEST(WalkForwardOptimizer, RegimeTrainingHookInvoked) {
    auto symbol = SymbolRegistry::instance().intern("WFOH");
    data::MemoryDataSource source;
    data::SymbolInfo info{};
    info.id = symbol;
    info.ticker = "WFOH";
    source.add_symbol_info(info);
    source.add_bars(symbol, build_bars(symbol, 60, 100.0));

    walkforward::WalkForwardConfig cfg;
    cfg.window_type = walkforward::WalkForwardConfig::WindowType::Rolling;
    cfg.in_sample_period = Duration::days(20);
    cfg.out_of_sample_period = Duration::days(10);
    cfg.step_size = Duration::days(10);
    cfg.optimization_method = walkforward::WalkForwardConfig::OptMethod::Grid;
    cfg.fitness_metric = "return";
    cfg.retrain_regime_each_window = true;

    walkforward::WalkForwardOptimizer optimizer(cfg);
    int hook_calls = 0;
    optimizer.on_regime_train([&](const walkforward::WalkForwardOptimizer::RegimeTrainingContext& ctx) {
        EXPECT_TRUE(ctx.data_source != nullptr);
        EXPECT_TRUE(ctx.detector != nullptr);
        hook_calls += 1;
        return true;
    });

    TimeRange range;
    range.start = Timestamp::from_date(2020, 1, 1);
    range.end = Timestamp::from_date(2020, 3, 1);

    walkforward::ParameterDef def;
    def.name = "qty";
    def.type = walkforward::ParameterDef::Type::Int;
    def.min_value = 1;
    def.max_value = 1;
    def.step = 1;

    optimizer.optimize(
        {def},
        [](const walkforward::ParameterSet&) { return std::make_unique<QtyStrategy>(1.0); },
        &source,
        range,
        [] { return std::make_unique<DummyRegimeDetector>(); });

    EXPECT_GT(hook_calls, 0);
}

TEST(WalkForwardOptimizer, ParallelRandomDeterministic) {
    SymbolRegistry::instance().intern("DUMMY2");
    auto symbol = SymbolRegistry::instance().intern("WFOD");
    ASSERT_NE(symbol, 0u);
    data::MemoryDataSource source;
    data::SymbolInfo info{};
    info.id = symbol;
    info.ticker = "WFOD";
    source.add_symbol_info(info);
    source.add_bars(symbol, build_bars(symbol, 90, 100.0));

    walkforward::WalkForwardConfig cfg;
    cfg.window_type = walkforward::WalkForwardConfig::WindowType::Rolling;
    cfg.in_sample_period = Duration::days(20);
    cfg.out_of_sample_period = Duration::days(10);
    cfg.step_size = Duration::days(10);
    cfg.optimization_method = walkforward::WalkForwardConfig::OptMethod::Random;
    cfg.max_trials = 6;
    cfg.fitness_metric = "return";
    cfg.num_parallel_backtests = 2;
    cfg.initial_capital = 100000.0;

    walkforward::ParameterDef def;
    def.name = "qty";
    def.type = walkforward::ParameterDef::Type::Int;
    def.min_value = 1;
    def.max_value = 4;
    def.step = 1;

    TimeRange range;
    range.start = Timestamp::from_date(2020, 1, 1);
    range.end = Timestamp::from_date(2020, 3, 30);

    walkforward::WalkForwardOptimizer optimizer_a(cfg);
    auto results_a = optimizer_a.optimize(
        {def},
        [](const walkforward::ParameterSet& params) {
            auto it = params.find("qty");
            double qty = 1.0;
            if (it != params.end()) {
                if (auto* v = std::get_if<int>(&it->second)) {
                    qty = static_cast<double>(*v);
                } else if (auto* v2 = std::get_if<double>(&it->second)) {
                    qty = *v2;
                }
            }
            return std::make_unique<QtyStrategy>(qty);
        },
        &source,
        range);

    walkforward::WalkForwardOptimizer optimizer_b(cfg);
    auto results_b = optimizer_b.optimize(
        {def},
        [](const walkforward::ParameterSet& params) {
            auto it = params.find("qty");
            double qty = 1.0;
            if (it != params.end()) {
                if (auto* v = std::get_if<int>(&it->second)) {
                    qty = static_cast<double>(*v);
                } else if (auto* v2 = std::get_if<double>(&it->second)) {
                    qty = *v2;
                }
            }
            return std::make_unique<QtyStrategy>(qty);
        },
        &source,
        range);

    ASSERT_EQ(results_a.windows.size(), results_b.windows.size());
    for (size_t i = 0; i < results_a.windows.size(); ++i) {
        const auto& a = results_a.windows[i];
        const auto& b = results_b.windows[i];
        auto it_a = a.optimal_params.find("qty");
        auto it_b = b.optimal_params.find("qty");
        ASSERT_TRUE(it_a != a.optimal_params.end());
        ASSERT_TRUE(it_b != b.optimal_params.end());
        EXPECT_EQ(std::get<int>(it_a->second), std::get<int>(it_b->second));
        EXPECT_NEAR(a.oos_fitness, b.oos_fitness, 1e-12);
        EXPECT_EQ(a.in_sample_range.start, b.in_sample_range.start);
        EXPECT_EQ(a.out_of_sample_range.start, b.out_of_sample_range.start);
    }
}

}  // namespace regimeflow::test
