# API Design Overview

This section is the canonical, JavaDoc-style reference for the RegimeFlow API surface. It is organized by package (namespace) and mirrors the header layout under `include/regimeflow/**`. Each package page documents its files, primary types, responsibilities, and integration points. Diagrams in `docs/diagrams` are referenced where they provide system-level context.

## How To Read This Reference

- **Package summary**: What the namespace is responsible for.
- **File index**: Every public header in that package and what it provides.
- **Type index**: Key classes/structs/enums and their roles.
- **Lifecycle & data flow**: How types collaborate at runtime.

## Package Index

- [Common](common.md)
- [Data](data.md)
- [Engine](engine.md)
- [Events](events.md)
- [Execution](execution.md)
- [Live](live.md)
- [Metrics](metrics.md)
- [Plugins][Plugins](../reference/plugins.md)
- [Regime](regime.md)
- [Risk](risk.md)
- [Strategy](strategy.md)
- [Walk-Forward](walkforward.md)
- [Python](python.md)
- [Full File Index](file-index.md)

## System Diagrams

- [Component Diagram](../diagrams/component-diagram.md)
- [Structural Flow](../diagrams/structural-flow.md)
- [UML Class Diagram](../diagrams/uml-class-diagram.md)
- [Backtest Sequence](../diagrams/sequence-backtest.md)
- [Live Sequence](../diagrams/sequence-live.md)
