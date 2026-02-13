from __future__ import annotations

import argparse
from dataclasses import dataclass
from typing import Iterable, List

import numpy as np
import regimeflow as rf

try:
    import torch
    import torch.nn as nn
    TORCH_AVAILABLE = True
except Exception:
    TORCH_AVAILABLE = False


@dataclass
class RegimeState:
    label: rf.RegimeType
    confidence: float


class NumpyTransformer:
    """Lightweight transformer-style attention using numpy (no training)."""

    def __init__(self, model_dim: int = 8) -> None:
        self.model_dim = model_dim
        rng = np.random.default_rng(42)
        self.Wq = rng.standard_normal((model_dim, model_dim)) * 0.05
        self.Wk = rng.standard_normal((model_dim, model_dim)) * 0.05
        self.Wv = rng.standard_normal((model_dim, model_dim)) * 0.05
        self.Wo = rng.standard_normal((model_dim, model_dim)) * 0.05

    def forward(self, x: np.ndarray) -> np.ndarray:
        # x: [T, D]
        q = x @ self.Wq
        k = x @ self.Wk
        v = x @ self.Wv
        scale = np.sqrt(self.model_dim)
        attn = (q @ k.T) / scale
        attn = np.exp(attn - attn.max(axis=-1, keepdims=True))
        attn = attn / (attn.sum(axis=-1, keepdims=True) + 1e-8)
        out = attn @ v
        return out @ self.Wo


if TORCH_AVAILABLE:
    class TorchTransformer(nn.Module):
        def __init__(self, d_model: int = 8, nhead: int = 2, num_layers: int = 2) -> None:
            super().__init__()
            encoder_layer = nn.TransformerEncoderLayer(d_model=d_model, nhead=nhead, batch_first=True)
            self.encoder = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
            self.proj = nn.Linear(d_model, 4)

        def forward(self, x: torch.Tensor) -> torch.Tensor:
            h = self.encoder(x)
            logits = self.proj(h[:, -1, :])
            return logits


class TransformerRegimeDetector:
    def __init__(self, window: int = 120, model_dim: int = 8) -> None:
        self.window = window
        self.model_dim = model_dim
        self._features: List[np.ndarray] = []
        self._np_model = NumpyTransformer(model_dim=model_dim)
        self._torch_model = TorchTransformer(d_model=model_dim) if TORCH_AVAILABLE else None

    def update(self, bar: rf.Bar) -> RegimeState:
        feat = np.array([
            bar.close,
            bar.high - bar.low,
            bar.volume,
            bar.open,
            bar.close - bar.open,
            bar.vwap if bar.vwap else bar.close,
            bar.trade_count,
            1.0,
        ], dtype=float)
        self._features.append(feat)
        if len(self._features) < self.window:
            return RegimeState(rf.RegimeType.NEUTRAL, 0.5)
        if len(self._features) > self.window:
            self._features.pop(0)
        x = np.stack(self._features, axis=0)
        x = (x - x.mean(axis=0)) / (x.std(axis=0) + 1e-8)

        if TORCH_AVAILABLE:
            with torch.no_grad():
                logits = self._torch_model(torch.tensor(x[None, :, :], dtype=torch.float32))
                probs = torch.softmax(logits, dim=-1).cpu().numpy()[0]
        else:
            h = self._np_model.forward(x)
            logits = h[-1]
            exp = np.exp(logits - logits.max())
            probs = exp / (exp.sum() + 1e-8)
            if probs.size != 4:
                probs = np.pad(probs, (0, max(0, 4 - probs.size)), constant_values=0.0)[:4]

        regimes = [rf.RegimeType.BULL, rf.RegimeType.NEUTRAL, rf.RegimeType.BEAR, rf.RegimeType.CRISIS]
        idx = int(np.argmax(probs))
        return RegimeState(regimes[idx], float(probs[idx]))


class TransformerRegimeStrategy(rf.Strategy):
    def __init__(self, symbol: str, lookback: int = 120) -> None:
        super().__init__()
        self.symbol = symbol
        self.detector = TransformerRegimeDetector(window=lookback)

    def initialize(self, ctx: rf.StrategyContext) -> None:
        self._ctx = ctx

    def on_bar(self, bar: rf.Bar) -> None:
        if bar.symbol != self.symbol:
            return
        state = self.detector.update(bar)
        if state.label == rf.RegimeType.CRISIS:
            target_dir = 0
        elif state.label == rf.RegimeType.BEAR:
            target_dir = -1
        elif state.label == rf.RegimeType.BULL:
            target_dir = 1
        else:
            target_dir = 0

        portfolio = self._ctx.portfolio()
        qty = int((portfolio.equity * 0.02) / max(bar.close, 1e-8))
        qty = max(qty, 1)
        target_qty = target_dir * qty

        current_qty = self._ctx.get_position(self.symbol)
        delta = target_qty - current_qty
        if delta == 0:
            return
        side = rf.OrderSide.BUY if delta > 0 else rf.OrderSide.SELL
        order = rf.Order(self.symbol, side, rf.OrderType.MARKET, abs(delta))
        self._ctx.submit_order(order)


def main() -> None:
    parser = argparse.ArgumentParser(description="Python transformer regime strategy")
    parser.add_argument("--config", default="examples/python_custom_regime_ensemble/config.yaml")
    args = parser.parse_args()

    cfg = rf.BacktestConfig.from_yaml(args.config)
    engine = rf.BacktestEngine(cfg)
    strategy = TransformerRegimeStrategy(symbol=cfg.symbols[0])
    results = engine.run(strategy)
    print(results.report_json())


if __name__ == "__main__":
    main()
