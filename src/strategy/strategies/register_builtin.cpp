#include "regimeflow/strategy/strategy_factory.h"
#include "regimeflow/strategy/strategies/buy_and_hold.h"
#include "regimeflow/strategy/strategies/harmonic_pattern.h"
#include "regimeflow/strategy/strategies/moving_average_cross.h"
#include "regimeflow/strategy/strategies/pairs_trading.h"

namespace regimeflow::strategy {

namespace {

struct BuiltinStrategyRegistrar {
    BuiltinStrategyRegistrar() {
        StrategyFactory::instance().register_creator(
            "buy_and_hold",
            [](const Config&) { return std::make_unique<BuyAndHoldStrategy>(); });
        StrategyFactory::instance().register_creator(
            "moving_average_cross",
            [](const Config&) { return std::make_unique<MovingAverageCrossStrategy>(); });
        StrategyFactory::instance().register_creator(
            "harmonic_pattern",
            [](const Config&) { return std::make_unique<HarmonicPatternStrategy>(); });
        StrategyFactory::instance().register_creator(
            "pairs_trading",
            [](const Config&) { return std::make_unique<PairsTradingStrategy>(); });
    }
};

const BuiltinStrategyRegistrar registrar;

}  // namespace

void register_builtin_strategies() {
    (void)registrar;
}

}  // namespace regimeflow::strategy
