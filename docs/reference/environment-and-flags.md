# Environment And Flags

This page centralizes the user-facing environment variables, secret-loading variables, dashboard launch variables, and build flags used across RegimeFlow docs, examples, and workflows.

## Runtime Environment

### Python From The Repo

- `PYTHONPATH=python:build/lib`
  Used when running Python examples directly from a source checkout.
- `PYTHONPATH=build/lib:build/python:python`
  Used when Python bindings and helper modules are both needed from the local build tree.

### Live Dashboard Launch

- `HOST`
  Bind address for `tools/launch_live_dashboard_prod.sh` and related launch scripts.
- `PORT`
  Bind port for the dashboard server.

### Alpaca

- `ALPACA_API_KEY`
  Alpaca trading API key.
- `ALPACA_API_SECRET`
  Alpaca trading API secret.
- `ALPACA_PAPER_BASE_URL`
  Paper trading REST base URL. Typical value: `https://paper-api.alpaca.markets`
- `ALPACA_STREAM_URL`
  Market-data stream URL. Typical equity paper/demo value: `wss://stream.data.alpaca.markets/v2/iex`

Mounted-secret variants:

- `ALPACA_API_KEY_FILE`
- `ALPACA_API_SECRET_FILE`

### Binance

- `BINANCE_API_KEY`
  Binance API key expected by the current live adapter.
- `BINANCE_SECRET_KEY`
  Binance secret key expected by the current live adapter.
- `BINANCE_BASE_URL`
  Spot REST base URL. Demo Mode example: `https://demo-api.binance.com`
- `BINANCE_STREAM_URL`
  Spot market stream URL. Demo Mode example: `wss://demo-stream.binance.com/ws`
- `BINANCE_RECV_WINDOW_MS`
  Optional recv-window override for signed requests.

Mounted-secret variants:

- `BINANCE_API_KEY_FILE`
- `BINANCE_SECRET_KEY_FILE`

### Interactive Brokers

- `IB_HOST`
  TWS or IB Gateway host.
- `IB_PORT`
  TWS or IB Gateway API port.
- `IB_CLIENT_ID`
  API client id used for the IB session.

## Secret-Manager Overrides

These variables override the command name used when resolving secret-manager references:

- `REGIMEFLOW_VAULT_BIN`
- `REGIMEFLOW_AWS_BIN`
- `REGIMEFLOW_GCLOUD_BIN`
- `REGIMEFLOW_AZ_BIN`

They are only needed when the provider CLI is not available under the default executable name.

## Example Build Variables

These are used by plugin and example builds launched from a source checkout:

- `REGIMEFLOW_ROOT`
  Repository root used by example/plugin `CMakeLists.txt`.
- `REGIMEFLOW_BUILD`
  Build directory used for linking example plugins against locally built RegimeFlow libraries.

## Test And CI Variables

- `REGIMEFLOW_TEST_ROOT`
  Test fixture root used by C++ tests.

## Build Flags

### Core Optional Components

- `-DENABLE_OPENSSL=ON`
  Enable TLS and secure WebSocket support.
- `-DENABLE_CURL=ON`
  Enable HTTP-based data sources and clients.
- `-DENABLE_POSTGRES=ON`
  Enable Postgres-backed data sources and live performance sinks.
- `-DENABLE_ZMQ=ON`
  Enable ZeroMQ message-bus support.
- `-DENABLE_REDIS=ON`
  Enable Redis Streams message-bus support.
- `-DENABLE_KAFKA=ON`
  Enable Kafka message-bus support.
- `-DENABLE_IBAPI=ON`
  Enable Interactive Brokers adapter build support.

### Developer And Quality Flags

- `-DENABLE_CLANG_TIDY=ON`
  Run clang-tidy during the build.
- `-DENABLE_WERROR=ON`
  Treat warnings as errors.
- `-DENABLE_SANITIZERS=ON`
  Enable sanitizer builds where supported.
- `-DBUILD_TESTS=ON`
  Build the test suite.
- `-DBUILD_PYTHON_BINDINGS=ON`
  Build Python bindings.
- `-DREGIMEFLOW_FETCH_DEPS=ON`
  Allow RegimeFlow-managed dependency fetching where the build supports it.

## Workflow Trigger Mode

Repository workflows are currently configured for:

- `workflow_dispatch`

That means CI, docs, wheels, and publish workflows are manual-trigger only from GitHub Actions.
