#!/usr/bin/env python3
"""
Strategy return correlation matrix for Algo-Catalyst.

Runs the backtest binary on each config file in configs/, collects per-trade
PnL sequences, and plots a Pearson correlation heatmap. Strategies with low
correlation can be combined in a portfolio for diversification.

Usage:
    python3 scripts/correlation_analysis.py --configs configs/*.json
"""

import argparse
import csv
import subprocess
import tempfile
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    import sys
    print("[ERROR] matplotlib and numpy required: pip install matplotlib numpy")
    sys.exit(1)


def run_backtest(binary: Path, cfg: Path, out_csv: Path) -> list[float]:
    try:
        subprocess.run(
            [str(binary), "--config", str(cfg), "--output", str(out_csv)],
            capture_output=True, timeout=60
        )
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []

    pnls = []
    if out_csv.exists():
        with open(out_csv) as f:
            for row in csv.DictReader(f):
                try:
                    pnls.append(float(row.get("PnL", 0)))
                except ValueError:
                    pass
    return pnls


def pearson(a: list[float], b: list[float]) -> float:
    n = min(len(a), len(b))
    if n < 2:
        return float("nan")
    a, b = a[:n], b[:n]
    ma, mb = sum(a) / n, sum(b) / n
    num = sum((x - ma) * (y - mb) for x, y in zip(a, b))
    da  = sum((x - ma) ** 2 for x in a) ** 0.5
    db  = sum((y - mb) ** 2 for y in b) ** 0.5
    return num / (da * db) if da * db > 0 else 0.0


def main():
    parser = argparse.ArgumentParser(description="Strategy correlation analysis")
    parser.add_argument("--configs", nargs="+", type=Path, default=list(Path("configs").glob("*.json")))
    parser.add_argument("--binary",  type=Path, default=Path("build/AlgoCatalyst"))
    parser.add_argument("--output",  type=Path, default=Path("correlation.png"))
    args = parser.parse_args()

    if not args.binary.exists():
        print(f"[ERROR] Binary {args.binary} not found — build the project first.")
        return

    configs = sorted(args.configs)
    if not configs:
        print("[ERROR] No config files found.")
        return

    series: dict[str, list[float]] = {}
    with tempfile.TemporaryDirectory() as tmpdir:
        for cfg in configs:
            name = cfg.stem
            print(f"  Running {name}...", flush=True)
            out = Path(tmpdir) / f"{name}_trades.csv"
            pnls = run_backtest(args.binary, cfg, out)
            if pnls:
                series[name] = pnls

    if len(series) < 2:
        print("[WARN] Need at least 2 strategies with trades to compute correlation.")
        return

    labels = list(series.keys())
    n = len(labels)
    matrix = [[pearson(series[a], series[b]) for b in labels] for a in labels]

    fig, ax = plt.subplots(figsize=(max(5, n * 1.5), max(4, n * 1.5)))
    fig.patch.set_facecolor("#0d1117")
    ax.set_facecolor("#0d1117")

    arr = np.array(matrix)
    im = ax.imshow(arr, cmap="RdYlGn", vmin=-1, vmax=1)
    cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    cbar.ax.yaxis.set_tick_params(color="white")
    plt.setp(cbar.ax.yaxis.get_ticklabels(), color="white")

    ax.set_xticks(range(n))
    ax.set_yticks(range(n))
    ax.set_xticklabels(labels, rotation=30, ha="right", color="white", fontsize=9)
    ax.set_yticklabels(labels, color="white", fontsize=9)
    ax.set_title("Strategy Return Correlation", color="white", fontsize=12, pad=12)

    for i in range(n):
        for j in range(n):
            val = matrix[i][j]
            txt = f"{val:.2f}" if not (val != val) else "N/A"
            ax.text(j, i, txt, ha="center", va="center",
                    color="black" if 0.3 <= abs(val) <= 0.7 else "white",
                    fontsize=8, fontweight="bold")

    for sp in ax.spines.values(): sp.set_edgecolor("#30363d")
    plt.tight_layout()
    plt.savefig(args.output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Correlation matrix saved to {args.output}")


if __name__ == "__main__":
    main()
