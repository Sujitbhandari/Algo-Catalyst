#!/usr/bin/env python3
"""
Monte Carlo simulation for Algo-Catalyst.

Resamples the historical trade PnL sequence N times to build a distribution
of possible equity curves, then reports percentile outcomes and the probability
of ruin (equity dropping below a given threshold).

Usage:
    python3 scripts/monte_carlo.py --trades trades.csv --sims 1000 --ruin -500
"""

import argparse
import csv
import random
import math
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


def load_pnls(path: Path) -> list[float]:
    pnls = []
    with open(path) as f:
        for row in csv.DictReader(f):
            try:
                pnls.append(float(row.get("PnL", 0)))
            except ValueError:
                pass
    return pnls


def run_simulation(pnls: list[float], n_sims: int, ruin_threshold: float, seed: int):
    random.seed(seed)
    n = len(pnls)
    all_curves = []
    final_equities = []
    ruin_count = 0

    for _ in range(n_sims):
        sample = random.choices(pnls, k=n)
        equity = 0.0
        curve = []
        ruined = False
        for p in sample:
            equity += p
            curve.append(equity)
            if equity <= ruin_threshold and not ruined:
                ruin_count += 1
                ruined = True
        all_curves.append(curve)
        final_equities.append(equity)

    return all_curves, final_equities, ruin_count


def plot_simulation(all_curves, final_equities, ruin_threshold, output: Path):
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.patch.set_facecolor("#0d1117")

    n_trades = len(all_curves[0])
    xs = list(range(1, n_trades + 1))

    ax1 = axes[0]
    ax1.set_facecolor("#0d1117")

    curves_arr = np.array(all_curves)
    p5  = np.percentile(curves_arr, 5,  axis=0)
    p25 = np.percentile(curves_arr, 25, axis=0)
    p50 = np.percentile(curves_arr, 50, axis=0)
    p75 = np.percentile(curves_arr, 75, axis=0)
    p95 = np.percentile(curves_arr, 95, axis=0)

    # Show a random sample of curves
    sample_indices = random.sample(range(len(all_curves)), min(50, len(all_curves)))
    for i in sample_indices:
        ax1.plot(xs, all_curves[i], color="#58a6ff", alpha=0.06, linewidth=0.7)

    ax1.fill_between(xs, p5,  p95, alpha=0.15, color="#3fb950", label="5–95%")
    ax1.fill_between(xs, p25, p75, alpha=0.25, color="#3fb950", label="25–75%")
    ax1.plot(xs, p50, color="#3fb950", linewidth=1.5, label="Median")
    ax1.axhline(ruin_threshold, color="#f85149", linewidth=1.0, linestyle="--", label="Ruin")
    ax1.axhline(0, color="#30363d", linewidth=0.6)
    ax1.set_title("Monte Carlo Equity Curves", color="white")
    ax1.set_xlabel("Trade #", color="white")
    ax1.set_ylabel("Equity ($)", color="white")
    ax1.tick_params(colors="white")
    ax1.legend(facecolor="#161b22", edgecolor="#30363d", labelcolor="white", fontsize=8)
    for sp in ax1.spines.values(): sp.set_edgecolor("#30363d")

    ax2 = axes[1]
    ax2.set_facecolor("#0d1117")
    arr = np.array(final_equities)
    bins = np.linspace(arr.min() * 1.05, arr.max() * 1.05, 40)
    ax2.hist(arr[arr >= 0], bins=bins, color="#3fb950", alpha=0.7, label="Profitable")
    ax2.hist(arr[arr <  0], bins=bins, color="#f85149", alpha=0.7, label="Losing")
    ax2.axvline(np.median(arr), color="white", linewidth=1.2,
                linestyle="--", label=f"Median ${np.median(arr):.0f}")
    ax2.set_title("Final Equity Distribution", color="white")
    ax2.set_xlabel("Final Equity ($)", color="white")
    ax2.set_ylabel("Frequency", color="white")
    ax2.tick_params(colors="white")
    ax2.legend(facecolor="#161b22", edgecolor="#30363d", labelcolor="white", fontsize=8)
    for sp in ax2.spines.values(): sp.set_edgecolor("#30363d")

    plt.tight_layout()
    plt.savefig(output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Chart saved to {output}")


def main():
    parser = argparse.ArgumentParser(description="Monte Carlo simulation")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--sims",   type=int,  default=1000, help="Number of simulations")
    parser.add_argument("--ruin",   type=float, default=-500.0,
                        help="Ruin threshold in $ (default: -500)")
    parser.add_argument("--seed",   type=int,  default=42)
    parser.add_argument("--output", type=Path, default=Path("monte_carlo.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    pnls = load_pnls(args.trades)
    if not pnls:
        print("[ERROR] No trades found in CSV")
        return

    print(f"Running {args.sims:,} simulations on {len(pnls)} trades...")
    curves, finals, ruin = run_simulation(pnls, args.sims, args.ruin, args.seed)

    finals_arr = [f for f in finals]
    print(f"\n  Median final equity:  ${sorted(finals_arr)[len(finals_arr)//2]:.2f}")
    print(f"  5th pct final equity: ${sorted(finals_arr)[int(0.05*len(finals_arr))]:.2f}")
    print(f"  95th pct:             ${sorted(finals_arr)[int(0.95*len(finals_arr))]:.2f}")
    print(f"  Probability of ruin:  {100*ruin/args.sims:.1f}%  "
          f"(equity <= ${args.ruin:.0f})")

    plot_simulation(curves, finals, args.ruin, args.output)


if __name__ == "__main__":
    main()
