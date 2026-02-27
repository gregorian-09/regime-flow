# Deployment

This page covers practical deployment paths for RegimeFlow services and batch backtests.

## Local Binary Deployment

1. Build the core binaries:

```bash
cmake -S . -B build
cmake --build build --target all
```

2. Run the live CLI:

```bash
./build/bin/regimeflow_live --config examples/live_paper_alpaca/config.yaml
```

## Container Deployment

A `Dockerfile` is present at the repo root. A standard flow is:

```bash
docker build -t regimeflow:latest .
```

Run with mounted configs and data:

```bash
docker run --rm -it \
  -v "$PWD/examples:/opt/regimeflow/examples" \
  -v "$PWD/data:/opt/regimeflow/data" \
  regimeflow:latest
```

## Live Trading Runtime Checklist

- Confirm broker credentials are injected via env or `live.broker_config`.
- Enable heartbeat and reconnect for production sessions.
- Set rate limits (`max_orders_per_minute`, `max_orders_per_second`).
- Persist logs (`log_dir`) and audit logs if enabled.

## Service Management

For long-running live sessions, prefer a process supervisor (systemd, supervisord) and log rotation.

## Next Steps

- `live/overview.md`
- `live/config.md`
