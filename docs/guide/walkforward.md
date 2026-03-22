# Walk-Forward Optimization

Walk-forward optimization evaluates strategy parameters across rolling or anchored windows and measures out-of-sample robustness.

## Configuration

`WalkForwardConfig` fields:

- `window_type`: `Rolling`, `Anchored`, `RegimeAware`.
- `in_sample_period`, `out_of_sample_period`, `step_size`.
- `optimization_method`: `Grid`, `Random`, `Bayesian`.
- `max_trials` for random/bayesian search.
- `fitness_metric`: `sharpe`, `sortino`, `calmar`, `return`, `drawdown`.
- `maximize` boolean.
- `retrain_regime_each_window` boolean.
- `optimize_per_regime` boolean.
- `disable_default_regime_training` boolean.
- `num_parallel_backtests` integer.
- `enable_overfitting_detection` boolean.
- `max_is_oos_ratio` double.
- `initial_capital`.
- `bar_type`.
- `periods_per_year`.

## Parameter Definitions

Each parameter uses a `ParameterDef`:

- `name`.
- `type`: `Int`, `Double`, `Categorical`.
- `min_value`, `max_value`, `step`.
- `categories` for categorical.
- `distribution`: `Uniform`, `LogUniform`, `Normal`.

## Output

`WalkForwardResults` include:

- In-sample and out-of-sample results per window.
- Parameter evolution and stability scores.
- Overfitting diagnostics and efficiency ratios.
- Regime distribution and regime-aware performance.

In Python, pass `WalkForwardResults` directly to the visualization layer:

```python
dashboard = rf.visualization.create_strategy_tester_dashboard(walkforward_results)
app = rf.visualization.create_interactive_dashboard(walkforward_results)
rf.visualization.export_dashboard_html(walkforward_results, "walkforward_report.html")
```

The dashboard will switch to an optimization-aware layout and add an `Optimization` tab.

## Next Steps

- `python/overview.md`
- `guide/backtesting.md`
