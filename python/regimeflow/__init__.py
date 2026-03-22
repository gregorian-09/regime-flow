"""RegimeFlow - Regime-adaptive backtesting framework.

Quick example:
    engine = BacktestEngine(config)
    results = engine.run(Strategy())
    summary = results.report_json()
    csv_text = results.report_csv()
"""

import importlib
import os
import sys
import importlib.util

_LOCAL_PACKAGE_DIR = os.path.dirname(__file__)
if "__path__" in globals():
    _normalized_path = [_LOCAL_PACKAGE_DIR]
    for _entry in list(__path__):
        if os.path.abspath(_entry) != os.path.abspath(_LOCAL_PACKAGE_DIR):
            _normalized_path.append(_entry)
    __path__ = _normalized_path

_CORE_MODULE_NAME = f"{__name__}._core"
_core_spec = importlib.util.spec_from_file_location(
    _CORE_MODULE_NAME, os.path.join(_LOCAL_PACKAGE_DIR, "_core.py")
)
if _core_spec is None or _core_spec.loader is None:
    raise ImportError("Failed to load regimeflow._core wrapper")
_core = importlib.util.module_from_spec(_core_spec)
sys.modules[_CORE_MODULE_NAME] = _core
_core_spec.loader.exec_module(_core)

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
        return importlib.import_module(f"{__name__}.visualization")
    if name == "analysis":
        return importlib.import_module(f"{__name__}.analysis")
    if name == "strategy_module":
        return importlib.import_module(f"{__name__}.strategy")
    if name == "metrics":
        return importlib.import_module(f"{__name__}.metrics")
    if name == "config":
        return importlib.import_module(f"{__name__}.config")
    if name == "research":
        return importlib.import_module(f"{__name__}.research")
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
    "research",
    "metrics",
]

__version__ = "1.0.1"
