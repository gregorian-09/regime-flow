from __future__ import annotations

import argparse
import csv
from typing import List

import numpy as np
import regimeflow as rf

try:
    import torch
    import torch.nn as nn
    TORCH_AVAILABLE = True
except Exception:
    TORCH_AVAILABLE = False


class NumpyTransformer:
    def __init__(self, model_dim: int = 8) -> None:
        self.model_dim = model_dim
        rng = np.random.default_rng(42)
        self.Wq = rng.standard_normal((model_dim, model_dim)) * 0.05
        self.Wk = rng.standard_normal((model_dim, model_dim)) * 0.05
        self.Wv = rng.standard_normal((model_dim, model_dim)) * 0.05
        self.Wo = rng.standard_normal((model_dim, model_dim)) * 0.05

    def forward(self, x: np.ndarray) -> np.ndarray:
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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate regime CSV using a transformer-style model")
    parser.add_argument("--config", default="examples/python_custom_regime_ensemble/config.yaml")
    parser.add_argument("--symbol", default=None)
    parser.add_argument("--output", default="examples/python_transformer_regime/regime_signals.csv")
    parser.add_argument("--window", type=int, default=120)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    cfg = rf.BacktestConfig.from_yaml(args.config)
    symbol = args.symbol or cfg.symbols[0]

    np_model = NumpyTransformer()
    torch_model = TorchTransformer() if TORCH_AVAILABLE else None
    regimes = [rf.RegimeType.BULL, rf.RegimeType.NEUTRAL, rf.RegimeType.BEAR, rf.RegimeType.CRISIS]

    class RegimeCsvStrategy(rf.Strategy):
        def initialize(self, ctx: rf.StrategyContext) -> None:
            self._ctx = ctx
            self._features: List[np.ndarray] = []
            self._rows: List[list] = []

        def on_bar(self, bar: rf.Bar) -> None:
            if bar.symbol != symbol:
                return
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
            if len(self._features) < args.window:
                return
            if len(self._features) > args.window:
                self._features.pop(0)

            x = np.stack(self._features, axis=0)
            x = (x - x.mean(axis=0)) / (x.std(axis=0) + 1e-8)

            if TORCH_AVAILABLE:
                with torch.no_grad():
                    logits = torch_model(torch.tensor(x[None, :, :], dtype=torch.float32))
                    probs = torch.softmax(logits, dim=-1).cpu().numpy()[0]
            else:
                h = np_model.forward(x)
                logits = h[-1]
                exp = np.exp(logits - logits.max())
                probs = exp / (exp.sum() + 1e-8)
                if probs.size != 4:
                    probs = np.pad(probs, (0, max(0, 4 - probs.size)), constant_values=0.0)[:4]

            idx = int(np.argmax(probs))
            self._rows.append([
                bar.timestamp.to_string("%Y-%m-%d %H:%M:%S"),
                regimes[idx].name.lower(),
                float(probs[idx]),
                float(probs[0]),
                float(probs[1]),
                float(probs[2]),
                float(probs[3]),
            ])

    engine = rf.BacktestEngine(cfg)
    strategy = RegimeCsvStrategy()
    engine.run(strategy)

    with open(args.output, "w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "regime", "confidence", "p_bull", "p_neutral", "p_bear", "p_crisis"])
        writer.writerows(strategy._rows)

    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
