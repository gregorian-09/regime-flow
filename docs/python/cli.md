# Python CLI

The Python CLI is implemented in `python/regimeflow/cli.py` and is designed for backtest runs that export reports.

## Usage

```bash
.venv/bin/python -m regimeflow.cli --config <path> --strategy <strategy>
```

## Arguments

- `--config` path to YAML backtest config.
- `--strategy` either a built-in strategy name or a `module:Class` Python strategy.
- `--strategy-params` optional JSON file to merge into `strategy_params`.
- `--output-json` write report JSON.
- `--output-csv` write report CSV.
- `--output-equity` write equity curve CSV.
- `--output-trades` write trades CSV.
- `--print-summary` prints a performance summary JSON to stdout.

## Examples

Built-in strategy:

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy moving_average_cross \
  --print-summary
```

Python strategy:

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy my_strategies:MyStrategy
```
