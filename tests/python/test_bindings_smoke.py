import os
import sys

TEST_ROOT = os.environ.get("REGIMEFLOW_TEST_ROOT")
if not TEST_ROOT:
    raise RuntimeError("REGIMEFLOW_TEST_ROOT not set")

# Ensure Windows DLL search paths are configured before importing extension.
if sys.platform == "win32" and hasattr(os, "add_dll_directory"):
    candidates = [
        os.path.join(TEST_ROOT, "build", "python"),
        os.path.join(TEST_ROOT, "build", "lib"),
        os.path.join(TEST_ROOT, "build", "bin"),
        os.path.join(TEST_ROOT, "vcpkg_installed", "x64-windows", "bin"),
        os.path.join(TEST_ROOT, "vcpkg_installed", "x64-windows", "debug", "bin"),
    ]
    for entry in candidates:
        if os.path.isdir(entry):
            os.add_dll_directory(entry)

# Ensure package + built module are visible
build_lib = os.path.join(TEST_ROOT, "build", "lib")
build_python = os.path.join(TEST_ROOT, "build", "python")
source_python = os.path.join(TEST_ROOT, "python")
for path in (build_lib, build_python, source_python):
    if path not in sys.path:
        sys.path.insert(0, path)

import regimeflow as rf


def main():
    try:
        import numpy as np
    except Exception as exc:
        raise RuntimeError("numpy import failed") from exc

    arr = np.array([1.0, 2.0, 3.0])
    if int(arr.sum()) != 6:
        raise RuntimeError("numpy basic check failed")

    data_config = {
        "data_directory": os.path.join(TEST_ROOT, "tests", "fixtures"),
        "file_pattern": "{symbol}.csv",
        "has_header": True,
    }

    cfg = rf.engine.BacktestConfig()
    cfg.data_source = "csv"
    cfg.data_config = data_config
    cfg.symbols = ["TEST"]
    cfg.start_date = "2020-01-01"
    cfg.end_date = "2020-01-03"
    cfg.bar_type = "1d"
    cfg.initial_capital = 100000.0
    cfg.currency = "USD"
    cfg.regime_detector = "constant"
    cfg.regime_params = {"regime": "neutral"}
    cfg.slippage_model = "zero"
    cfg.commission_model = "zero"

    class NoopStrategy(rf.strategy.Strategy):
        def initialize(self, ctx):
            pass

    engine = rf.engine.BacktestEngine(cfg)
    results = engine.run(NoopStrategy())

    equity_df = results.equity_curve()
    trades_df = results.trades()

    if equity_df.empty:
        raise RuntimeError("Equity curve DataFrame is empty")
    if "equity" not in equity_df.columns:
        raise RuntimeError("Equity curve missing equity column")

    if trades_df is None:
        raise RuntimeError("Trades DataFrame not returned")

    regime_metrics = results.regime_metrics()
    if not isinstance(regime_metrics, dict):
        raise RuntimeError("regime_metrics did not return dict")


if __name__ == "__main__":
    main()
