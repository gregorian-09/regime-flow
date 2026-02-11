from __future__ import annotations

from typing import Any, Callable

from .. import Strategy, StrategyContext, register_strategy


class StrategyRegistry:
    """Lightweight registry wrapper for Python strategies."""

    @staticmethod
    def register(name: str, strategy_cls: type[Strategy]) -> None:
        if not issubclass(strategy_cls, Strategy):
            raise TypeError("strategy_cls must inherit from regimeflow.Strategy")
        register_strategy(name, strategy_cls)

    @staticmethod
    def decorator(name: str) -> Callable[[type[Strategy]], type[Strategy]]:
        def _wrap(cls: type[Strategy]) -> type[Strategy]:
            StrategyRegistry.register(name, cls)
            return cls
        return _wrap


__all__ = [
    "Strategy",
    "StrategyContext",
    "register_strategy",
    "StrategyRegistry",
]
