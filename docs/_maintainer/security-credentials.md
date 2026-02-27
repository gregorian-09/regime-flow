# Security & Credentials

This page summarizes best practices for handling broker credentials and sensitive data.

## Do Not Commit Secrets

Never commit broker keys or account credentials to the repository. Use environment variables or `.env` files that are excluded from version control.

## Environment Variables

The live CLI reads env variables and merges them into `live.broker_config`. For Alpaca:

- `ALPACA_API_KEY`
- `ALPACA_API_SECRET`
- `ALPACA_PAPER_BASE_URL`

## .env Files

Place a `.env` file in the project root and the live CLI will load it automatically.

Example:

```
ALPACA_API_KEY=...
ALPACA_API_SECRET=...
ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
```

## Data Hygiene

- Treat broker logs and fills as sensitive.
- Avoid exporting PII into reports.
- Rotate API keys on a schedule.
