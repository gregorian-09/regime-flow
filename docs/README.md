# RegimeFlow Docs

RegimeFlow is a quantitative trading platform that combines regime detection, backtesting, and live execution across brokers. This docs site is structured so you can start with the big picture, then drill down into how data flows, how decisions are made, and how the system executes orders.

## Recommended Reading Path

If you are new, follow this order for a complete mental model:

1. **What the system is**  
   Start with the product overview and architecture.
   - [Overview]([Overview](explanation/overview.md))
   - [Architecture]([Architecture](explanation/architecture.md))

2. **How data flows through the system**  
   Understand ingestion, validation, and event propagation.
   - [Data Flow]([Data Flow](explanation/data-flow.md))
   - [Event Model]([Event Model](explanation/event-model.md))
   - [Execution Flow]([Execution Flow](explanation/execution-flow.md))
   - [Order State Machine]([Order State Machine](explanation/order-state-machine.md))

3. **How regimes influence trading**  
   Learn the regime model, transitions, and feature set.
   - [Regime Detection]([Regime Detection](explanation/regime-detection.md))
   - [Regime Features]([Regime Features](explanation/regime-features.md))
   - [Regime Transitions]([Regime Transitions](explanation/regime-transitions.md))
   - [HMM Math]([HMM Math](explanation/hmm-math.md))

4. **How strategies are selected and executed**  
   Understand execution models, costs, and strategy selection.
   - [Strategy Selection]([Strategy Selection](explanation/strategy-selection.md))
   - [Execution Models]([Execution Models](explanation/execution-models.md))
   - [Execution Costs]([Execution Costs](explanation/execution-costs.md))
   - [Slippage Math]([Slippage Math](explanation/slippage-math.md))

5. **How risk and portfolio accounting work**  
   Review risk policies and portfolio accounting.
   - [Risk Controls]([Risk Controls](explanation/risk-controls.md))
   - [Risk Limits]([Risk Limits](explanation/risk-limits-deep.md))
   - [Portfolio Model]([Portfolio Model](explanation/portfolio-model.md))
   - [Performance Metrics]([Performance Metrics](explanation/performance-metrics.md))

6. **How to run backtests and live trading**  
   Hands-on usage and operational behavior.
   - [Examples]([Examples](tutorials/examples.md))
   - [Data Validation]([Data Validation](how-to/data-validation.md))
   - [Live Trading]([Live Trading](how-to/live-trading.md))
   - [Dashboard Flow]([Dashboard Flow](how-to/dashboard-flow.md))
   - [Live Resiliency]([Live Resiliency](explanation/live-resiliency.md))
   - [Live Order Reconciliation]([Live Order Reconciliation](explanation/live-order-reconciliation.md))

7. **Advanced systems and integrations**  
   Data storage, messaging, and plugins.
   - [Database]([Database](how-to/database.md))
   - [Mmap Storage]([Mmap Storage](how-to/mmap-storage.md))
   - [Symbol Metadata]([Symbol Metadata](how-to/symbol-metadata.md))
   - [Message Bus]([Message Bus](explanation/message-bus.md))
   - [Plugin API]([Plugin API](reference/plugin-api.md))
   - [Plugins]([Plugins](reference/plugins.md))

## Quick Entry Points

- **API Reference (JavaDoc-style):** [API Index]([API Index](api/index.md))  
- **Configuration keys:** [Config Reference]([Config Reference](reference/config-reference.md))  
- **Python API:** [Python Interfaces]([Python Interfaces](reference/python-interfaces.md)) and [Python Usage]([Python Usage](tutorials/python-usage.md))  
- **Broker streaming behavior:** [Broker Streaming]([Broker Streaming](explanation/broker-streaming.md))  
- **Audit logging:** [Audit Logging]([Audit Logging](how-to/audit-logging.md))  

## Diagrams

The diagrams live in `diagrams/` and are referenced throughout the docs. If you prefer visuals first:

- [Component Diagram](diagrams/component-diagram.md)
- [Structural Flow](diagrams/structural-flow.md)
- [UML Class Diagram](diagrams/uml-class-diagram.md)
- [Backtest Sequence](diagrams/sequence-backtest.md)
- [Live Sequence](diagrams/sequence-live.md)

## Docs Map (By Type)

### Tutorials
- [Examples]([Examples](tutorials/examples.md))
- [Python Usage]([Python Usage](tutorials/python-usage.md))

### How-to Guides
- [Live Trading]([Live Trading](how-to/live-trading.md))
- [Data Validation]([Data Validation](how-to/data-validation.md))
- [Database]([Database](how-to/database.md))
- [Mmap Storage]([Mmap Storage](how-to/mmap-storage.md))
- [Symbol Metadata]([Symbol Metadata](how-to/symbol-metadata.md))
- [Audit Logging]([Audit Logging](how-to/audit-logging.md))
- [Dashboard Flow]([Dashboard Flow](how-to/dashboard-flow.md))

### Reference
- [Config Reference]([Config Reference](reference/config-reference.md))
- [Broker Message Types]([Broker Message Types](reference/broker-message-types.md))
- [Python Interfaces]([Python Interfaces](reference/python-interfaces.md))
- [Plugin API]([Plugin API](reference/plugin-api.md))
- [Plugins]([Plugins](reference/plugins.md))
- [API Index]([API Index](api/index.md))

### Explanation
- [Overview]([Overview](explanation/overview.md))
- [Architecture]([Architecture](explanation/architecture.md))
- [Data Flow]([Data Flow](explanation/data-flow.md))
- [Execution Flow]([Execution Flow](explanation/execution-flow.md))
- [Event Model]([Event Model](explanation/event-model.md))
- [Order State Machine]([Order State Machine](explanation/order-state-machine.md))
- [Live Resiliency]([Live Resiliency](explanation/live-resiliency.md))
- [Live Order Reconciliation]([Live Order Reconciliation](explanation/live-order-reconciliation.md))
- [Broker Streaming]([Broker Streaming](explanation/broker-streaming.md))
- [Regime Detection]([Regime Detection](explanation/regime-detection.md))
- [Regime Features]([Regime Features](explanation/regime-features.md))
- [Regime Transitions]([Regime Transitions](explanation/regime-transitions.md))
- [HMM Math]([HMM Math](explanation/hmm-math.md))
- [Strategy Selection]([Strategy Selection](explanation/strategy-selection.md))
- [Risk Controls]([Risk Controls](explanation/risk-controls.md))
- [Risk Limits]([Risk Limits](explanation/risk-limits-deep.md))
- [Execution Models]([Execution Models](explanation/execution-models.md))
- [Execution Costs]([Execution Costs](explanation/execution-costs.md))
- [Slippage Math]([Slippage Math](explanation/slippage-math.md))
- [Portfolio Model]([Portfolio Model](explanation/portfolio-model.md))
- [Performance Metrics]([Performance Metrics](explanation/performance-metrics.md))
- [Backtest Methodology]([Backtest Methodology](explanation/backtest-methodology.md))
- [Walk-Forward]([Walk-Forward](explanation/walk-forward.md))
- [Message Bus]([Message Bus](explanation/message-bus.md))

### Diagrams
- [Component Diagram](diagrams/component-diagram.md)
- [Structural Flow](diagrams/structural-flow.md)
- [UML Class Diagram](diagrams/uml-class-diagram.md)
- [Backtest Sequence](diagrams/sequence-backtest.md)
- [Live Sequence](diagrams/sequence-live.md)

## Role-Based Reading Paths

### Quant Developer
- [Overview]([Overview](explanation/overview.md))
- [Regime Detection]([Regime Detection](explanation/regime-detection.md))
- [Regime Features]([Regime Features](explanation/regime-features.md))
- [Regime Transitions]([Regime Transitions](explanation/regime-transitions.md))
- [HMM Math]([HMM Math](explanation/hmm-math.md))
- [Strategy Selection]([Strategy Selection](explanation/strategy-selection.md))
- [Execution Models]([Execution Models](explanation/execution-models.md))
- [Execution Costs]([Execution Costs](explanation/execution-costs.md))
- [Slippage Math]([Slippage Math](explanation/slippage-math.md))
- [Performance Metrics]([Performance Metrics](explanation/performance-metrics.md))
- [Backtest Methodology]([Backtest Methodology](explanation/backtest-methodology.md))
- [Walk-Forward]([Walk-Forward](explanation/walk-forward.md))
- [Examples]([Examples](tutorials/examples.md))
- [Data Validation]([Data Validation](how-to/data-validation.md))

### Software Engineer / Platform
- [Architecture]([Architecture](explanation/architecture.md))
- [Data Flow]([Data Flow](explanation/data-flow.md))
- [Event Model]([Event Model](explanation/event-model.md))
- [Execution Flow]([Execution Flow](explanation/execution-flow.md))
- [Order State Machine]([Order State Machine](explanation/order-state-machine.md))
- [Live Resiliency]([Live Resiliency](explanation/live-resiliency.md))
- [Live Order Reconciliation]([Live Order Reconciliation](explanation/live-order-reconciliation.md))
- [Message Bus]([Message Bus](explanation/message-bus.md))
- [Database]([Database](how-to/database.md))
- [Mmap Storage]([Mmap Storage](how-to/mmap-storage.md))
- [Symbol Metadata]([Symbol Metadata](how-to/symbol-metadata.md))
- [Config Reference]([Config Reference](reference/config-reference.md))
- [API Index]([API Index](api/index.md))

### Trading Operations / Risk
- [Risk Controls]([Risk Controls](explanation/risk-controls.md))
- [Risk Limits]([Risk Limits](explanation/risk-limits-deep.md))
- [Portfolio Model]([Portfolio Model](explanation/portfolio-model.md))
- [Performance Metrics]([Performance Metrics](explanation/performance-metrics.md))
- [Execution Models]([Execution Models](explanation/execution-models.md))
- [Execution Costs]([Execution Costs](explanation/execution-costs.md))
- [Live Trading]([Live Trading](how-to/live-trading.md))
- [Audit Logging]([Audit Logging](how-to/audit-logging.md))
- [Dashboard Flow]([Dashboard Flow](how-to/dashboard-flow.md))
- [Broker Message Types]([Broker Message Types](reference/broker-message-types.md))

### Python / Research
- [Python Usage]([Python Usage](tutorials/python-usage.md))
- [Python Interfaces]([Python Interfaces](reference/python-interfaces.md))
- [Python API]([Python API](api/python.md))
- [Examples]([Examples](tutorials/examples.md))
- [Performance Metrics]([Performance Metrics](explanation/performance-metrics.md))
- [Regime Detection]([Regime Detection](explanation/regime-detection.md))
- [Strategy Selection]([Strategy Selection](explanation/strategy-selection.md))
