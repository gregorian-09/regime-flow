#include <gtest/gtest.h>

#include "regimeflow/common/config.h"
#include "regimeflow/engine/backtest_runner.h"

#include <filesystem>

namespace regimeflow::test {

TEST(BacktestRunnerParallel, RunsMultipleSpecs) {
    Config engine_cfg;
    engine_cfg.set_path("engine.initial_capital", 100000.0);
    engine_cfg.set_path("engine.currency", "USD");

    Config data_cfg;
    data_cfg.set("type", "csv");
    data_cfg.set("file_pattern", "{symbol}.csv");
    data_cfg.set("has_header", true);
    data_cfg.set("data_directory",
                 (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string());

    Config strategy_cfg;
    strategy_cfg.set("type", "buy_and_hold");
    strategy_cfg.set("symbol", "TEST");
    strategy_cfg.set("quantity", 1.0);

    engine::BacktestRunSpec spec;
    spec.engine_config = engine_cfg;
    spec.data_config = data_cfg;
    spec.strategy_config = strategy_cfg;
    spec.range.start = Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    spec.range.end = Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");
    spec.symbols = {"TEST"};

    auto results = engine::BacktestRunner::run_parallel({spec, spec}, 2);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_NEAR(results[0].total_return, results[1].total_return, 1e-12);
}

class ParamStrategy final : public strategy::Strategy {
public:
    explicit ParamStrategy(double qty) : qty_(qty) {}

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

TEST(BacktestEngineParallel, RunsWithFactoryAndContext) {
    engine::BacktestEngine engine(100000.0);

    engine::ParallelContext ctx;
    ctx.data_config.set("type", "csv");
    ctx.data_config.set("file_pattern", "{symbol}.csv");
    ctx.data_config.set("has_header", true);
    ctx.data_config.set("data_directory",
                        (std::filesystem::path(REGIMEFLOW_TEST_ROOT) / "tests/fixtures").string());
    ctx.range.start = Timestamp::from_string("2020-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    ctx.range.end = Timestamp::from_string("2020-01-03 00:00:00", "%Y-%m-%d %H:%M:%S");
    ctx.symbols = {"TEST"};
    engine.set_parallel_context(std::move(ctx));

    std::vector<std::map<std::string, double>> params = {
        {{"qty", 1.0}},
        {{"qty", 2.0}}
    };

    auto results = engine.run_parallel(
        params,
        [](const std::map<std::string, double>& param_set) {
            auto it = param_set.find("qty");
            double qty = (it == param_set.end()) ? 1.0 : it->second;
            return std::make_unique<ParamStrategy>(qty);
        },
        2);

    ASSERT_EQ(results.size(), 2u);
}

}  // namespace regimeflow::test
