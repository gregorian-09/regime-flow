from __future__ import annotations

import argparse
import pathlib
from dataclasses import dataclass
from typing import Tuple

import numpy as np
import torch
import torch.nn as nn


@dataclass
class TrainConfig:
    csv_path: str
    output_path: str
    window: int = 120
    feature_dim: int = 8
    batch_size: int = 64
    epochs: int = 3
    lr: float = 1e-3
    max_rows: int = 0


class RegimeTransformer(nn.Module):
    def __init__(self, d_model: int = 8, nhead: int = 2, num_layers: int = 2) -> None:
        super().__init__()
        encoder_layer = nn.TransformerEncoderLayer(d_model=d_model, nhead=nhead, batch_first=True)
        self.encoder = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
        self.proj = nn.Linear(d_model, 4)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        h = self.encoder(x)
        return self.proj(h[:, -1, :])


def parse_args() -> TrainConfig:
    parser = argparse.ArgumentParser(description="Train a simple transformer regime model")
    parser.add_argument("--csv", default="examples/python_custom_regime_ensemble/data_intraday/BTCUSDT.csv")
    parser.add_argument("--output", default="examples/python_transformer_regime/regime_transformer.pt")
    parser.add_argument("--window", type=int, default=120)
    parser.add_argument("--epochs", type=int, default=3)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--max-rows", type=int, default=0)
    args = parser.parse_args()
    return TrainConfig(
        csv_path=args.csv,
        output_path=args.output,
        window=args.window,
        batch_size=args.batch_size,
        epochs=args.epochs,
        lr=args.lr,
        max_rows=args.max_rows,
    )


def load_csv(path: str, max_rows: int) -> np.ndarray:
    data = np.genfromtxt(path, delimiter=",", skip_header=1, max_rows=max_rows or None)
    # columns: timestamp, open, high, low, close, volume
    return data


def build_features(data: np.ndarray) -> np.ndarray:
    # Build 8-dim features per row
    # [close, range, volume, open, close-open, vwap(approx), trade_count(0), bias]
    open_ = data[:, 1]
    high = data[:, 2]
    low = data[:, 3]
    close = data[:, 4]
    volume = data[:, 5]
    vwap = close
    trade_count = np.zeros_like(close)
    bias = np.ones_like(close)
    feats = np.stack([
        close,
        high - low,
        volume,
        open_,
        close - open_,
        vwap,
        trade_count,
        bias,
    ], axis=1)
    # standardize per feature
    mean = feats.mean(axis=0)
    std = feats.std(axis=0) + 1e-8
    feats = (feats - mean) / std
    return feats


def build_labels(data: np.ndarray) -> np.ndarray:
    # Simple heuristic: next return -> bull/bear, with neutral band and crisis on large drawdown
    close = data[:, 4]
    returns = np.diff(close, prepend=close[0]) / np.maximum(close, 1e-8)
    labels = np.zeros_like(returns, dtype=np.int64)
    # 0 bull, 1 neutral, 2 bear, 3 crisis
    labels[returns > 0.0005] = 0
    labels[(returns <= 0.0005) & (returns >= -0.0005)] = 1
    labels[returns < -0.0005] = 2
    # crisis on extreme negatives
    labels[returns < -0.005] = 3
    return labels


def make_dataset(feats: np.ndarray, labels: np.ndarray, window: int) -> Tuple[torch.Tensor, torch.Tensor]:
    xs = []
    ys = []
    for i in range(window, len(feats)):
        xs.append(feats[i - window:i])
        ys.append(labels[i])
    x = torch.tensor(np.array(xs), dtype=torch.float32)
    y = torch.tensor(np.array(ys), dtype=torch.int64)
    return x, y


def main() -> None:
    torch.manual_seed(42)
    cfg = parse_args()
    data = load_csv(cfg.csv_path, cfg.max_rows)
    feats = build_features(data)
    labels = build_labels(data)
    x, y = make_dataset(feats, labels, cfg.window)

    model = RegimeTransformer(d_model=cfg.feature_dim)
    optimizer = torch.optim.Adam(model.parameters(), lr=cfg.lr)
    loss_fn = nn.CrossEntropyLoss()

    model.train()
    for epoch in range(cfg.epochs):
        perm = torch.randperm(x.size(0))
        x_shuf = x[perm]
        y_shuf = y[perm]
        total_loss = 0.0
        for i in range(0, x_shuf.size(0), cfg.batch_size):
            xb = x_shuf[i:i + cfg.batch_size]
            yb = y_shuf[i:i + cfg.batch_size]
            optimizer.zero_grad()
            logits = model(xb)
            loss = loss_fn(logits, yb)
            loss.backward()
            optimizer.step()
            total_loss += loss.item()
        print(f"epoch {epoch+1}: loss={total_loss:.4f}")

    model.eval()
    scripted = torch.jit.script(model)

    out_path = pathlib.Path(cfg.output_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    scripted.save(out_path)
    print(f"Saved TorchScript model to {out_path}")


if __name__ == "__main__":
    main()
