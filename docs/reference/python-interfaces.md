# Python Interfaces

Python bindings expose the core engine, data sources, metrics, and plotting helpers.

## Package Structure

```mermaid
flowchart LR
  A[regimeflow] --> B[analysis]
  A --> C[data]
  A --> D[strategy]
  A --> E[visualization]
  A --> F[cli]
```

## Plugin Loading (Python)

Python backtests can load native plugins by passing search paths and explicit plugin filenames
through `BacktestConfig` (or YAML).

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("examples/python_engine_regime/config_custom_plugin.yaml")
cfg.plugins_search_paths = ["examples/plugins/custom_regime/build"]
cfg.plugins_load = ["libcustom_regime_detector.so"]
```

YAML equivalent:

```yaml
plugins:
  search_paths:
    - examples/plugins/custom_regime/build
  load:
    - libcustom_regime_detector.so
```

## Typical Workflow

```mermaid
sequenceDiagram
  participant User
  participant PythonAPI
  participant Core

  User->>PythonAPI: load data
  PythonAPI->>Core: DataSource
  User->>PythonAPI: run backtest
  PythonAPI->>Core: BacktestEngine
  User->>PythonAPI: analyze
  PythonAPI->>Core: Metrics/Reports
```



## Interpretation

Interpretation: Python calls map directly into the same engine used in C++.
