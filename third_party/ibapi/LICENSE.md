# Interactive Brokers TWS API License Notice

This directory vendors the subset of the Interactive Brokers Trader Workstation
(TWS) API needed by RegimeFlow's optional Interactive Brokers adapter.

- Upstream: https://interactivebrokers.github.io/
- License: TWS API Non-Commercial License
- Vendored scope: C++ client sources and generated C++ protobuf stubs under
  `IBJts/source/cppclient/client`

The IBKR TWS API is not distributed under the RegimeFlow project license. Users
are responsible for reviewing and complying with Interactive Brokers' current
license terms before enabling or distributing builds that include this adapter.

The unused Java client, Python client, sample applications, binary JARs, and
source `.proto` files are intentionally not vendored here. RegimeFlow consumes
only the C++ API surface required to build `REGIMEFLOW_ENABLE_IBAPI`.
