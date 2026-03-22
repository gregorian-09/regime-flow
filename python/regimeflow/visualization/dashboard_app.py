from __future__ import annotations

from collections import deque
from datetime import date, datetime
from enum import Enum
import json
import os
import threading
import time
from pathlib import Path
from typing import Any, Callable, Mapping, Optional

import pandas as pd

from .dashboard import (
    BLOOMBERG_THEME,
    FONT_STACK,
    _build_execution_replay_figure,
    _build_live_equity_figure,
    create_strategy_tester_dashboard,
    create_live_runtime_payload,
    create_interactive_dashboard,
    _normalize_equity_curve,
    _normalize_table,
    _normalize_regime_state,
)


def create_dash_app(results: Any):
    return create_interactive_dashboard(results)


def create_live_dash_app(
    snapshot_provider: Callable[[], Mapping[str, Any]],
    title: str = "RegimeFlow Strategy Tester",
    interval_ms: int = 1000,
    advance_step: Optional[Callable[[], bool]] = None,
):
    try:
        import dash
        from dash import Patch, dcc, html
        from dash.dash_table import DataTable
        from flask_sock import Sock
    except Exception as exc:
        raise ImportError("Dash is required for live dashboards. Install with `regimeflow[viz]`.") from exc

    theme = BLOOMBERG_THEME
    profile_presets: dict[str, dict[str, Any]] = {
        "balanced": {
            "speed": 4,
            "zoom_bars": 180,
            "delta_account_curve": 32,
            "delta_trades": 24,
            "delta_journal": 32,
            "table_rows": 160,
            "max_append_bars": 256,
            "max_points_window_cap": 2800,
            "max_points_all": 2400,
            "max_overlay_annotations": 6,
        },
        "low_latency": {
            "speed": 8,
            "zoom_bars": 120,
            "delta_account_curve": 16,
            "delta_trades": 12,
            "delta_journal": 16,
            "table_rows": 96,
            "max_append_bars": 128,
            "max_points_window_cap": 1400,
            "max_points_all": 1200,
            "max_overlay_annotations": 4,
        },
        "high_history": {
            "speed": 2,
            "zoom_bars": 480,
            "delta_account_curve": 64,
            "delta_trades": 48,
            "delta_journal": 64,
            "table_rows": 360,
            "max_append_bars": 768,
            "max_points_window_cap": 6000,
            "max_points_all": 10000,
            "max_overlay_annotations": 10,
        },
    }

    def _normalize_profile_name(value: Any) -> str:
        raw = str(value or "balanced").strip().lower().replace("-", "_")
        return raw if raw in profile_presets else "balanced"

    default_profile = _normalize_profile_name(os.environ.get("DASHBOARD_PROFILE", "balanced"))

    def _profile_settings(profile_name: str) -> dict[str, Any]:
        return profile_presets[_normalize_profile_name(profile_name)]

    app = dash.Dash(__name__, assets_folder=str(Path(__file__).with_name("assets")))
    sock = Sock(app.server)
    engine_lock = threading.Lock()
    playback_lock = threading.Lock()
    websocket_lock = threading.Lock()
    payload_cache_lock = threading.Lock()
    websocket_clients: dict[Any, dict[str, Any]] = {}
    playback_state = {"playing": False}
    payload_cache: dict[str, Any] = {"key": None, "payload": None}
    telemetry_lock = threading.Lock()
    telemetry: dict[str, Any] = {
        "replay_patch_ms": deque(maxlen=256),
        "replay_extend_ms": deque(maxlen=256),
        "overlay_cache_hit": 0,
        "overlay_cache_miss": 0,
    }
    telemetry_output_raw = str(os.environ.get("DASHBOARD_TELEMETRY_FILE", "logs/live_dashboard_telemetry.jsonl")).strip()
    telemetry_output_path: Optional[Path]
    if telemetry_output_raw.lower() in {"", "off", "none", "0"}:
        telemetry_output_path = None
    else:
        telemetry_output_path = Path(telemetry_output_raw)
    try:
        telemetry_emit_interval_sec = max(0.5, float(os.environ.get("DASHBOARD_TELEMETRY_INTERVAL_SEC", "2.0")))
    except Exception:
        telemetry_emit_interval_sec = 2.0
    telemetry_emit_state = {"last_emit_monotonic": 0.0, "sequence": 0}

    def _safe_snapshot_provider() -> Mapping[str, Any]:
        with engine_lock:
            return snapshot_provider()

    def _safe_advance_step() -> bool:
        if advance_step is None:
            return False
        with engine_lock:
            return advance_step()

    def _record_latency(metric: str, elapsed_ms: float) -> None:
        with telemetry_lock:
            values = telemetry.get(metric)
            if isinstance(values, deque):
                values.append(float(elapsed_ms))

    def _compute_percentile(sorted_values: list[float], percentile: float) -> float:
        if not sorted_values:
            return 0.0
        if len(sorted_values) == 1:
            return sorted_values[0]
        rank = max(0.0, min(100.0, percentile)) / 100.0 * (len(sorted_values) - 1)
        lower = int(rank)
        upper = min(len(sorted_values) - 1, lower + 1)
        fraction = rank - lower
        return sorted_values[lower] * (1.0 - fraction) + sorted_values[upper] * fraction

    def _latency_snapshot(metric: str) -> tuple[float, float]:
        with telemetry_lock:
            values = telemetry.get(metric)
            samples = list(values) if isinstance(values, deque) else []
        if not samples:
            return (0.0, 0.0)
        sorted_values = sorted(float(value) for value in samples)
        return (
            _compute_percentile(sorted_values, 50.0),
            _compute_percentile(sorted_values, 95.0),
        )

    def _telemetry_badge() -> str:
        replay_p50, replay_p95 = _latency_snapshot("replay_patch_ms")
        extend_p50, extend_p95 = _latency_snapshot("replay_extend_ms")
        with telemetry_lock:
            cache_hit = int(telemetry.get("overlay_cache_hit", 0))
            cache_miss = int(telemetry.get("overlay_cache_miss", 0))
        total_cache = cache_hit + cache_miss
        hit_rate = (100.0 * cache_hit / total_cache) if total_cache > 0 else 0.0
        return (
            f"cb patch p50/p95={replay_p50:.1f}/{replay_p95:.1f}ms | "
            f"extend p50/p95={extend_p50:.1f}/{extend_p95:.1f}ms | "
            f"overlay hit={hit_rate:.0f}%"
        )

    def _maybe_emit_telemetry_log(profile_name: str, speed: int, zoom_bars: int) -> None:
        if telemetry_output_path is None:
            return
        now = time.monotonic()
        with telemetry_lock:
            last_emit = float(telemetry_emit_state.get("last_emit_monotonic", 0.0))
            if now - last_emit < telemetry_emit_interval_sec:
                return
            telemetry_emit_state["last_emit_monotonic"] = now
            sequence = int(telemetry_emit_state.get("sequence", 0)) + 1
            telemetry_emit_state["sequence"] = sequence

            replay_patch = sorted(float(v) for v in telemetry.get("replay_patch_ms", []))
            replay_extend = sorted(float(v) for v in telemetry.get("replay_extend_ms", []))
            cache_hit = int(telemetry.get("overlay_cache_hit", 0))
            cache_miss = int(telemetry.get("overlay_cache_miss", 0))

        patch_p50 = _compute_percentile(replay_patch, 50.0) if replay_patch else 0.0
        patch_p95 = _compute_percentile(replay_patch, 95.0) if replay_patch else 0.0
        extend_p50 = _compute_percentile(replay_extend, 50.0) if replay_extend else 0.0
        extend_p95 = _compute_percentile(replay_extend, 95.0) if replay_extend else 0.0
        cache_total = cache_hit + cache_miss
        cache_hit_rate = (100.0 * cache_hit / cache_total) if cache_total > 0 else 0.0

        record = {
            "ts": datetime.utcnow().replace(microsecond=0).isoformat() + "Z",
            "event": "live_dashboard_telemetry",
            "sequence": sequence,
            "profile": profile_name,
            "speed": int(speed),
            "zoom_bars": int(zoom_bars),
            "replay_patch_p50_ms": round(patch_p50, 3),
            "replay_patch_p95_ms": round(patch_p95, 3),
            "replay_extend_p50_ms": round(extend_p50, 3),
            "replay_extend_p95_ms": round(extend_p95, 3),
            "overlay_cache_hit": cache_hit,
            "overlay_cache_miss": cache_miss,
            "overlay_cache_hit_rate_pct": round(cache_hit_rate, 3),
            "samples_patch": len(replay_patch),
            "samples_extend": len(replay_extend),
            "pid": os.getpid(),
        }
        try:
            telemetry_output_path.parent.mkdir(parents=True, exist_ok=True)
            with telemetry_output_path.open("a", encoding="utf-8") as handle:
                handle.write(json.dumps(record, separators=(",", ":")) + "\n")
        except Exception:
            return

    def _json_safe(value: Any) -> Any:
        if isinstance(value, pd.DataFrame):
            return [_json_safe(record) for record in value.to_dict("records")]
        if isinstance(value, pd.Series):
            return [_json_safe(item) for item in value.to_list()]
        if isinstance(value, Mapping):
            return {str(key): _json_safe(item) for key, item in value.items()}
        if isinstance(value, (list, tuple)):
            return [_json_safe(item) for item in value]
        if isinstance(value, Enum):
            return str(value.name)
        if hasattr(value, "name") and hasattr(value, "value"):
            try:
                return str(value.name)
            except Exception:
                pass
        if isinstance(value, pd.Timestamp):
            return value.isoformat()
        if isinstance(value, (datetime, date)):
            return value.isoformat()
        if hasattr(value, "isoformat"):
            try:
                return value.isoformat()
            except Exception:
                pass
        if hasattr(value, "item"):
            try:
                return value.item()
            except Exception:
                pass
        return value

    def _snapshot_json_payload() -> dict[str, Any]:
        snapshot = _safe_snapshot_provider()
        return _json_safe(snapshot)

    def _delta_rows(current: Any, previous: Any, limit: int) -> list[Any]:
        current_rows = current if isinstance(current, list) else []
        previous_rows = previous if isinstance(previous, list) else []
        start = min(len(previous_rows), len(current_rows))
        delta = current_rows[start:]
        return delta[-limit:]

    def _trim_snapshot_for_profile(snapshot_json: Mapping[str, Any], profile_name: str) -> dict[str, Any]:
        cfg = _profile_settings(profile_name)
        trimmed = dict(snapshot_json)
        for key, limit_key in (
            ("account_curve", "delta_account_curve"),
            ("trades", "delta_trades"),
            ("tester_journal", "delta_journal"),
        ):
            values = trimmed.get(key)
            if isinstance(values, list):
                limit = int(cfg[limit_key])
                trimmed[key] = values[-limit:]
        return trimmed

    def _build_client_delta_message(
        current_json: Mapping[str, Any],
        previous_json: Mapping[str, Any] | None,
        profile_name: str,
    ) -> str:
        if previous_json is None:
            return json.dumps({"type": "snapshot", "payload": _trim_snapshot_for_profile(current_json, profile_name)})

        cfg = _profile_settings(profile_name)
        delta_payload = {
            "timestamp": current_json.get("timestamp"),
            "headline": current_json.get("headline", {}),
            "account": current_json.get("account", {}),
            "execution": current_json.get("execution", {}),
            "regime": current_json.get("regime", {}),
            "append": {
                "account_curve": _delta_rows(
                    current_json.get("account_curve", []),
                    previous_json.get("account_curve", []),
                    int(cfg["delta_account_curve"]),
                ),
                "trades": _delta_rows(
                    current_json.get("trades", []),
                    previous_json.get("trades", []),
                    int(cfg["delta_trades"]),
                ),
                "tester_journal": _delta_rows(
                    current_json.get("tester_journal", []),
                    previous_json.get("tester_journal", []),
                    int(cfg["delta_journal"]),
                ),
            },
        }
        for key in ("positions", "open_orders", "quote_summary", "alerts"):
            if current_json.get(key) != previous_json.get(key):
                delta_payload[key] = current_json.get(key, [])
        return json.dumps({"type": "delta", "payload": delta_payload})

    def _payload_cache_key(raw_snapshot: Mapping[str, Any]) -> tuple[Any, ...]:
        execution = raw_snapshot.get("execution", {})
        trades = raw_snapshot.get("trades")
        journal = raw_snapshot.get("tester_journal")
        account_curve = raw_snapshot.get("account_curve")
        trades_len = len(trades.index) if isinstance(trades, pd.DataFrame) else len(trades or [])
        journal_len = len(journal.index) if isinstance(journal, pd.DataFrame) else len(journal or [])
        curve_len = len(account_curve.index) if isinstance(account_curve, pd.DataFrame) else len(account_curve or [])
        return (
            str(raw_snapshot.get("timestamp")),
            execution.get("fill_count"),
            execution.get("open_order_count"),
            trades_len,
            journal_len,
            curve_len,
        )

    def _build_live_payload(raw_snapshot: Mapping[str, Any]) -> Mapping[str, Any]:
        key = _payload_cache_key(raw_snapshot)
        with payload_cache_lock:
            if payload_cache["key"] == key and payload_cache["payload"] is not None:
                return payload_cache["payload"]
        payload = create_live_runtime_payload(raw_snapshot)
        with payload_cache_lock:
            payload_cache["key"] = key
            payload_cache["payload"] = payload
        return payload

    def _broadcast_snapshot() -> None:
        try:
            current_json = _snapshot_json_payload()
        except Exception:
            return
        stale_clients: list[Any] = []
        with websocket_lock:
            for client, session in list(websocket_clients.items()):
                try:
                    profile_name = _normalize_profile_name(session.get("profile", default_profile))
                    previous_json = session.get("last_json")
                    message = _build_client_delta_message(current_json, previous_json, profile_name)
                    client.send(message)
                    session["last_json"] = current_json
                except Exception:
                    stale_clients.append(client)
            for client in stale_clients:
                websocket_clients.pop(client, None)

    @sock.route("/ws/live-dashboard")
    def live_dashboard_socket(ws):
        with websocket_lock:
            websocket_clients[ws] = {"profile": default_profile, "last_json": None}
        with websocket_lock:
            session = websocket_clients.get(ws, {})
        try:
            try:
                full_snapshot = _snapshot_json_payload()
                profile_name = _normalize_profile_name(session.get("profile", default_profile))
                ws.send(json.dumps({"type": "snapshot", "payload": _trim_snapshot_for_profile(full_snapshot, profile_name)}))
                with websocket_lock:
                    if ws in websocket_clients:
                        websocket_clients[ws]["last_json"] = full_snapshot
            except Exception:
                return
            while True:
                message = ws.receive()
                if message is None:
                    break
                try:
                    payload = json.loads(message) if isinstance(message, str) else {}
                except Exception:
                    continue
                if not isinstance(payload, Mapping):
                    continue
                msg_type = str(payload.get("type", "")).lower()
                if msg_type not in {"hello", "client_profile"}:
                    continue
                profile_name = _normalize_profile_name(payload.get("profile", default_profile))
                with websocket_lock:
                    if ws in websocket_clients:
                        websocket_clients[ws]["profile"] = profile_name
                try:
                    full_snapshot = _snapshot_json_payload()
                    ws.send(json.dumps({"type": "snapshot", "payload": _trim_snapshot_for_profile(full_snapshot, profile_name)}))
                    with websocket_lock:
                        if ws in websocket_clients:
                            websocket_clients[ws]["last_json"] = full_snapshot
                except Exception:
                    break
        finally:
            with websocket_lock:
                websocket_clients.pop(ws, None)

    def _playback_loop() -> None:
        while True:
            should_advance = False
            with playback_lock:
                should_advance = bool(playback_state.get("playing", False))
            if should_advance and advance_step is not None:
                running = _safe_advance_step()
                with playback_lock:
                    playback_state["playing"] = bool(running)
                _broadcast_snapshot()
            time.sleep(max(0.2, interval_ms / 1000.0))

    threading.Thread(target=_playback_loop, daemon=True).start()
    replay_compute_lock = threading.Lock()
    normalized_market_cache: dict[str, Any] = {"key": None, "bars": pd.DataFrame()}
    overlay_cache: dict[tuple[Any, ...], tuple[list[dict[str, Any]], list[dict[str, Any]]]] = {}
    overlay_cache_order: list[tuple[Any, ...]] = []
    overlay_cache_max_entries = 192

    def _get_normalized_market_bars(payload: Mapping[str, Any]) -> pd.DataFrame:
        market_bars = payload.get("market_bars")
        if market_bars is None or not isinstance(market_bars, pd.DataFrame) or market_bars.empty:
            return pd.DataFrame()
        if "timestamp" not in market_bars.columns:
            return pd.DataFrame()
        last_timestamp = market_bars["timestamp"].iloc[-1] if len(market_bars.index) > 0 else None
        cache_key = (id(market_bars), len(market_bars.index), str(last_timestamp))
        with replay_compute_lock:
            if normalized_market_cache["key"] == cache_key:
                cached = normalized_market_cache["bars"]
                if isinstance(cached, pd.DataFrame):
                    return cached

        bars = market_bars.copy()
        if not pd.api.types.is_datetime64_any_dtype(bars["timestamp"]):
            bars["timestamp"] = pd.to_datetime(bars["timestamp"], errors="coerce")
        if bars["timestamp"].isna().any():
            bars = bars.dropna(subset=["timestamp"])
        if bars.empty:
            return pd.DataFrame()
        if not bars["timestamp"].is_monotonic_increasing:
            bars = bars.sort_values("timestamp")
        bars = bars.reset_index(drop=True)
        with replay_compute_lock:
            normalized_market_cache["key"] = cache_key
            normalized_market_cache["bars"] = bars
        return bars

    def _get_open_orders_frame(payload: Mapping[str, Any], cutoff_ts: pd.Timestamp) -> pd.DataFrame:
        orders_df = payload.get("orders")
        if not isinstance(orders_df, pd.DataFrame):
            orders_df = _normalize_table(orders_df)
        if orders_df is not None and not orders_df.empty:
            return orders_df

        journal_df = payload.get("journal")
        if not isinstance(journal_df, pd.DataFrame):
            journal_df = _normalize_table(journal_df)
        if journal_df is None or journal_df.empty or "time" not in journal_df.columns or "event" not in journal_df.columns:
            return pd.DataFrame()

        journal = journal_df.copy()
        journal["time"] = pd.to_datetime(journal["time"], errors="coerce")
        journal = journal.dropna(subset=["time"])
        journal = journal[journal["time"] <= cutoff_ts]
        if journal.empty or "order_id" not in journal.columns:
            return pd.DataFrame()

        active: dict[str, dict[str, Any]] = {}
        for _, row in journal.iterrows():
            event_name = str(row.get("event", "")).lower()
            order_id = str(row.get("order_id", "")).strip()
            if not order_id:
                continue
            if event_name == "order submitted":
                active[order_id] = row.to_dict()
            elif event_name in {"order cancelled", "order rejected", "order filled"}:
                active.pop(order_id, None)
        return pd.DataFrame(active.values()) if active else pd.DataFrame()

    def _build_overlay_state(
        payload: Mapping[str, Any],
        profile_name: str,
        profile_cfg: Mapping[str, Any],
        window_start_ts: pd.Timestamp,
        cutoff_ts: pd.Timestamp,
        current_close: float,
    ) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
        open_orders = _get_open_orders_frame(payload, cutoff_ts)
        order_signature: tuple[Any, ...] = ()
        if not open_orders.empty:
            signature_cols = [col for col in ("order_id", "side", "limit_price", "stop_price") if col in open_orders.columns]
            if signature_cols:
                order_signature = tuple(open_orders[signature_cols].itertuples(index=False, name=None))
        quote_df = payload.get("quote_summary")
        if not isinstance(quote_df, pd.DataFrame):
            quote_df = _normalize_table(quote_df)
        bid_value = 0.0
        ask_value = 0.0
        if quote_df is not None and not quote_df.empty:
            first_quote = quote_df.iloc[0]
            try:
                bid_value = float(first_quote.get("bid", 0.0) or 0.0)
            except Exception:
                bid_value = 0.0
            try:
                ask_value = float(first_quote.get("ask", 0.0) or 0.0)
            except Exception:
                ask_value = 0.0

        cache_key = (
            str(payload.get("snapshot", {}).get("timestamp")),
            profile_name,
            int(profile_cfg["max_overlay_annotations"]),
            str(window_start_ts),
            str(cutoff_ts),
            round(float(current_close), 6),
            round(bid_value, 6),
            round(ask_value, 6),
            order_signature,
        )
        with replay_compute_lock:
            cached_overlay = overlay_cache.get(cache_key)
            if cached_overlay is not None:
                with telemetry_lock:
                    telemetry["overlay_cache_hit"] = int(telemetry.get("overlay_cache_hit", 0)) + 1
                return cached_overlay
        with telemetry_lock:
            telemetry["overlay_cache_miss"] = int(telemetry.get("overlay_cache_miss", 0)) + 1

        shapes: list[dict[str, Any]] = []
        annotation_candidates: list[tuple[float, dict[str, Any]]] = []
        positive = theme.get("positive", "#37C978")
        negative = theme.get("negative", "#FF9F43")
        warning = theme.get("warning", "#FFD166")

        for _, order in open_orders.iterrows():
            limit_price = order.get("limit_price")
            stop_price = order.get("stop_price")
            side = str(order.get("side", "")).lower()
            side_color = positive if "buy" in side else negative
            if pd.notna(limit_price):
                try:
                    limit_value = float(limit_price)
                except Exception:
                    limit_value = 0.0
                if limit_value > 0.0:
                    shapes.append(
                        {
                            "type": "line",
                            "xref": "x",
                            "yref": "y",
                            "x0": window_start_ts,
                            "x1": cutoff_ts,
                            "y0": limit_value,
                            "y1": limit_value,
                            "line": {"color": side_color, "dash": "dash", "width": 1},
                            "opacity": 0.75,
                        }
                    )
                    annotation_candidates.append(
                        (
                            abs(limit_value - current_close),
                            {
                                "x": cutoff_ts,
                                "y": limit_value,
                                "xref": "x",
                                "yref": "y",
                                "text": f"LMT {str(order.get('side', '')).upper()} #{order.get('order_id', '')}",
                                "showarrow": False,
                                "xanchor": "left",
                                "font": {"size": 10, "color": side_color},
                                "bgcolor": "rgba(11,15,20,0.8)",
                            },
                        )
                    )
            if pd.notna(stop_price):
                try:
                    stop_value = float(stop_price)
                except Exception:
                    stop_value = 0.0
                if stop_value > 0.0:
                    shapes.append(
                        {
                            "type": "line",
                            "xref": "x",
                            "yref": "y",
                            "x0": window_start_ts,
                            "x1": cutoff_ts,
                            "y0": stop_value,
                            "y1": stop_value,
                            "line": {"color": warning, "dash": "dot", "width": 1},
                            "opacity": 0.75,
                        }
                    )
                    annotation_candidates.append(
                        (
                            abs(stop_value - current_close),
                            {
                                "x": cutoff_ts,
                                "y": stop_value,
                                "xref": "x",
                                "yref": "y",
                                "text": f"STP #{order.get('order_id', '')}",
                                "showarrow": False,
                                "xanchor": "left",
                                "font": {"size": 10, "color": warning},
                                "bgcolor": "rgba(11,15,20,0.8)",
                            },
                        )
                    )

        if bid_value > 0.0:
            shapes.append(
                {
                    "type": "line",
                    "xref": "x",
                    "yref": "y",
                    "x0": window_start_ts,
                    "x1": cutoff_ts,
                    "y0": bid_value,
                    "y1": bid_value,
                    "line": {"color": "#4CB3FF", "dash": "dot", "width": 1},
                    "opacity": 0.7,
                }
            )
            annotation_candidates.append(
                (
                    abs(bid_value - current_close),
                    {
                        "x": cutoff_ts,
                        "y": bid_value,
                        "xref": "x",
                        "yref": "y",
                        "text": f"Bid {bid_value:.2f}",
                        "showarrow": False,
                        "xanchor": "left",
                        "font": {"size": 10, "color": "#4CB3FF"},
                        "bgcolor": "rgba(11,15,20,0.8)",
                    },
                )
            )
        if ask_value > 0.0:
            shapes.append(
                {
                    "type": "line",
                    "xref": "x",
                    "yref": "y",
                    "x0": window_start_ts,
                    "x1": cutoff_ts,
                    "y0": ask_value,
                    "y1": ask_value,
                    "line": {"color": "#FFD166", "dash": "dot", "width": 1},
                    "opacity": 0.7,
                }
            )
            annotation_candidates.append(
                (
                    abs(ask_value - current_close),
                    {
                        "x": cutoff_ts,
                        "y": ask_value,
                        "xref": "x",
                        "yref": "y",
                        "text": f"Ask {ask_value:.2f}",
                        "showarrow": False,
                        "xanchor": "left",
                        "font": {"size": 10, "color": "#FFD166"},
                        "bgcolor": "rgba(11,15,20,0.8)",
                    },
                )
            )

        annotation_candidates.sort(key=lambda item: item[0])
        annotations = [annotation for _, annotation in annotation_candidates[: int(profile_cfg["max_overlay_annotations"])]]
        result = (shapes, annotations)

        with replay_compute_lock:
            overlay_cache[cache_key] = result
            overlay_cache_order.append(cache_key)
            while len(overlay_cache_order) > overlay_cache_max_entries:
                oldest = overlay_cache_order.pop(0)
                overlay_cache.pop(oldest, None)
        return result

    def _panel(title_text: str, child: Any, min_height: str = "0") -> Any:
        return html.Div(
            [
                html.Div(
                    title_text,
                    style={
                        "fontSize": "11px",
                        "fontWeight": "700",
                        "textTransform": "uppercase",
                        "letterSpacing": "0.1em",
                        "color": theme["muted"],
                        "marginBottom": "10px",
                    },
                ),
                child,
            ],
            style={
                "background": "linear-gradient(180deg, rgba(17,24,33,0.98) 0%, rgba(10,14,19,0.98) 100%)",
                "border": f"1px solid {theme['border']}",
                "borderRadius": "10px",
                "padding": "12px",
                "minHeight": min_height,
                "overflow": "hidden",
            },
        )

    def _table(table_id: str) -> Any:
        return DataTable(
            id=table_id,
            page_action="none",
            virtualization=True,
            style_as_list_view=True,
            style_table={"overflowX": "auto", "overflowY": "auto", "maxHeight": "260px", "minWidth": 0},
            style_header={
                "backgroundColor": "#1A232D",
                "color": theme["text"],
                "fontWeight": "700",
                "fontSize": 10,
                "textTransform": "uppercase",
                "letterSpacing": "0.05em",
                "borderBottom": f"1px solid {theme['border']}",
                "padding": "6px 8px",
            },
            style_cell={
                "backgroundColor": "#10161D",
                "color": theme["text"],
                "fontFamily": FONT_STACK,
                "fontSize": 11,
                "padding": "6px 8px",
                "borderBottom": f"1px solid {theme['border']}",
                "textAlign": "left",
                "minWidth": "72px",
                "maxWidth": "180px",
                "whiteSpace": "nowrap",
                "overflow": "hidden",
                "textOverflow": "ellipsis",
                "fontVariantNumeric": "tabular-nums",
            },
            style_data_conditional=[
                {"if": {"row_index": "odd"}, "backgroundColor": "#0D1319"},
            ],
        )

    def _pill(text: str, pill_id: Optional[str] = None) -> Any:
        kwargs = {"id": pill_id} if pill_id else {}
        return html.Div(
            text,
            **kwargs,
            style={
                "padding": "5px 9px",
                "border": f"1px solid {theme['border']}",
                "background": "#0F151C",
                "fontSize": "11px",
                "borderRadius": "4px",
                "color": theme["text"],
                "whiteSpace": "nowrap",
                "fontVariantNumeric": "tabular-nums",
            },
        )

    def _toolbar_group(label: str, children: list[Any]) -> Any:
        return html.Div(
            [
                html.Div(
                    label,
                    style={
                        "fontSize": "10px",
                        "fontWeight": "700",
                        "textTransform": "uppercase",
                        "letterSpacing": "0.08em",
                        "color": theme["muted"],
                        "marginBottom": "6px",
                    },
                ),
                html.Div(children, style={"display": "flex", "gap": "8px", "flexWrap": "wrap"}),
            ],
            style={
                "padding": "8px 10px",
                "border": f"1px solid {theme['border']}",
                "borderRadius": "8px",
                "background": "rgba(10,14,19,0.7)",
            },
        )

    def _button_style(active: bool = False, accent: bool = False) -> dict[str, Any]:
        border_color = theme["accent"] if (active or accent) else theme["border"]
        background = "#163247" if (active or accent) else "#111821"
        return {
            "padding": "7px 12px",
            "border": f"1px solid {border_color}",
            "background": background,
            "color": theme["text"],
            "borderRadius": "6px",
            "boxShadow": "inset 0 0 0 1px rgba(57,160,237,0.18)" if active else "none",
        }

    def _kv_rows(rows: list[tuple[str, Any]]) -> Any:
        return html.Div(
            [
                html.Div(
                    [
                        html.Span(key, style={"color": theme["muted"]}),
                        html.Span(value, style={"fontVariantNumeric": "tabular-nums", "textAlign": "right"}),
                    ],
                    style={
                        "display": "grid",
                        "gridTemplateColumns": "minmax(0, 1fr) auto",
                        "gap": "10px",
                        "padding": "4px 0",
                        "borderBottom": f"1px solid {theme['border']}",
                    },
                )
                for key, value in rows
            ],
            style={"display": "grid", "gap": "2px"},
        )

    def _format_metric(value: Any) -> str:
        try:
            if value is None:
                return "n/a"
            number = float(value)
            if abs(number) >= 1000:
                return f"{number:,.2f}"
            if abs(number) >= 1:
                return f"{number:.4f}".rstrip("0").rstrip(".")
            return f"{number:.6f}".rstrip("0").rstrip(".")
        except Exception:
            return str(value)

    app.layout = html.Div(
        [
            dcc.Store(
                id="live-replay-state",
                data={
                    "playing": False,
                    "speed": int(_profile_settings(default_profile)["speed"]),
                    "zoom_bars": int(_profile_settings(default_profile)["zoom_bars"]),
                    "show_side": True,
                    "show_bottom": True,
                    "follow_latest": True,
                    "focus_panel": "replay",
                    "profile": default_profile,
                },
            ),
            dcc.Store(id="live-ws-snapshot", data={}),
            dcc.Store(id="live-ws-message", data={}),
            html.Div(
                [
                    html.Div(
                        [
                            html.Div(title, style={"margin": "0", "fontSize": "22px", "fontWeight": "700"}),
                            html.Div("Visual Mode / Execution Replay", style={"fontSize": "11px", "color": theme["muted"], "marginTop": "4px"}),
                        ]
                    ),
                    html.Div(
                        [
                            _toolbar_group(
                                "Replay",
                                [
                                    html.Button("Slow", id="live-slow", n_clicks=0, style={"padding": "7px 12px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Start", id="live-start", n_clicks=0, style={"padding": "7px 12px", "border": f"1px solid {theme['accent']}", "background": "#163247", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Pause", id="live-pause", n_clicks=0, style={"padding": "7px 12px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Step", id="live-step", n_clicks=0, style={"padding": "7px 12px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Fast", id="live-fast", n_clicks=0, style={"padding": "7px 12px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Skip +50", id="live-skip", n_clicks=0, style={"padding": "7px 12px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                ],
                            ),
                            _toolbar_group(
                                "Zoom",
                                [
                                    html.Button("1H", id="live-zoom-1h", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("4H", id="live-zoom-4h", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("1D", id="live-zoom-1d", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("All", id="live-zoom-all", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Latest", id="live-latest", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['accent']}", "background": "#163247", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Auto Follow", id="live-follow", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['accent']}", "background": "#163247", "color": theme["text"], "borderRadius": "6px"}),
                                ],
                            ),
                            _toolbar_group(
                                "Layout",
                                [
                                    html.Button("Candles", id="live-focus-replay", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['accent']}", "background": "#163247", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Equity", id="live-focus-equity", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Drawdown", id="live-focus-drawdown", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Reset View", id="live-focus-reset", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Toggle Rail", id="live-toggle-side", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Toggle Console", id="live-toggle-bottom", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                ],
                            ),
                            _toolbar_group(
                                "Performance",
                                [
                                    html.Button("Balanced", id="live-profile-balanced", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['accent']}", "background": "#163247", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("Low Latency", id="live-profile-low-latency", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                    html.Button("High History", id="live-profile-high-history", n_clicks=0, style={"padding": "7px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px"}),
                                ],
                            ),
                        ],
                        style={"display": "flex", "gap": "8px", "flexWrap": "wrap", "justifyContent": "flex-end"},
                    ),
                ],
                style={"display": "flex", "justifyContent": "space-between", "alignItems": "center", "marginBottom": "12px"},
            ),
            html.Div(
                [
                    _pill("status", "live-status"),
                    _pill("mode=backtest"),
                    _pill("symbol=n/a", "live-symbol"),
                    _pill("fills=0", "live-fills"),
                    _pill("open_orders=0", "live-orders"),
                    _pill(f"profile={default_profile}", "live-profile"),
                    _pill("cb patch p50/p95=n/a", "live-cb-latency"),
                    _pill(f"speed={int(_profile_settings(default_profile)['speed'])}", "live-speed"),
                    _pill(f"window={int(_profile_settings(default_profile)['zoom_bars'])}", "live-window"),
                    _pill("ohlc=n/a", "live-ohlc"),
                ],
                style={"display": "flex", "gap": "8px", "flexWrap": "wrap", "marginBottom": "12px"},
            ),
            html.Div(
                [
                    html.Div(
                        [
                            _panel(
                                "Visual Mode",
                                dcc.Graph(
                                    id="replay-graph",
                                    responsive=True,
                                    config={"displayModeBar": False, "responsive": True, "scrollZoom": True, "doubleClick": "reset"},
                                    style={"height": "640px", "width": "100%"},
                                ),
                            ),
                            html.Div(
                                [
                                    _panel(
                                        "Equity",
                                        dcc.Graph(
                                            id="equity-graph",
                                            responsive=True,
                                            config={"displayModeBar": False, "responsive": True, "scrollZoom": True, "doubleClick": "reset"},
                                            style={"height": "340px", "width": "100%"},
                                        ),
                                    ),
                                    _panel(
                                        "Drawdown",
                                        dcc.Graph(
                                            id="drawdown-graph",
                                            responsive=True,
                                            config={"displayModeBar": False, "responsive": True, "scrollZoom": True, "doubleClick": "reset"},
                                            style={"height": "300px", "width": "100%"},
                                        ),
                                    ),
                                ],
                                id="live-lower-stage",
                                style={"display": "grid", "gridTemplateColumns": "minmax(0, 1fr)", "gap": "12px", "marginTop": "12px"},
                            ),
                        ],
                        id="live-left-stage",
                        style={"minWidth": 0},
                    ),
                    html.Div(
                        [
                            _panel("Execution", html.Div(id="execution-state")),
                            _panel("Account", html.Div(id="account-state")),
                            _panel("Regime", html.Div(id="regime-state")),
                            _panel("Open Orders", _table("open-orders-table")),
                        ],
                        id="live-side-rail",
                        style={"display": "grid", "gap": "12px", "alignContent": "start", "minWidth": 0},
                    ),
                ],
                id="live-main-grid",
                style={"display": "grid", "gridTemplateColumns": "minmax(0, 1.8fr) minmax(300px, 0.85fr)", "gap": "12px"},
            ),
            html.Div(
                [
                    dcc.Tabs(
                        [
                            dcc.Tab(label="Trades", children=[_panel("Trades", _table("trades-table"))]),
                            dcc.Tab(label="Orders", children=[_panel("Orders", _table("orders-table"))]),
                            dcc.Tab(label="Positions", children=[_panel("Positions", _table("positions-table"))]),
                            dcc.Tab(label="Journal", children=[_panel("Journal", _table("journal-table"))]),
                        ],
                        parent_className="tester-tabs",
                        className="tester-tabs-container",
                        content_style={"paddingTop": "10px"},
                        colors={"border": theme["border"], "primary": theme["accent"], "background": "transparent"},
                    )
                ],
                id="live-bottom-panels",
                style={"marginTop": "12px"},
            ),
        ],
        style={"padding": "18px", "background": "radial-gradient(circle at 12% 8%, #16202B 0%, #0B0F14 42%, #06080B 100%)", "color": theme["text"], "minHeight": "100vh", "fontFamily": FONT_STACK},
    )

    @app.callback(
        dash.Output("live-replay-state", "data"),
        [
            dash.Input("live-slow", "n_clicks"),
            dash.Input("live-start", "n_clicks"),
            dash.Input("live-pause", "n_clicks"),
            dash.Input("live-step", "n_clicks"),
            dash.Input("live-fast", "n_clicks"),
            dash.Input("live-skip", "n_clicks"),
            dash.Input("live-zoom-1h", "n_clicks"),
            dash.Input("live-zoom-4h", "n_clicks"),
            dash.Input("live-zoom-1d", "n_clicks"),
            dash.Input("live-zoom-all", "n_clicks"),
            dash.Input("live-latest", "n_clicks"),
            dash.Input("live-follow", "n_clicks"),
            dash.Input("live-focus-replay", "n_clicks"),
            dash.Input("live-focus-equity", "n_clicks"),
            dash.Input("live-focus-drawdown", "n_clicks"),
            dash.Input("live-focus-reset", "n_clicks"),
            dash.Input("live-toggle-side", "n_clicks"),
            dash.Input("live-toggle-bottom", "n_clicks"),
            dash.Input("live-profile-balanced", "n_clicks"),
            dash.Input("live-profile-low-latency", "n_clicks"),
            dash.Input("live-profile-high-history", "n_clicks"),
        ],
        dash.State("live-replay-state", "data"),
        prevent_initial_call=True,
    )
    def control_replay(
        _slow,
        _start,
        _pause,
        _step,
        _fast,
        _skip,
        _zoom_1h,
        _zoom_4h,
        _zoom_1d,
        _zoom_all,
        _latest,
        _follow,
        _focus_replay,
        _focus_equity,
        _focus_drawdown,
        _focus_reset,
        _toggle_side,
        _toggle_bottom,
        _profile_balanced,
        _profile_low_latency,
        _profile_high_history,
        state,
    ):
        current_profile = _normalize_profile_name((state or {}).get("profile", default_profile))
        profile_cfg = _profile_settings(current_profile)
        state = dict(
            state
            or {
                "playing": False,
                "speed": int(profile_cfg["speed"]),
                "zoom_bars": int(profile_cfg["zoom_bars"]),
                "show_side": True,
                "show_bottom": True,
                "follow_latest": True,
                "focus_panel": "replay",
                "profile": current_profile,
            }
        )
        trigger = dash.callback_context.triggered_id
        playing = bool(state.get("playing", False))
        profile_name = _normalize_profile_name(state.get("profile", default_profile))
        speed = int(state.get("speed", _profile_settings(profile_name)["speed"]))
        zoom_bars = int(state.get("zoom_bars", _profile_settings(profile_name)["zoom_bars"]))
        show_side = bool(state.get("show_side", True))
        show_bottom = bool(state.get("show_bottom", True))
        follow_latest = bool(state.get("follow_latest", True))
        focus_panel = str(state.get("focus_panel", "replay"))
        if trigger == "live-start":
            playing = True
            with playback_lock:
                playback_state["playing"] = True
        elif trigger == "live-pause":
            playing = False
            with playback_lock:
                playback_state["playing"] = False
        elif trigger == "live-slow":
            speed = max(1, speed // 2)
        elif trigger == "live-fast":
            speed = min(128, speed * 2)
        elif trigger == "live-zoom-1h":
            zoom_bars = 60
            follow_latest = True
        elif trigger == "live-zoom-4h":
            zoom_bars = 240
            follow_latest = True
        elif trigger == "live-zoom-1d":
            zoom_bars = 1440
            follow_latest = True
        elif trigger == "live-zoom-all":
            zoom_bars = 0
            follow_latest = False
        elif trigger == "live-latest":
            follow_latest = True
        elif trigger == "live-follow":
            follow_latest = not follow_latest
        elif trigger == "live-focus-replay":
            focus_panel = "replay"
            show_bottom = True
        elif trigger == "live-focus-equity":
            focus_panel = "equity"
            show_bottom = True
        elif trigger == "live-focus-drawdown":
            focus_panel = "drawdown"
            show_bottom = True
        elif trigger == "live-focus-reset":
            focus_panel = "replay"
            show_bottom = True
            show_side = True
        elif trigger == "live-toggle-side":
            show_side = not show_side
        elif trigger == "live-toggle-bottom":
            show_bottom = not show_bottom
        elif trigger == "live-profile-balanced":
            profile_name = "balanced"
        elif trigger == "live-profile-low-latency":
            profile_name = "low_latency"
        elif trigger == "live-profile-high-history":
            profile_name = "high_history"
        elif trigger in {"live-step", "live-skip"} and advance_step is not None:
            iterations = 1 if trigger == "live-step" else 50
            running = True
            for _ in range(iterations):
                running = _safe_advance_step()
                if not running:
                    break
            with playback_lock:
                playback_state["playing"] = False
            playing = False
            _broadcast_snapshot()

        if trigger in {"live-profile-balanced", "live-profile-low-latency", "live-profile-high-history"}:
            profile_cfg = _profile_settings(profile_name)
            speed = int(profile_cfg["speed"])
            zoom_bars = int(profile_cfg["zoom_bars"])

        return {
            "playing": playing,
            "speed": speed,
            "zoom_bars": zoom_bars,
            "show_side": show_side,
            "show_bottom": show_bottom,
            "follow_latest": follow_latest,
            "focus_panel": focus_panel,
            "profile": profile_name,
        }

    @app.callback(
        [
            dash.Output("live-main-grid", "style"),
            dash.Output("live-side-rail", "style"),
            dash.Output("live-bottom-panels", "style"),
            dash.Output("live-lower-stage", "style"),
            dash.Output("replay-graph", "style"),
            dash.Output("equity-graph", "style"),
            dash.Output("drawdown-graph", "style"),
            dash.Output("live-start", "style"),
            dash.Output("live-pause", "style"),
            dash.Output("live-step", "style"),
            dash.Output("live-slow", "style"),
            dash.Output("live-fast", "style"),
            dash.Output("live-skip", "style"),
            dash.Output("live-zoom-1h", "style"),
            dash.Output("live-zoom-4h", "style"),
            dash.Output("live-zoom-1d", "style"),
            dash.Output("live-zoom-all", "style"),
            dash.Output("live-latest", "style"),
            dash.Output("live-follow", "style"),
            dash.Output("live-focus-replay", "style"),
            dash.Output("live-focus-equity", "style"),
            dash.Output("live-focus-drawdown", "style"),
            dash.Output("live-focus-reset", "style"),
            dash.Output("live-toggle-side", "style"),
            dash.Output("live-toggle-bottom", "style"),
            dash.Output("live-profile-balanced", "style"),
            dash.Output("live-profile-low-latency", "style"),
            dash.Output("live-profile-high-history", "style"),
        ],
        dash.Input("live-replay-state", "data"),
    )
    def update_layout_visibility(state):
        state = dict(state or {})
        playing = bool(state.get("playing", False))
        profile_name = _normalize_profile_name(state.get("profile", default_profile))
        profile_cfg = _profile_settings(profile_name)
        zoom_bars = int(state.get("zoom_bars", int(profile_cfg["zoom_bars"])))
        show_side = bool(state.get("show_side", True))
        show_bottom = bool(state.get("show_bottom", True))
        follow_latest = bool(state.get("follow_latest", True))
        focus_panel = str(state.get("focus_panel", "replay"))
        main_grid_style = {
            "display": "grid",
            "gridTemplateColumns": "minmax(0, 1fr)" if not show_side else "minmax(0, 1.8fr) minmax(300px, 0.85fr)",
            "gap": "12px",
        }
        side_rail_style = {
            "display": "grid" if show_side else "none",
            "gap": "12px",
            "alignContent": "start",
            "minWidth": 0,
        }
        bottom_style = {"marginTop": "12px", "display": "block" if show_bottom else "none"}
        lower_stage_style = {
            "display": "grid" if show_bottom else "none",
            "gridTemplateColumns": "minmax(0, 1fr) minmax(0, 1fr)" if focus_panel == "replay" else "minmax(0, 1fr)",
            "gap": "12px",
            "marginTop": "12px",
        }
        replay_height = "820px" if not show_bottom else "640px"
        equity_height = "340px"
        drawdown_height = "300px"
        if focus_panel == "replay":
            replay_height = "820px" if not show_bottom else "760px"
        elif focus_panel == "equity":
            replay_height = "420px"
            equity_height = "620px"
            drawdown_height = "220px"
        elif focus_panel == "drawdown":
            replay_height = "420px"
            equity_height = "220px"
            drawdown_height = "520px"
        replay_style = {"height": replay_height}
        equity_style = {"height": equity_height, "width": "100%"}
        drawdown_style = {"height": drawdown_height, "width": "100%"}
        if focus_panel == "replay":
            lower_stage_style["alignItems"] = "stretch"

        return (
            main_grid_style,
            side_rail_style,
            bottom_style,
            lower_stage_style,
            replay_style,
            equity_style,
            drawdown_style,
            _button_style(active=playing, accent=True),
            _button_style(active=not playing),
            _button_style(),
            _button_style(),
            _button_style(),
            _button_style(),
            _button_style(active=zoom_bars == 60),
            _button_style(active=zoom_bars == 240),
            _button_style(active=zoom_bars == 1440),
            _button_style(active=zoom_bars == 0),
            _button_style(active=follow_latest, accent=follow_latest),
            _button_style(active=follow_latest),
            _button_style(active=focus_panel == "replay"),
            _button_style(active=focus_panel == "equity"),
            _button_style(active=focus_panel == "drawdown"),
            _button_style(active=focus_panel == "replay" and show_side and show_bottom),
            _button_style(active=show_side),
            _button_style(active=show_bottom),
            _button_style(active=profile_name == "balanced"),
            _button_style(active=profile_name == "low_latency"),
            _button_style(active=profile_name == "high_history"),
        )

    @app.callback(
        [
            dash.Output("trades-table", "data"),
            dash.Output("trades-table", "columns"),
            dash.Output("positions-table", "data"),
            dash.Output("positions-table", "columns"),
            dash.Output("open-orders-table", "data"),
            dash.Output("open-orders-table", "columns"),
            dash.Output("orders-table", "data"),
            dash.Output("orders-table", "columns"),
            dash.Output("journal-table", "data"),
            dash.Output("journal-table", "columns"),
            dash.Output("regime-state", "children"),
            dash.Output("execution-state", "children"),
            dash.Output("account-state", "children"),
            dash.Output("live-status", "children"),
            dash.Output("live-symbol", "children"),
            dash.Output("live-fills", "children"),
            dash.Output("live-orders", "children"),
            dash.Output("live-profile", "children"),
            dash.Output("live-cb-latency", "children"),
            dash.Output("live-speed", "children"),
            dash.Output("live-window", "children"),
            dash.Output("live-ohlc", "children"),
        ],
        [
            dash.Input("live-replay-state", "data"),
            dash.Input("live-ws-snapshot", "data"),
        ],
    )
    def refresh(_state, _ws_snapshot):
        payload = _build_live_payload(_ws_snapshot or _safe_snapshot_provider())
        state = dict(_state or {})
        profile_name = _normalize_profile_name(state.get("profile", default_profile))
        profile_cfg = _profile_settings(profile_name)
        speed = int(state.get("speed", int(profile_cfg["speed"])))
        zoom_bars = int(state.get("zoom_bars", int(profile_cfg["zoom_bars"])))
        equity_df = _normalize_equity_curve(payload.get("account_curve", payload.get("equity")))
        trades_df = _normalize_table(payload.get("recent_fills"))
        positions_df = _normalize_table(payload.get("positions"))
        orders_df = _normalize_table(payload.get("orders"))
        journal_df = _normalize_table(payload.get("journal"))
        regime = _normalize_regime_state(payload.get("regime"))
        headline = dict(payload.get("headline", {}))
        execution = dict(payload.get("execution", {}))
        account = dict(payload.get("account", {}))
        snapshot = payload.get("snapshot", {})

        max_table_rows = int(profile_cfg["table_rows"])
        positions_data = positions_df.tail(max_table_rows).to_dict("records")
        positions_cols = [{"name": c, "id": c} for c in positions_df.columns]
        trades_data = trades_df.tail(max_table_rows).to_dict("records")
        trades_cols = [{"name": c, "id": c} for c in trades_df.columns]
        orders_data = orders_df.tail(max_table_rows).to_dict("records")
        orders_cols = [{"name": c, "id": c} for c in orders_df.columns]
        journal_data = journal_df.tail(max_table_rows).to_dict("records")
        journal_cols = [{"name": c, "id": c} for c in journal_df.columns]
        regime_text = html.Div(
            [
                html.Div(f"Regime: {regime.get('regime', 'n/a')}", style={"fontWeight": "600"}),
                html.Div(f"Confidence: {regime.get('confidence', 'n/a')}", style={"color": theme["muted"], "marginTop": "4px", "fontVariantNumeric": "tabular-nums"}),
            ]
        )
        execution_text = html.Div(
            _kv_rows(
                [
                    ("Fills", execution.get("fill_count", len(trades_df.index))),
                    ("Open Orders", execution.get("open_order_count", len(orders_df.index))),
                    ("Total Cost", _format_metric(execution.get("total_cost", "n/a"))),
                    ("Maker Ratio", _format_metric(execution.get("maker_fill_ratio", "n/a"))),
                ]
            )
        )
        account_text = html.Div(
            _kv_rows(
                [
                    ("Equity", _format_metric(headline.get("equity", "n/a"))),
                    ("Buying Power", _format_metric(headline.get("buying_power", "n/a"))),
                    ("Available Funds", _format_metric(account.get("available_funds", "n/a"))),
                    ("Margin Excess", _format_metric(account.get("margin_excess", "n/a"))),
                ]
            )
        )
        timestamp = snapshot.get("timestamp") or "n/a"
        symbols = payload.get("setup", {}).get("symbols", [])
        symbol_text = f"symbol={symbols[0]}" if symbols else "symbol=n/a"
        status = f"time={timestamp}"
        fills_text = f"fills={execution.get('fill_count', len(trades_df.index))}"
        orders_text = f"open_orders={execution.get('open_order_count', len(orders_df.index))}"
        profile_text = f"profile={profile_name}"
        callback_text = _telemetry_badge()
        speed_text = f"speed={speed}"
        window_text = "window=all" if zoom_bars == 0 else f"window={zoom_bars}"
        ohlc_text = "ohlc=n/a"
        _maybe_emit_telemetry_log(profile_name, speed, zoom_bars)
        market_bars = payload.get("market_bars")
        if market_bars is not None and not market_bars.empty and "timestamp" in market_bars.columns:
            visible_bars = market_bars.copy()
            visible_bars["timestamp"] = visible_bars["timestamp"].astype("datetime64[ns]")
            if timestamp != "n/a":
                current_cutoff = pd.to_datetime(timestamp, errors="coerce")
                if pd.notna(current_cutoff):
                    visible_bars = visible_bars[visible_bars["timestamp"] <= current_cutoff]
            if not visible_bars.empty:
                last_bar = visible_bars.iloc[-1]
                ohlc_text = (
                    "ohlc="
                    f"{_format_metric(last_bar.get('open'))}/"
                    f"{_format_metric(last_bar.get('high'))}/"
                    f"{_format_metric(last_bar.get('low'))}/"
                    f"{_format_metric(last_bar.get('close'))}"
                )

        return (
            trades_data,
            trades_cols,
            positions_data,
            positions_cols,
            orders_data,
            orders_cols,
            orders_data,
            orders_cols,
            journal_data,
            journal_cols,
            regime_text,
            execution_text,
            account_text,
            status,
            symbol_text,
            fills_text,
            orders_text,
            profile_text,
            callback_text,
            speed_text,
            window_text,
            ohlc_text,
        )

    @app.callback(
        [
            dash.Output("equity-graph", "figure"),
            dash.Output("drawdown-graph", "figure"),
        ],
        [
            dash.Input("live-ws-snapshot", "data"),
        ],
        [
            dash.State("equity-graph", "figure"),
            dash.State("drawdown-graph", "figure"),
        ],
    )
    def refresh_equity_chart(_ws_snapshot, current_equity_figure, current_drawdown_figure):
        payload = _build_live_payload(_ws_snapshot or _safe_snapshot_provider())
        next_equity_figure, next_drawdown_figure = _build_live_equity_figure(payload)

        def _patch_graph(current_figure, next_figure):
            if not current_figure or not isinstance(current_figure, Mapping):
                return next_figure
            patch = Patch()
            patch["data"] = list(next_figure.data)
            patch["layout"]["title"] = next_figure.layout.title
            return patch

        return (
            _patch_graph(current_equity_figure, next_equity_figure),
            _patch_graph(current_drawdown_figure, next_drawdown_figure),
        )

    @app.callback(
        dash.Output("equity-graph", "extendData"),
        dash.Input("live-ws-message", "data"),
        dash.State("equity-graph", "figure"),
        prevent_initial_call=True,
    )
    def extend_equity_chart(_ws_message, current_figure):
        return dash.no_update

    @app.callback(
        dash.Output("drawdown-graph", "extendData"),
        dash.Input("live-ws-message", "data"),
        dash.State("drawdown-graph", "figure"),
        prevent_initial_call=True,
    )
    def extend_drawdown_chart(_ws_message, current_figure):
        return dash.no_update

    @app.callback(
        dash.Output("replay-graph", "figure"),
        [
            dash.Input("live-replay-state", "data"),
            dash.Input("live-ws-message", "data"),
        ],
        [
            dash.State("live-ws-snapshot", "data"),
            dash.State("replay-graph", "figure"),
        ],
    )
    def refresh_chart(_state, _ws_message, _ws_snapshot, current_figure):
        started = time.perf_counter()

        def _done(value: Any) -> Any:
            _record_latency("replay_patch_ms", (time.perf_counter() - started) * 1000.0)
            return value

        payload = _build_live_payload(_ws_snapshot or _safe_snapshot_provider())
        state = dict(_state or {})
        profile_name = _normalize_profile_name(state.get("profile", default_profile))
        profile_cfg = _profile_settings(profile_name)
        zoom_bars = int(state.get("zoom_bars", int(profile_cfg["zoom_bars"])))
        follow_latest = bool(state.get("follow_latest", True))
        trigger = dash.callback_context.triggered_id

        if trigger == "live-ws-message" and current_figure and isinstance(current_figure, Mapping):
            if not follow_latest:
                return _done(dash.no_update)
            bars = _get_normalized_market_bars(payload)
            if bars.empty:
                return _done(dash.no_update)
            snapshot = payload.get("snapshot", {})
            cutoff = pd.to_datetime(snapshot.get("timestamp"), errors="coerce")
            target_end = int((bars["timestamp"] <= cutoff).sum()) if pd.notna(cutoff) else len(bars.index)
            if target_end <= 0:
                return _done(dash.no_update)
            window_start = 0 if zoom_bars == 0 else max(0, target_end - zoom_bars)
            active_window = bars.iloc[window_start:target_end]
            if active_window.empty:
                return _done(dash.no_update)
            low = float(active_window["low"].min())
            high = float(active_window["high"].max())
            spread = max(high - low, abs(high) * 0.001, 1e-6)
            pad = spread * 0.06

            cutoff_ts = active_window["timestamp"].iloc[-1]
            window_start_ts = active_window["timestamp"].iloc[0]
            current_close = float(active_window["close"].iloc[-1])
            shapes, annotations = _build_overlay_state(
                payload,
                profile_name,
                profile_cfg,
                window_start_ts,
                cutoff_ts,
                current_close,
            )
            patch = Patch()
            patch["layout"]["xaxis"]["range"] = [active_window["timestamp"].iloc[0], active_window["timestamp"].iloc[-1]]
            patch["layout"]["yaxis"]["range"] = [low - pad, high + pad]
            patch["layout"]["shapes"] = shapes
            patch["layout"]["annotations"] = annotations
            return _done(patch)

        next_figure = _build_execution_replay_figure(
            payload,
            include_animation=False,
            window_bars=zoom_bars,
            max_order_annotations=int(profile_cfg["max_overlay_annotations"]),
        )
        if not current_figure or not isinstance(current_figure, Mapping):
            return _done(next_figure)

        try:
            patch = Patch()
            patch["data"] = list(next_figure.data)
            patch["layout"]["shapes"] = list(next_figure.layout.shapes)
            patch["layout"]["annotations"] = list(next_figure.layout.annotations)
            trigger = dash.callback_context.triggered_id
            if (trigger == "live-replay-state" or (trigger == "live-ws-snapshot" and follow_latest)) and getattr(next_figure.layout.xaxis, "range", None):
                patch["layout"]["xaxis"]["range"] = list(next_figure.layout.xaxis.range)
            if (trigger == "live-replay-state" or (trigger == "live-ws-snapshot" and follow_latest)) and getattr(next_figure.layout.yaxis, "range", None):
                patch["layout"]["yaxis"]["range"] = list(next_figure.layout.yaxis.range)
            patch["layout"]["title"] = next_figure.layout.title
            return _done(patch)
        except Exception:
            return _done(next_figure)

    @app.callback(
        dash.Output("replay-graph", "extendData"),
        dash.Input("live-ws-message", "data"),
        [
            dash.State("live-replay-state", "data"),
            dash.State("live-ws-snapshot", "data"),
            dash.State("replay-graph", "figure"),
        ],
        prevent_initial_call=True,
    )
    def extend_replay_chart(_ws_message, _state, _ws_snapshot, current_figure):
        started = time.perf_counter()

        def _done(value: Any) -> Any:
            _record_latency("replay_extend_ms", (time.perf_counter() - started) * 1000.0)
            return value

        message = _ws_message if isinstance(_ws_message, Mapping) else {}
        if message.get("type") not in {"snapshot", "delta"}:
            return _done(dash.no_update)
        if not current_figure or not isinstance(current_figure, Mapping):
            return _done(dash.no_update)

        figure_data = current_figure.get("data", [])
        if isinstance(figure_data, tuple):
            figure_data = list(figure_data)
        if not isinstance(figure_data, list) or not figure_data:
            return _done(dash.no_update)

        payload = _build_live_payload(_ws_snapshot or _safe_snapshot_provider())
        bars = _get_normalized_market_bars(payload)
        if bars.empty:
            return _done(dash.no_update)

        snapshot = payload.get("snapshot", {})
        cutoff = pd.to_datetime(snapshot.get("timestamp"), errors="coerce")
        target_end = int((bars["timestamp"] <= cutoff).sum()) if pd.notna(cutoff) else len(bars.index)
        if target_end <= 0:
            return _done(dash.no_update)

        state = dict(_state or {})
        profile_name = _normalize_profile_name(state.get("profile", default_profile))
        profile_cfg = _profile_settings(profile_name)
        zoom_bars = max(0, int(state.get("zoom_bars", int(profile_cfg["zoom_bars"]))))
        window_start = 0 if zoom_bars == 0 else max(0, target_end - zoom_bars)
        window_start_ts = bars["timestamp"].iloc[window_start]

        candlestick_index = next((i for i, trace in enumerate(figure_data) if str(trace.get("type", "")).lower() == "candlestick"), None)
        if candlestick_index is None:
            return _done(dash.no_update)

        candle_trace = figure_data[candlestick_index]
        existing_x = candle_trace.get("x", [])
        if isinstance(existing_x, tuple):
            existing_x = list(existing_x)
        start_index = window_start
        if isinstance(existing_x, list) and existing_x:
            last_visible = pd.to_datetime(existing_x[-1], errors="coerce")
            if pd.notna(last_visible):
                start_index = max(window_start, int(bars["timestamp"].searchsorted(last_visible, side="right")))
        if start_index >= target_end:
            return _done(dash.no_update)

        appended_bars = bars.iloc[start_index:target_end]
        if appended_bars.empty:
            return _done(dash.no_update)
        profile_append = int(profile_cfg["max_append_bars"])
        max_append_bars = min(profile_append, max(96, zoom_bars if zoom_bars > 0 else min(profile_append, 512)))
        if len(appended_bars.index) > max_append_bars:
            appended_bars = appended_bars.tail(max_append_bars)

        trace_indices: list[int] = []
        trace_updates: list[dict[str, Any]] = []

        def add_trace_update(index: int, values: dict[str, Any]) -> None:
            trace_indices.append(index)
            trace_updates.append(values)

        add_trace_update(
            candlestick_index,
            {
                "x": appended_bars["timestamp"].tolist(),
                "open": appended_bars["open"].tolist(),
                "high": appended_bars["high"].tolist(),
                "low": appended_bars["low"].tolist(),
                "close": appended_bars["close"].tolist(),
            },
        )

        trades_df = _normalize_table(payload.get("recent_fills"))
        if not trades_df.empty and {"timestamp", "price", "quantity"}.issubset(trades_df.columns):
            trades = trades_df.copy()
            trades["timestamp"] = pd.to_datetime(trades["timestamp"], errors="coerce")
            trades = trades.dropna(subset=["timestamp"])
            if not trades.empty:
                trades = trades[(trades["timestamp"] >= window_start_ts) & (trades["timestamp"] <= bars["timestamp"].iloc[target_end - 1])]
                side_trace_names = (("Buy Fills", True), ("Sell Fills", False))
                for trace_name, is_buy in side_trace_names:
                    trace_index = next((i for i, trace in enumerate(figure_data) if trace.get("name") == trace_name), None)
                    if trace_index is None:
                        continue
                    side_trades = trades[trades["quantity"] > 0] if is_buy else trades[trades["quantity"] < 0]
                    if side_trades.empty:
                        continue
                    trace_x = figure_data[trace_index].get("x", [])
                    if isinstance(trace_x, tuple):
                        trace_x = list(trace_x)
                    if isinstance(trace_x, list) and trace_x:
                        last_marker_time = pd.to_datetime(trace_x[-1], errors="coerce")
                        if pd.notna(last_marker_time):
                            side_trades = side_trades[side_trades["timestamp"] > last_marker_time]
                    if side_trades.empty:
                        continue
                    add_trace_update(
                        trace_index,
                        {
                            "x": side_trades["timestamp"].tolist(),
                            "y": side_trades["price"].tolist(),
                            "customdata": side_trades["quantity"].tolist(),
                        },
                    )

        if not trace_updates:
            return _done(dash.no_update)

        update_payload: dict[str, list[Any]] = {}
        keys = {key for values in trace_updates for key in values}
        for key in keys:
            update_payload[key] = [values.get(key, []) for values in trace_updates]

        if zoom_bars > 0:
            max_points = max(320, min(int(profile_cfg["max_points_window_cap"]), zoom_bars + 96))
        else:
            max_points = int(profile_cfg["max_points_all"])
        return _done((update_payload, trace_indices, max_points))

    return app


def launch_dashboard(results: Any, host: str = "127.0.0.1", port: int = 8050, debug: bool = False) -> None:
    app = create_dash_app(results)
    if hasattr(app, "run"):
        app.run(host=host, port=port, debug=debug, threaded=False)
    else:
        app.run_server(host=host, port=port, debug=debug, threaded=False)


def launch_live_dashboard(snapshot_provider: Callable[[], Mapping[str, Any]],
                          host: str = "127.0.0.1",
                          port: int = 8050,
                          debug: bool = False,
                          interval_ms: int = 1000,
                          advance_step: Optional[Callable[[], bool]] = None) -> None:
    app = create_live_dash_app(snapshot_provider, interval_ms=interval_ms, advance_step=advance_step)
    if hasattr(app, "run"):
        app.run(host=host, port=port, debug=debug, threaded=False)
    else:
        app.run_server(host=host, port=port, debug=debug, threaded=False)


def export_dashboard_html(results: Any, path: str) -> str:
    try:
        import plotly.io as pio
    except Exception as exc:
        raise ImportError("export_dashboard_html requires plotly. Install with `regimeflow[viz]`.") from exc

    payload = create_strategy_tester_dashboard(results)
    main_figure = payload.get("figure")
    venue_figure = payload.get("venue_figure")

    headline_html = ""
    if payload.get("headline"):
        headline_rows = "".join(
            f"<tr><th>{key}</th><td>{value}</td></tr>"
            for key, value in payload["headline"].items()
        )
        headline_html = (
            "<section><h2>Report</h2>"
            "<table border='1' cellspacing='0' cellpadding='6'>"
            f"{headline_rows}</table></section>"
        )

    optimization_html = ""
    optimization = payload.get("optimization", {})
    if optimization.get("enabled"):
        summary_rows = "".join(
            f"<tr><th>{key}</th><td>{value}</td></tr>"
            for key, value in optimization.get("summary", {}).items()
        )
        windows_html = optimization["windows"].to_html(index=False) if not optimization["windows"].empty else "<p>No windows.</p>"
        params_html = optimization["params"].reset_index().to_html(index=False) if not optimization["params"].empty else "<p>No parameter evolution.</p>"
        optimization_html = (
            "<section><h2>Optimization</h2>"
            "<table border='1' cellspacing='0' cellpadding='6'>"
            f"{summary_rows}</table>{windows_html}{params_html}</section>"
        )

    html = (
        "<html><head><meta charset='utf-8'><title>RegimeFlow Strategy Tester</title></head>"
        "<body style='background:#0B0F14;color:#D7E1EA;font-family:IBM Plex Sans,Segoe UI,sans-serif;padding:24px;'>"
        "<h1>RegimeFlow Strategy Tester</h1>"
        f"<section>{pio.to_html(main_figure, include_plotlyjs='cdn', full_html=False)}</section>"
        f"<section>{pio.to_html(venue_figure, include_plotlyjs=False, full_html=False) if venue_figure is not None else ''}</section>"
        f"{headline_html}"
        f"{optimization_html}"
        "</body></html>"
    )
    with open(path, "w", encoding="utf-8") as f:
        f.write(html)
    return path


__all__ = [
    "create_dash_app",
    "launch_dashboard",
    "create_live_dash_app",
    "launch_live_dashboard",
    "export_dashboard_html",
]
