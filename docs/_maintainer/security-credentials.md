# Security & Credentials

This page summarizes best practices for handling broker credentials and sensitive data.

## Do Not Commit Secrets

Never commit broker keys or account credentials to the repository. Use injected environment variables, mounted secret files, or a secret manager.

## Environment Variables

The live CLI reads broker settings from environment variables and merges them into `live.broker_config`.

Supported env-backed broker keys:

- Alpaca:
  - `ALPACA_API_KEY`
  - `ALPACA_API_SECRET`
  - `ALPACA_PAPER_BASE_URL`
  - `ALPACA_STREAM_URL`
- Binance:
  - `BINANCE_API_KEY`
  - `BINANCE_SECRET_KEY`
  - `BINANCE_BASE_URL`
  - `BINANCE_STREAM_URL`
  - `BINANCE_RECV_WINDOW_MS`
- Interactive Brokers:
  - `IB_HOST`
  - `IB_PORT`
  - `IB_CLIENT_ID`

Each variable also supports the conventional `*_FILE` form. Example:

```bash
export ALPACA_API_KEY_FILE=/var/run/secrets/alpaca/api_key
export ALPACA_API_SECRET_FILE=/var/run/secrets/alpaca/api_secret
```

This works well with mounted secrets from Kubernetes, Vault Agent, Docker secrets, or cloud secret-manager sidecars.

## Secret-Manager References

`live.broker_config` values can also be resolved from provider-backed references before the live engine starts:

- `vault://secret/trading#api_key`
- `aws-sm://prod/regimeflow/alpaca#api_key`
- `gcp-sm://quant-prod/regimeflow-binance#secret_key`
- `azure-kv://ops-vault/ib-credentials#client_secret`

These helpers intentionally use the official provider CLIs instead of linking cloud SDKs into the engine:

- `vault kv get`
- `aws secretsmanager get-secret-value`
- `gcloud secrets versions access`
- `az keyvault secret show`

Override the binary path when needed with:

- `REGIMEFLOW_VAULT_BIN`
- `REGIMEFLOW_AWS_BIN`
- `REGIMEFLOW_GCLOUD_BIN`
- `REGIMEFLOW_AZ_BIN`

Operational notes:

- Vault references default to field `value` when no fragment is provided.
- AWS, GCP, and Azure references return the whole secret value when no fragment is provided.
- When the fetched secret payload is a JSON object, the fragment is treated as a dotted JSON field path.

## .env Files

Place a `.env` file in the project root and the live CLI will load it automatically.
This is a local development convenience, not the production mechanism.

Example:

```
ALPACA_API_KEY=...
ALPACA_API_SECRET=...
ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
```

## Redaction

- The live CLI redacts registered secret values before writing startup errors or runtime error messages to stdout/stderr.
- The live audit log also redacts registered secret values before writing error details to disk.
- Redaction is value-based, so once a secret is loaded into broker config it is masked consistently across those sinks.

## Secret Manager Guidance

Preferred order:

1. Short-lived credentials from a secret manager.
2. Mounted secret files consumed through `*_FILE`.
3. Direct secret-manager references in config resolved through provider CLIs.
4. Direct environment variables.
5. Local `.env` for development only.

## Data Hygiene

- Treat broker logs and fills as sensitive.
- Avoid exporting PII into reports.
- Rotate API keys on a schedule.
