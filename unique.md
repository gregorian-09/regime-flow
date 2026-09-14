# RegimeFlow: Research-to-Deployment Feature Direction

The highest-value product direction is a local-first **Experiment Ledger** that turns every
backtest, walk-forward run, and later paper or live run into a portable, verifiable research
artifact.

RegimeFlow should be organized around the complete research-to-deployment lifecycle rather than
around isolated technical components:

```text
Data + Config
     |
Experiment Run
     |
Manifest + Artifacts
     |
Verify + Compare
     |
Promote to Paper Trading
     |
Promote to Live Trading
     |
Monitor + Audit + Replay
```

## 1. Research Workspace

The research workspace should make hypothesis development fast and Python-first.

Features:

- Python research API
- Notebook integration
- Dataset and feature inspection
- Regime visualization
- Strategy templates
- Custom indicators and signals
- Interactive equity, trade, and regime charts
- Fast screening across symbols and parameter combinations
- Easy transition from exploratory code to a formal strategy

Primary users:

- Quant researchers
- Quantitative analysts
- Data scientists
- Students and educators

Example:

```python
session = rf.ResearchSession("configs/momentum.yaml")
results = session.run_backtest("momentum")
rf.analysis.display_report(results)
```

The researcher should not need to understand the C++ build system, broker APIs, or deployment
infrastructure to obtain a credible result.

## 2. Data and Feature Lineage

Every result should explain which exact inputs produced it.

Features:

- Dataset identifiers and SHA-256 fingerprints
- Data schema and timezone metadata
- Missing-bar and duplicate-row detection
- Corporate-action and symbol-change tracking
- Feature-generation metadata
- Training, validation, and test-period declarations
- Detection of look-ahead and future-data contamination
- Optional data catalog integration
- Cached normalized datasets

A result should answer:

> Which exact data, transformations, symbols, dates, and timezone rules produced this number?

## 3. Experiment Execution

The computational layer should combine fast broad screening with realistic event-driven
validation.

Features:

- Event-driven backtesting
- Fast vectorized screening path
- Walk-forward optimization
- Grid, random, and Bayesian parameter search
- Multi-asset and portfolio-level simulations
- Slippage and transaction-cost models
- Market-impact and order-book simulation
- Regime-conditioned strategy behavior
- Deterministic random seeds
- Parallel execution with resource limits

Researchers should be able to screen many candidates quickly, then validate only the strongest
candidates with realistic execution assumptions.

## 4. Reproducible Experiment Ledger

This should be RegimeFlow's signature feature. Every run produces a portable artifact bundle:

```text
runs/
  2026-09-14T120000Z-a81f9d/
    manifest.json
    resolved-config.yaml
    metrics.json
    equity.csv
    fills.csv
    orders.csv
    events.ndjson
    report.html
    dashboard.html
    environment.json
    integrity.sha256
```

The manifest should capture:

- RegimeFlow version
- Git commit and dirty-worktree state
- Platform, compiler, Python version, and ABI
- Fully resolved configuration
- Data fingerprints
- Strategy and plugin versions
- Random seeds
- Regime-model settings
- Execution and cost assumptions
- Metrics and artifact checksums
- Validation results

Proposed commands:

```bash
regimeflow experiment run --config strategies/momentum.yaml --strategy momentum --output runs/
regimeflow experiment verify runs/<run-id>
regimeflow experiment compare runs/<run-a> runs/<run-b>
regimeflow experiment replay runs/<run-id>
```

The default should be local filesystem storage. DVC and MLflow exporters can be optional
integrations rather than core dependencies.

## 5. Analysis and Model Diagnostics

Performance numbers alone are not enough. Reports should explain how a result was produced.

Features:

- Return and risk metrics
- Drawdown analysis
- Trade-distribution analysis
- Regime-conditioned performance
- Exposure and concentration analysis
- Turnover and transaction-cost attribution
- Benchmark comparison
- Parameter-sensitivity heatmaps
- In-sample versus out-of-sample comparison
- Stability and degradation analysis
- Stress tests for spread, slippage, latency, and missing data

An automatic diagnostic report should answer:

- Which regimes contributed most of the return?
- Which symbols contributed most of the risk?
- How much performance disappeared after costs?
- How sensitive is the result to the selected parameters?
- Did performance degrade out of sample?

## 6. Collaboration and Review

The output of a researcher should be easy for another person to inspect without manually
recreating the environment.

Features:

- Static HTML reports
- Run comparison pages
- Shareable run directories
- Notes and annotations
- Run tags such as `candidate`, `rejected`, `paper`, or `production`
- Export to Markdown, JSON, CSV, and HTML
- Reproducibility status on every report
- Links between code, data, configuration, and result artifacts

An analyst should be able to compare runs such as:

```text
Run A: Momentum, 2020-2023, 12 bps cost
Run B: Momentum, 2020-2023, 20 bps cost
Run C: Momentum, 2018-2024, walk-forward
```

without opening the strategy source code.

## 7. Promotion and Validation Gates

RegimeFlow should provide a responsible bridge from research to trading.

Before paper or live promotion, configurable gates should verify:

- Manifest integrity
- Dataset fingerprints
- Data-quality status
- Presence of an out-of-sample period
- Maximum drawdown limits
- Explicit cost assumptions
- Backtest/live parity
- Known strategy and plugin checksums
- Risk checks
- Completion of a paper-trading observation period

The strategy lifecycle should be explicit:

```text
research -> candidate -> paper -> approved -> live -> retired
```

The system should expose evidence and unresolved risks rather than automatically declaring a
strategy safe or profitable.

## 8. Paper and Live Trading

For traders and portfolio managers, the same strategy contract should work in research, paper,
and live environments while recording environment-specific execution evidence.

Features:

- Paper trading with the same strategy and configuration as backtesting
- Broker adapter abstraction
- Order and position reconciliation
- Risk limits and kill switches
- Live regime-state display
- Expected-versus-actual execution analysis
- Connection and heartbeat monitoring
- Safe restart and recovery
- Audit journal
- Replay of live events

## 9. Developer and Plugin Platform

Quant developers should be able to extend the system without reverse-engineering internal
implementation details.

Features:

- Stable strategy plugin API
- Regime detector plugin API
- Risk-manager plugin API
- Execution-model plugin API
- Python and C++ extension points
- Plugin metadata and checksums
- Compatibility checks
- Example plugin templates
- Contract tests for plugins
- Benchmark harness
- ABI and version diagnostics

A plugin should be testable against historical bars, synthetic data, malformed data, multiple
regimes, concurrent event delivery, paper execution, and live reconciliation.

## 10. Security and Governance

Because RegimeFlow can connect to brokers and execute financial operations, security must be a
product category rather than an afterthought.

Features:

- Secret redaction in manifests and logs
- No secrets in configuration artifacts
- Plugin allowlists
- Third-party dependency manifests
- Dependency vulnerability scanning
- Signed release artifacts
- Checksums for broker and plugin binaries
- Explicit plaintext-network warnings
- Capability restrictions for plugins
- Audit-retention policies
- Safe defaults for live execution

## Persona Workflows

### Quant Researcher

1. Load or inspect a dataset.
2. Explore features and regime behavior in Python.
3. Run a quick screening experiment.
4. Validate candidates with event-driven execution.
5. Run walk-forward analysis.
6. Generate an artifact bundle.
7. Compare against benchmarks and previous runs.
8. Share the report with an analyst or developer.

### Quantitative Analyst

1. Open a run report.
2. Verify data and code fingerprints.
3. Compare performance under different costs and periods.
4. Inspect drawdowns, exposures, and regime attribution.
5. Review out-of-sample stability.
6. Approve, reject, or request another experiment.

### Quant Developer

1. Create a strategy or plugin from a template.
2. Run contract and unit tests.
3. Benchmark it against the current implementation.
4. Execute it through research and paper-trading paths.
5. Inspect generated journals and manifests.
6. Package it for team or deployment use.

### Trader or Portfolio Manager

1. Review a strategy's research evidence.
2. Confirm risk and execution assumptions.
3. Run it in paper mode.
4. Monitor regime, orders, fills, and exposure.
5. Approve live promotion after configured gates pass.
6. Investigate differences between expected and actual execution.

## Recommended Delivery Order

1. `ExperimentManifest` and portable run artifacts.
2. Python `ExperimentSession` API.
3. `experiment run`, `verify`, `compare`, and `replay` CLI commands.
4. Data fingerprints and data-quality metadata.
5. Static HTML comparison reports.
6. Research-to-paper promotion gates.
7. Live execution evidence and monitoring.
8. Optional MLflow/DVC exporters and remote storage.

## Product Identity

RegimeFlow should let researchers discover ideas quickly, prove them reproducibly, and move them
toward paper or live execution with explicit evidence and safety gates.

The first release should prioritize the `run -> verify -> compare -> promote` workflow over a
generic AI strategy generator, another isolated broker adapter, or a cloud dashboard. The goal is
to make RegimeFlow a regime-aware research-to-execution platform with trustworthy evidence.

## Research References

- [VectorBT](https://vectorbt.dev/) demonstrates a pandas/NumPy-native rapid research workflow
  with notebook-oriented visualization.
- [QuantConnect Research](https://www.quantconnect.com/docs/v1/research/overview) demonstrates
  the value of a dedicated research environment and reuse between research and backtesting.
- [NautilusTrader Getting Started](https://nautilustrader.io/docs/latest/getting_started/)
  demonstrates a higher-level backtest/live path organized around a data catalog.
- [DVC Command Reference](https://dvc.org/doc/command-reference/) documents versioned data,
  pipelines, and experiments for reproducible workflows.
- [MLflow Tracking API](https://www.mlflow.org/docs/latest/api_reference/rest-api.html)
  treats parameters, metrics, tags, and artifacts as first-class run data.
