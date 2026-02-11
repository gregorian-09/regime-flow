from __future__ import annotations

import argparse
import importlib
import json
import sys
from typing import Any

import regimeflow as rf


def _load_strategy(strategy_path: str) -> rf.Strategy:
    if ":" not in strategy_path:
        raise ValueError("Strategy must be in module:Class format")
    module_name, class_name = strategy_path.split(":", 1)
    module = importlib.import_module(module_name)
    cls = getattr(module, class_name, None)
    if cls is None:
        raise ValueError(f"Strategy class '{class_name}' not found in '{module_name}'")
    instance = cls()
    if not isinstance(instance, rf.Strategy):
        raise ValueError("Strategy class must inherit from regimeflow.Strategy")
    return instance


def _apply_strategy_params(cfg: rf.BacktestConfig, params_path: str | None) -> None:
    if not params_path:
        return
    with open(params_path, "r", encoding="utf-8") as f:
        params = json.load(f)
    if not isinstance(params, dict):
        raise ValueError("strategy params must be a JSON object")
    cfg.strategy_params = params


def _write_text(path: str, payload: str) -> None:
    with open(path, "w", encoding="utf-8") as f:
        f.write(payload)


def _write_dataframe(path: str, df: Any) -> None:
    df.to_csv(path, index=True)


def run_backtest(args: argparse.Namespace) -> int:
    cfg = rf.BacktestConfig.from_yaml(args.config)
    _apply_strategy_params(cfg, args.strategy_params)

    engine = rf.BacktestEngine(cfg)
    if ":" in args.strategy:
        strategy = _load_strategy(args.strategy)
        results = engine.run(strategy)
    else:
        # Treat as registered/builtin strategy name
        results = engine.run(args.strategy)

    if args.output_json:
        _write_text(args.output_json, results.report_json())
    if args.output_csv:
        _write_text(args.output_csv, results.report_csv())
    if args.output_equity:
        _write_dataframe(args.output_equity, results.equity_curve())
    if args.output_trades:
        _write_dataframe(args.output_trades, results.trades())

    if args.print_summary:
        summary = rf.analysis.performance_summary(results)
        if summary:
            print(json.dumps(summary, indent=2))

    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="regimeflow-backtest")
    parser.add_argument("--config", required=True, help="Path to YAML backtest config")
    parser.add_argument(
        "--strategy",
        required=True,
        help="Python strategy class as module:Class (must subclass regimeflow.Strategy)",
    )
    parser.add_argument(
        "--strategy-params",
        help="Optional JSON file with strategy params to inject into config",
    )
    parser.add_argument("--output-json", help="Write report JSON to path")
    parser.add_argument("--output-csv", help="Write report CSV to path")
    parser.add_argument("--output-equity", help="Write equity curve CSV to path")
    parser.add_argument("--output-trades", help="Write trades CSV to path")
    parser.add_argument(
        "--print-summary",
        action="store_true",
        help="Print performance summary JSON to stdout",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return run_backtest(args)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
