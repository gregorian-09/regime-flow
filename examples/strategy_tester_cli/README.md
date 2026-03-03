# Strategy Tester CLI

This compiled C++ example renders the shared strategy tester dashboard directly in a terminal.
It uses a split-pane TUI during the run, with non-blocking key controls for tab switching.

Build:

```bash
cmake -S . -B build
cmake --build build --target regimeflow_strategy_tester
```

Run with live terminal refresh:

```bash
./build/bin/regimeflow_strategy_tester
```

Run the live TUI explicitly and switch tabs while the engine is running:

```bash
./build/bin/regimeflow_strategy_tester --tui
```

Render once without refresh:

```bash
./build/bin/regimeflow_strategy_tester --no-live --no-ansi --tab=venues
```

Export the final dashboard snapshot as JSON:

```bash
./build/bin/regimeflow_strategy_tester --json
```

Continuously write the current snapshot to a file while the run is executing:

```bash
./build/bin/regimeflow_strategy_tester --snapshot-file=/tmp/strategy_tester_snapshot.json
```

The demo uses injected quote events and a small built-in strategy so it works without external data files.

Interactive TUI controls:

- `0-8`: jump to a tab (`All`, `Setup`, `Report`, `Graph`, `Trades`, `Orders`, `Venues`, `Optimization`, `Journal`)
- `n`: next tab
- `p`: previous tab
- `q`: quit the live view
