"""RegimeFlow - Regime-adaptive backtesting framework.

Quick example:
    engine = BacktestEngine(config)
    results = engine.run(Strategy())
    summary = results.report_json()
    csv_text = results.report_csv()
"""

from . import _core as _core

from ._core import (
    Config,
    load_config,
    Timestamp,
    Order,
    OrderSide,
    OrderType,
    OrderStatus,
    TimeInForce,
    Fill,
    RegimeType,
    RegimeState,
    RegimeTransition,
    register_strategy,
)

engine = _core.engine
data = _core.data
strategy = _core.strategy
core_strategy = _core.strategy
walkforward = _core.walkforward

def __getattr__(name):
    if name == "visualization":
        from . import visualization as _visualization
        return _visualization
    if name == "analysis":
        from . import analysis as _analysis
        return _analysis
    if name == "strategy_module":
        from . import strategy as _strategy
        return _strategy
    if name == "config":
        from . import config as _config
        return _config
    raise AttributeError(name)

BacktestEngine = engine.BacktestEngine
BacktestConfig = engine.BacktestConfig
BacktestResults = engine.BacktestResults
Portfolio = engine.Portfolio
Position = engine.Position
ParameterDef = walkforward.ParameterDef
WalkForwardConfig = walkforward.WalkForwardConfig
WalkForwardOptimizer = walkforward.WalkForwardOptimizer
WalkForwardResults = walkforward.WalkForwardResults
WindowResult = walkforward.WindowResult

Bar = data.Bar
Tick = data.Tick
Quote = data.Quote
OrderBook = data.OrderBook
BookLevel = data.BookLevel
BarType = data.BarType

Strategy = strategy.Strategy
StrategyContext = strategy.StrategyContext

__all__ = [
    "Timestamp",
    "Config",
    "load_config",
    "Order",
    "OrderSide",
    "OrderType",
    "OrderStatus",
    "TimeInForce",
    "Fill",
    "RegimeType",
    "RegimeState",
    "RegimeTransition",
    "register_strategy",
    "BacktestEngine",
    "BacktestConfig",
    "BacktestResults",
    "Portfolio",
    "Position",
    "Bar",
    "Tick",
    "Quote",
    "OrderBook",
    "BookLevel",
    "BarType",
    "Strategy",
    "StrategyContext",
    "walkforward",
    "core_strategy",
    "strategy_module",
    "ParameterDef",
    "WalkForwardConfig",
    "WalkForwardOptimizer",
    "WalkForwardResults",
    "WindowResult",
    "visualization",
    "analysis",
    "config",
]

__version__ = "0.1.0"
