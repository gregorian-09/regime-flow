# Python CLI

The Python package installs a CLI entry point:

```bash
regimeflow-backtest
```

It is implemented in `python/regimeflow/cli.py` and is intended for Python-facing
backtest runs that export reports and summaries.

## What The CLI Is For

Use the CLI when you want:

- a reproducible command for a Python-run backtest
- report export without writing a wrapper script
- a quick bridge from YAML config to JSON/CSV outputs
- Python strategy loading through `module:Class`

## Command Form

```bash
regimeflow-backtest --config <path> --strategy <strategy>
```

Equivalent module form:

```bash
.venv/bin/python -m regimeflow.cli --config <path> --strategy <strategy>
```

## Arguments

| Argument | Meaning |
| --- | --- |
| `--config` | YAML backtest config path |
| `--strategy` | Built-in/registered strategy name, or `module:Class` for a Python strategy |
| `--strategy-params` | Optional JSON file merged into `strategy_params` |
| `--output-json` | Write serialized report JSON |
| `--output-csv` | Write serialized report CSV |
| `--output-equity` | Write equity curve CSV |
| `--output-trades` | Write trades CSV |
| `--print-summary` | Print performance summary JSON to stdout |

## Built-In Strategy Example

```bash
regimeflow-backtest \
  --config examples/backtest_basic/config.yaml \
  --strategy moving_average_cross \
  --output-json report.json \
  --output-equity equity.csv \
  --output-trades trades.csv \
  --print-summary
```

## Python Strategy Example

```bash
regimeflow-backtest \
  --config examples/backtest_basic/config.yaml \
  --strategy my_strategies:MyStrategy
```

The class must:

- be importable from the given module
- subclass `regimeflow.Strategy`

## Strategy Parameters

If you want to inject `strategy_params` from a JSON file:

```bash
regimeflow-backtest \
  --config examples/backtest_basic/config.yaml \
  --strategy my_strategies:MyStrategy \
  --strategy-params params.json
```

The JSON file must contain an object.

## What The CLI Returns

On success:

- exit code `0`
- optional files written to the requested paths
- optional summary printed to stdout

On failure:

- exit code `1`
- a human-readable error message to stderr

## When To Use The CLI Vs Python API

Use the CLI when:

- you want a single shell command for a run
- your configuration is already YAML-driven
- your output is report/export oriented

Use the Python API directly when:

- you are iterating inside notebooks or scripts
- you want multi-step programmatic control
- you need direct access to `results`, dataframes, NumPy arrays, or dashboards

## Related Pages

- `python/overview.md`
- `python/workflow.md`
- `tutorials/python-usage.md`
