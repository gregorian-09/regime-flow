# Python Usage

This tutorial is a Python-first walkthrough of the package. It focuses on
what a Python user actually does with `regimeflow` after installation.

## Use Case 1: Run A Backtest And Export Outputs

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")

rf.analysis.write_report_json(results, "report.json")
rf.analysis.write_report_csv(results, "report.csv")
rf.analysis.write_report_html(results, "report.html")
results.equity_curve().to_csv("equity.csv", index=True)
results.trades().to_csv("trades.csv", index=True)
```

Use this pattern when you want a compact Python script that runs a strategy and
hands downstream consumers JSON, CSV, HTML, and dataframe outputs.

## Use Case 2: Develop A Python Strategy

```python
import regimeflow as rf

class ThresholdStrategy(rf.Strategy):
    def initialize(self, ctx):
        self.ctx = ctx

    def on_bar(self, bar):
        if bar.close > bar.open:
            order = rf.Order("AAPL", rf.OrderSide.BUY, rf.OrderType.MARKET, 1.0)
            self.ctx.submit_order(order)

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run(ThresholdStrategy())
```

Use this pattern when the strategy logic itself belongs in Python.

## Use Case 3: Work With Pandas

```python
from regimeflow.data import load_csv_dataframe, normalize_timezone, fill_missing_time_bars

df = load_csv_dataframe("bars.csv")
df = normalize_timezone(df, timestamp_col="timestamp", tz="America/New_York")
df = fill_missing_time_bars(df, freq="1min")
```

Use this pattern when your research loop starts from dataframes and preprocessing.

## Use Case 4: Convert Native Results Into Research Tables

```python
tables = rf.data.results_to_dataframe(results)
summary_df = tables["summary"]
equity_df = tables["equity_curve"]
trades_df = tables["trades"]
regime_df = tables["regime_metrics"]
```

Use this pattern when you want a predictable dataframe bundle for notebooks,
analytics pipelines, or report generation.

## Use Case 5: Inspect Regime Metrics

```python
summary = rf.analysis.performance_summary(results)
stats = rf.analysis.performance_stats(results)
regime = rf.analysis.regime_performance(results)
transitions = rf.analysis.transition_metrics(results)
```

Use this pattern when you care about regime attribution and not only total return.

## Use Case 6: Validate Regime Attribution

```python
ok, message = rf.metrics.validate_regime_attribution(results)
print(ok, message)
```

Use this pattern when you want an independent check that the regime attribution
in the report matches a recomputation from the equity curve and regime history.

## Use Case 7: Notebook Display

```python
rf.analysis.display_report(results)
rf.analysis.display_equity(results)
```

Use this pattern in Jupyter when you want lightweight inline report rendering.

## Use Case 8: Dashboard Export

```python
rf.visualization.export_dashboard_html(results, "strategy_tester_report.html")
```

Use this pattern when you want a shareable HTML artifact from a Python workflow.

## Use Case 9: Research Sessions And Parity

```python
session = rf.research.ResearchSession(config_path="examples/backtest_basic/config.yaml")
results = session.run_backtest("moving_average_cross")
parity = session.parity_check(live_config_path="examples/live_paper_alpaca/config.yaml")
```

Use this pattern when your research tooling needs a higher-level session object.

## Use Case 10: Walk-Forward Work

Use:

- `rf.ParameterDef`
- `rf.WalkForwardConfig`
- `rf.WalkForwardOptimizer`
- `rf.WalkForwardResults`

when you want optimization windows and stitched out-of-sample results from Python.

## Common Python Output Surfaces

From `results`:

- `results.report_json()`
- `results.report_csv()`
- `results.equity_curve()`
- `results.account_curve()`
- `results.trades()`
- `results.account_state()`
- `results.venue_fill_summary()`
- `results.regime_metrics()`
- `results.regime_history()`
- `results.dashboard_snapshot()`
- `results.tester_report()`
- `results.tester_journal()`

## Recommended Reading Order

1. [Python Overview](../python/overview.md)
2. [Python Workflow](../python/workflow.md)
3. [Python CLI](../python/cli.md)
4. [Python API Reference](../api/python.md)
