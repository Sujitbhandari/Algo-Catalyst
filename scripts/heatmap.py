#!/usr/bin/env python3
"""
Heatmap visualization for parameter sensitivity.

Reads optimization_results.csv (produced by optimize_params.py) and plots
2-D heatmaps of Sharpe vs stop-loss/take-profit and trailing-stop/take-profit
combinations so you can see which regions of parameter space are most robust.

Usage:
    python3 scripts/heatmap.py --metric sharpe
    python3 scripts/heatmap.py --metric pnl --output sensitivity.png
"""

import argparse
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS_CSV = ROOT / "optimization_results.csv"

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("[ERROR] matplotlib and numpy are required: pip install matplotlib numpy")
    sys.exit(1)


def load_results(path: Path) -> list[dict]:
    rows = []
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            parsed = {}
            for k, v in row.items():
                try:
                    parsed[k] = float(v)
                except ValueError:
                    parsed[k] = v
            rows.append(parsed)
    return rows


def build_heatmap(rows, x_key, y_key, z_key):
    xs = sorted({r[x_key] for r in rows if x_key in r})
    ys = sorted({r[y_key] for r in rows if y_key in r})

    grid = np.full((len(ys), len(xs)), np.nan)
    for r in rows:
        if x_key not in r or y_key not in r or z_key not in r:
            continue
        xi = xs.index(r[x_key])
        yi = ys.index(r[y_key])
        existing = grid[yi, xi]
        val = r[z_key]
        grid[yi, xi] = val if np.isnan(existing) else max(existing, val)

    return np.array(xs), np.array(ys), grid


def main():
    parser = argparse.ArgumentParser(description="Parameter sensitivity heatmap")
    parser.add_argument("--metric", default="sharpe",
                        choices=["sharpe", "pnl", "profit_factor", "win_rate"])
    parser.add_argument("--results", type=Path, default=RESULTS_CSV)
    parser.add_argument("--output", type=Path, default=ROOT / "sensitivity_heatmap.png")
    args = parser.parse_args()

    if not args.results.exists():
        print(f"[ERROR] {args.results} not found. Run optimize_params.py first.")
        sys.exit(1)

    rows = load_results(args.results)
    print(f"Loaded {len(rows)} parameter combinations from {args.results}")

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.patch.set_facecolor("#0d1117")
    fig.suptitle(f"Parameter Sensitivity — metric: {args.metric}",
                 color="white", fontsize=14, fontweight="bold")

    panels = [
        ("stop_loss", "take_profit", "Stop Loss", "Take Profit"),
        ("trailing",  "take_profit", "Trailing Stop", "Take Profit"),
    ]

    for ax, (xk, yk, xl, yl) in zip(axes, panels):
        xs, ys, grid = build_heatmap(rows, xk, yk, args.metric)
        ax.set_facecolor("#0d1117")

        im = ax.imshow(grid, aspect="auto", origin="lower",
                       cmap="RdYlGn", interpolation="nearest")
        ax.set_xticks(range(len(xs)))
        ax.set_xticklabels([f"{x:.1f}" for x in xs], color="white", fontsize=8)
        ax.set_yticks(range(len(ys)))
        ax.set_yticklabels([f"{y:.1f}" for y in ys], color="white", fontsize=8)
        ax.set_xlabel(xl, color="white")
        ax.set_ylabel(yl, color="white")
        ax.tick_params(colors="white")
        for spine in ax.spines.values():
            spine.set_edgecolor("#30363d")

        cbar = fig.colorbar(im, ax=ax, shrink=0.85)
        cbar.ax.tick_params(colors="white")
        cbar.ax.yaxis.label.set_color("white")

        for yi in range(len(ys)):
            for xi in range(len(xs)):
                val = grid[yi, xi]
                if not np.isnan(val):
                    ax.text(xi, yi, f"{val:.2f}", ha="center", va="center",
                            color="black", fontsize=7)

    plt.tight_layout()
    plt.savefig(args.output, dpi=150, bbox_inches="tight",
                facecolor=fig.get_facecolor())
    print(f"Saved to {args.output}")


if __name__ == "__main__":
    main()
