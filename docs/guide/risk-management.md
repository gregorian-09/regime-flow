# Risk Management

Risk in RegimeFlow is enforced by the `RiskManager` and optional stop-loss rules. Risk configuration is applied through `BacktestEngine::configure_risk` and the live engine's `LiveConfig::risk_config`.

## Risk Limits

Risk limits are configured under `limits`.

Supported keys:

- `limits.max_notional` maximum notional per order.
- `limits.max_position` maximum position size in shares/contracts.
- `limits.max_position_pct` maximum position as a fraction of equity.
- `limits.max_drawdown` maximum drawdown fraction.
- `limits.max_gross_exposure` maximum gross exposure as a fraction of equity.
- `limits.max_net_exposure` maximum net exposure as a fraction of equity.
- `limits.max_leverage` maximum leverage.

Sector and industry limits:

- `limits.sector_limits` map of sector to max exposure pct.
- `limits.sector_map` map of symbol to sector.
- `limits.industry_limits` map of industry to max exposure pct.
- `limits.industry_map` map of symbol to industry.

Correlation limits:

- `limits.correlation.window` price history window length.
- `limits.correlation.max_corr` correlation threshold.
- `limits.correlation.max_pair_exposure_pct` max exposure for correlated pairs.

Regime-specific limits:

- `limits_by_regime.<regime>.limits.*` use the same keys as above, scoped to a regime label.

Example:

```yaml
risk:
  limits:
    max_notional: 100000
    max_position_pct: 0.2
    max_drawdown: 0.2
  limits_by_regime:
    crisis:
      limits:
        max_notional: 25000
        max_position_pct: 0.05
```

## Stop-Loss Configuration

Stop-loss rules are configured under `stop_loss`, `trailing_stop`, `atr_stop`, and `time_stop`:

- `stop_loss.enable` and `stop_loss.pct` for fixed stop loss.
- `trailing_stop.enable` and `trailing_stop.pct` for trailing stop loss.
- `atr_stop.enable`, `atr_stop.window`, `atr_stop.multiplier` for ATR-based stops.
- `time_stop.enable`, `time_stop.max_holding_seconds` for time-based exits.

Example:

```yaml
risk:
  limits:
    max_position_pct: 0.2
  stop_loss:
    enable: true
    pct: 0.05
  trailing_stop:
    enable: true
    pct: 0.03
  atr_stop:
    enable: true
    window: 14
    multiplier: 2.0
  time_stop:
    enable: true
    max_holding_seconds: 7200
```

## Next Steps

- `guide/execution-models.md`
- `reference/configuration.md`
