from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import regimeflow as rf


@dataclass
class ParityResult:
    status: str
    hard_errors: list[str]
    warnings: list[str]
    backtest_values: dict[str, str]
    live_values: dict[str, str]
    compat_matrix: dict[str, str]


def parity_check(backtest_config: rf.Config | dict[str, Any],
                 live_config: rf.Config | dict[str, Any]) -> ParityResult:
    if isinstance(backtest_config, dict):
        backtest_cfg = rf.Config(backtest_config)
    else:
        backtest_cfg = backtest_config
    if isinstance(live_config, dict):
        live_cfg = rf.Config(live_config)
    else:
        live_cfg = live_config

    report = rf._core.parity_check(backtest_cfg, live_cfg)
    return ParityResult(
        status=report.get("status", "unknown"),
        hard_errors=list(report.get("hard_errors", [])),
        warnings=list(report.get("warnings", [])),
        backtest_values=dict(report.get("backtest_values", {})),
        live_values=dict(report.get("live_values", {})),
        compat_matrix=dict(report.get("compat_matrix", {})),
    )


class ResearchSession:
    def __init__(self, config_path: str | None = None, config: dict[str, Any] | None = None):
        self._config_path = config_path
        self._raw_config = None
        self._backtest_config = None

        if config_path:
            self._raw_config = rf.load_config(config_path)
            self._backtest_config = rf.BacktestConfig.from_yaml(config_path)
        elif config is not None:
            self._raw_config = rf.Config(config)
            self._backtest_config = rf.BacktestConfig.from_dict(config)

    @property
    def backtest_config(self) -> rf.BacktestConfig | None:
        return self._backtest_config

    @property
    def raw_config(self) -> rf.Config | None:
        return self._raw_config

    def run_backtest(self, strategy: rf.Strategy | str) -> rf.BacktestResults:
        if self._backtest_config is None:
            raise ValueError("No backtest config loaded")
        engine = rf.BacktestEngine(self._backtest_config)
        return engine.run(strategy)

    def parity_check(self, live_config_path: str | None = None,
                     live_config: dict[str, Any] | None = None) -> ParityResult:
        if self._raw_config is None:
            raise ValueError("No backtest config loaded")
        if live_config_path:
            live_cfg = rf.load_config(live_config_path)
        elif live_config is not None:
            live_cfg = rf.Config(live_config)
        else:
            raise ValueError("live_config_path or live_config is required")
        return parity_check(self._raw_config, live_cfg)
