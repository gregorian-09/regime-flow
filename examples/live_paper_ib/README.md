# Live Paper Trading: Interactive Brokers

Paper trading example using IB. This is **env-gated** and should never be run in CI.

Important:

- Interactive Brokers access can fail for reasons outside this project, including account policy,
  jurisdiction restrictions, IP-origin restrictions, and other compliance controls enforced by IB.
- If TWS login or paper access is blocked by IB for your region or IP, that is an external broker-side
  limitation rather than a RegimeFlow build problem.
- If you need to adapt the IB integration for a deployment environment we do not currently handle,
  fork the project, make the change, and open a pull request for review.

## Required env vars

- `IB_HOST` (example: 127.0.0.1)
- `IB_PORT` (example: 7497 for paper)
- `IB_CLIENT_ID`

## Contract Coverage

The sample config now shows how to keep one IB paper profile and mix contract types:

- US equities through `SMART`
- FX through `CASH` and `IDEALPRO`
- Futures through `local_symbol`
- Options through expiry, strike, right, and multiplier

If you only trade US equities, keep the `defaults.*` section and remove the `contracts.*` overrides you do not need.

## Run

```bash
export IB_HOST=127.0.0.1
export IB_PORT=7497
export IB_CLIENT_ID=1
./build/bin/regimeflow_live --config examples/live_paper_ib/config.yaml
```
