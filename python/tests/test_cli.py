import json
import os
import sys
from pathlib import Path

import regimeflow as rf

from regimeflow import cli


def _write_config(path: Path) -> None:
    content = """
    data_source: csv
    data_config:
      data_directory: {data_dir}
      file_pattern: "{{symbol}}.csv"
      has_header: true
    symbols: ["TEST"]
    start_date: "2020-01-01"
    end_date: "2020-01-03"
    bar_type: "1d"
    initial_capital: 100000.0
    currency: "USD"
    regime_detector: "constant"
    regime_params:
      regime: "neutral"
    slippage_model: "zero"
    commission_model: "zero"
    """.format(
        data_dir=os.path.join(os.path.dirname(__file__), "..", "..", "tests", "fixtures")
    )
    path.write_text(content)


def test_cli_builtin_strategy(tmp_path: Path):
    cfg_path = tmp_path / "config.yaml"
    _write_config(cfg_path)

    out_json = tmp_path / "report.json"
    out_equity = tmp_path / "equity.csv"

    rc = cli.main([
        "--config", str(cfg_path),
        "--strategy", "buy_and_hold",
        "--output-json", str(out_json),
        "--output-equity", str(out_equity),
    ])
    assert rc == 0
    assert out_json.exists()
    assert out_equity.exists()

    report = json.loads(out_json.read_text())
    assert "performance_summary" in report


def test_cli_module_strategy(tmp_path: Path, monkeypatch):
    cfg_path = tmp_path / "config.yaml"
    _write_config(cfg_path)

    module_path = tmp_path / "temp_strategy.py"
    module_path.write_text(
        "import regimeflow as rf\n"
        "class NoopStrategy(rf.Strategy):\n"
        "    def initialize(self, ctx):\n"
        "        pass\n"
    )
    monkeypatch.syspath_prepend(str(tmp_path))

    out_json = tmp_path / "report.json"

    rc = cli.main([
        "--config", str(cfg_path),
        "--strategy", "temp_strategy:NoopStrategy",
        "--output-json", str(out_json),
    ])
    assert rc == 0
    assert out_json.exists()
