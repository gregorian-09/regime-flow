#include "regimeflow/strategy/strategy_factory.h"
#include "regimeflow/strategy/strategies/buy_and_hold.h"
#include "regimeflow/strategy/strategies/moving_average_cross.h"

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
    }
};

const BuiltinStrategyRegistrar registrar;

}  // namespace

void register_builtin_strategies() {
    (void)registrar;
}

}  // namespace regimeflow::strategy
