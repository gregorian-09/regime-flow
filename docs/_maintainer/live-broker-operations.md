# Live Broker Operations

This runbook covers the minimum operational workflow for broker secrets, paper validation, and rotation.

## 1. Inject Secrets

Preferred production patterns:

- Secret manager to environment at process start.
- Mounted files with `*_FILE`.
- Local `.env` only for development.

Examples:

```bash
export ALPACA_API_KEY_FILE=/var/run/secrets/alpaca/api_key
export ALPACA_API_SECRET_FILE=/var/run/secrets/alpaca/api_secret
export ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
export ALPACA_STREAM_URL=wss://stream.data.alpaca.markets/v2/iex
```

```bash
export BINANCE_API_KEY_FILE=/var/run/secrets/binance/api_key
export BINANCE_SECRET_KEY_FILE=/var/run/secrets/binance/secret_key
export BINANCE_BASE_URL=https://demo-api.binance.com/api
export BINANCE_STREAM_URL=wss://demo-stream.binance.com/ws
```

```bash
export IB_HOST=127.0.0.1
export IB_PORT=7497
export IB_CLIENT_ID=7
```

## 2. Run Paper Smoke Validation

From an existing build directory:

```bash
ctest -R live_paper_ --output-on-failure
```

These tests:

- skip cleanly when required env vars are absent
- confirm the `regimeflow_live` binary is wired for the selected broker
- exercise the env-backed startup path without committing credentials

## 3. Run Manual Paper Session

```bash
./build/bin/regimeflow_live --config examples/live_paper_alpaca/config.yaml
./build/bin/regimeflow_live --config examples/live_paper_binance/config.yaml
./build/bin/regimeflow_live --config examples/live_paper_ib/config.yaml
```

Verify:

- startup reaches `Live engine started (connected)`
- heartbeat transitions remain healthy
- audit log is created under the configured log directory
- no credential material appears in stdout, stderr, or audit logs

## 4. Rotate Keys

1. Create new paper/demo keys in the broker console.
2. Update the secret manager entry or mounted secret file.
3. Restart the staging or paper process.
4. Re-run `ctest -R live_paper_ --output-on-failure`.
5. Re-run one manual paper session.
6. Revoke the old keys after validation.

## 5. Escalate On Exposure

If a key is suspected to be exposed:

1. Revoke it immediately in the broker console.
2. Replace the secret in the secret manager or mounted file.
3. Confirm the old value no longer appears in process environment or mounted path.
4. Re-run smoke validation.
5. Review logs and retained artifacts for possible historical leakage.

## 6. Current Boundaries

- CLI and audit-log redaction protect loaded secret values.
- Broker adapters still rely on the surrounding runtime for broker permissions, network policy, and CLI authentication state.
- Secret-manager references are resolved through the official provider CLIs rather than linked cloud SDKs.
