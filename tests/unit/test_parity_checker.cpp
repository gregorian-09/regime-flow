#include "regimeflow/engine/parity_checker.h"

#include <gtest/gtest.h>

using regimeflow::Config;
using regimeflow::ConfigValue;
using regimeflow::engine::ParityChecker;
using regimeflow::engine::ParityStatus;

TEST(ParityCheckerTest, MissingLiveStrategyFails) {
    Config backtest;
    backtest.set_path("strategy.name", "buy_and_hold");
    Config live;

    auto report = ParityChecker::check(backtest, live);
    EXPECT_EQ(report.status, ParityStatus::Fail);
    EXPECT_FALSE(report.hard_errors.empty());
}

TEST(ParityCheckerTest, MatchingStrategyPasses) {
    Config backtest;
    backtest.set_path("strategy.name", "buy_and_hold");
    Config live;
    live.set_path("strategy.name", "buy_and_hold");

    auto report = ParityChecker::check(backtest, live);
    EXPECT_NE(report.status, ParityStatus::Fail);
    EXPECT_TRUE(report.hard_errors.empty());
}

TEST(ParityCheckerTest, RiskMismatchWarns) {
    Config backtest;
    backtest.set_path("strategy.name", "buy_and_hold");
    backtest.set_path("risk.limits.max_notional", ConfigValue(static_cast<int64_t>(100000)));

    Config live;
    live.set_path("strategy.name", "buy_and_hold");
    live.set_path("live.risk.limits.max_notional", ConfigValue(static_cast<int64_t>(50000)));

    auto report = ParityChecker::check(backtest, live);
    EXPECT_EQ(report.status, ParityStatus::Warn);
    EXPECT_FALSE(report.warnings.empty());
}
