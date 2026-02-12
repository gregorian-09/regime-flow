"""Metric validation utilities."""

from __future__ import annotations

from typing import Dict, List, Tuple


def _regime_name(value: object) -> str:
    if hasattr(value, "name"):
        return str(value.name)
    return str(value)


def validate_regime_attribution(results, tolerance: float = 1.0e-6) -> Tuple[bool, str]:
    """Cross-check regime attribution against an independent recomputation.

    This computes returns by regime from the equity curve and the engine's regime history,
    then compares the totals and observation counts against the built-in regime attribution.
    """
    if results is None:
        return False, "Missing results"

    try:
        equity = results.equity_curve()
    except Exception as exc:  # pragma: no cover - defensive
        return False, f"Unable to load equity curve: {exc}"

    if equity is None or equity.empty:
        return False, "Empty equity curve"

    try:
        regime_history = results.regime_history()
    except Exception as exc:  # pragma: no cover - defensive
        return False, f"Unable to load regime history: {exc}"

    if not regime_history:
        return False, "Missing regime history"

    try:
        attribution = results.regime_metrics()
    except Exception as exc:  # pragma: no cover - defensive
        return False, f"Unable to load regime attribution: {exc}"

    if not attribution:
        return False, "Missing regime attribution"

    timestamps: List = list(equity.index)
    equities = equity["equity"].to_list()
    if len(timestamps) != len(equities):
        return False, "Equity curve index mismatch"

    states = sorted(regime_history, key=lambda s: s.timestamp.value)
    if not states:
        return False, "Regime history too short"

    recomputed: Dict[str, Dict[str, float]] = {}

    if len(states) == len(equities):
        first_regime = _regime_name(states[0].regime)
        recomputed.setdefault(first_regime, {"return": 0.0, "observations": 0})
        recomputed[first_regime]["observations"] += 1
        for i in range(1, len(equities)):
            prev = equities[i - 1]
            ret = 0.0 if prev == 0 else (equities[i] - prev) / prev
            regime = _regime_name(states[i].regime)
            bucket = recomputed.setdefault(regime, {"return": 0.0, "observations": 0})
            bucket["return"] += ret
            bucket["observations"] += 1
    else:
        idx = 0
        first_ts_value = int(timestamps[0].timestamp() * 1_000_000)
        while idx + 1 < len(states) and states[idx + 1].timestamp.value < first_ts_value:
            idx += 1
        first_regime = _regime_name(states[idx].regime)
        recomputed.setdefault(first_regime, {"return": 0.0, "observations": 0})
        recomputed[first_regime]["observations"] += 1
        for i in range(1, len(equities)):
            prev = equities[i - 1]
            ret = 0.0 if prev == 0 else (equities[i] - prev) / prev
            ts_value = int(timestamps[i].timestamp() * 1_000_000)
            while idx + 1 < len(states) and states[idx + 1].timestamp.value <= ts_value:
                idx += 1
            regime = _regime_name(states[idx].regime)
            bucket = recomputed.setdefault(regime, {"return": 0.0, "observations": 0})
            bucket["return"] += ret
            bucket["observations"] += 1

    if len(states) == 1:
        regime = _regime_name(states[0].regime)
        recomputed.setdefault(regime, {"return": 0.0, "observations": 0})
        recomputed[regime]["observations"] += 1

    for regime, metrics in attribution.items():
        if metrics.get("observations", 0) == 0:
            continue
        if regime not in recomputed:
            return False, f"Regime {regime} missing from recompute"
        rec = recomputed[regime]
        if abs(rec["return"] - metrics.get("return", 0.0)) > tolerance:
            return False, (
                f"Return mismatch for {regime}: "
                f"{rec['return']} vs {metrics.get('return', 0.0)}"
            )
        if int(rec["observations"]) != int(metrics.get("observations", 0)):
            return False, (
                f"Observation mismatch for {regime}: "
                f"{rec['observations']} vs {metrics.get('observations', 0)}"
            )

    return True, "Regime attribution validated"
