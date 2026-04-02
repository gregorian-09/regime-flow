#include <regimeflow/common/config.h>
#include <regimeflow/engine/backtest_engine.h>

int main() {
    regimeflow::engine::BacktestEngine engine{100000.0, "USD"};
    return engine.portfolio().cash() > 0.0 ? 0 : 1;
}
