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

