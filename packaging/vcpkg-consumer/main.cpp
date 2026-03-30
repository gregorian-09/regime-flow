#include <regimeflow/common/config.h>
#include <regimeflow/engine/backtest_engine.h>
#include <regimeflow/strategy/strategy_factory.h>

int main() {
    regimeflow::Config strategy_config;
    strategy_config.set("name", "buy_and_hold");

    regimeflow::strategy::register_builtin_strategies();
    auto strategy = regimeflow::strategy::StrategyFactory::instance().create(strategy_config);
    if (!strategy) {
        return 1;
    }

    regimeflow::engine::BacktestEngine engine{100000.0, "USD"};
    return engine.portfolio().cash() > 0.0 ? 0 : 1;
}
