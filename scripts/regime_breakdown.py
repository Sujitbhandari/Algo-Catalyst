#!/usr/bin/env python3
"""
Regime breakdown analysis for Algo-Catalyst.

Reads a trades CSV and groups performance metrics by the regime label
(TRENDING, CHOPPY, VOLATILE, UNKNOWN) to show which market conditions
produced the best and worst results.

Usage:
    python3 scripts/regime_breakdown.py --trades trades.csv
"""

import argparse
import csv
import math
from collections import defaultdict
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


def load_trades(path: Path) -> list[dict]:
    rows = []
    with open(path) as f:
        for row in csv.DictReader(f):
            parsed = {}
            for k, v in row.items():
                try:
                    parsed[k] = float(v)
                except (ValueError, TypeError):
                    parsed[k] = v
            rows.append(parsed)
    return rows


def group_by_regime(trades: list[dict]) -> dict:
    groups = defaultdict(list)
    for t in trades:
        regime = str(t.get("Regime", t.get("regime", "UNKNOWN"))).strip()
        groups[regime].append(float(t.get("PnL", t.get("pnl", 0))))
    return dict(groups)


def summarise(pnls: list[float]) -> dict:
    n = len(pnls)
    if n == 0:
        return {}
    wins = [p for p in pnls if p > 0]
    return {
        "n":            n,
        "total_pnl":    sum(pnls),
        "win_rate":     100.0 * len(wins) / n,
        "avg_pnl":      sum(pnls) / n,
        "best":         max(pnls),
        "worst":        min(pnls),
        "profit_factor": sum(wins) / abs(sum(p for p in pnls if p < 0)) if any(p < 0 for p in pnls) else float("inf"),
    }


def main():
    parser = argparse.ArgumentParser(description="Regime breakdown analysis")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--output", type=Path, default=Path("regime_breakdown.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    trades = load_trades(args.trades)
    groups = group_by_regime(trades)

    if not groups:
        print("[WARN] No trades with regime labels found.")
        return

    print(f"\n{'Regime':<12} {'Trades':>7} {'Total PnL':>12} {'Win%':>8} {'Avg PnL':>10} {'PF':>8}")
    print("-" * 60)
    for regime, pnls in sorted(groups.items()):
        s = summarise(pnls)
        pf = f"{s['profit_factor']:.2f}" if math.isfinite(s['profit_factor']) else "∞"
        print(f"{regime:<12} {s['n']:>7} ${s['total_pnl']:>10.2f} "
              f"{s['win_rate']:>7.1f}% ${s['avg_pnl']:>9.2f} {pf:>8}")

    # Bar chart
    regimes = sorted(groups.keys())
    totals = [sum(groups[r]) for r in regimes]
    colors = ["#3fb950" if t >= 0 else "#f85149" for t in totals]

    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    fig.patch.set_facecolor("#0d1117")

    for ax, (values, title, ylabel) in zip(axes, [
        (totals, "Total PnL by Regime ($)", "PnL ($)"),
        ([100.0 * len([p for p in groups[r] if p > 0]) / max(len(groups[r]), 1) for r in regimes],
         "Win Rate by Regime (%)", "Win Rate (%)"),
    ]):
        ax.set_facecolor("#0d1117")
        bar_colors = ["#3fb950" if v >= 0 else "#f85149" for v in values]
        ax.bar(regimes, values, color=bar_colors, alpha=0.8, edgecolor="#0d1117")
        ax.axhline(0, color="#30363d", linewidth=0.8)
        ax.set_title(title, color="white", fontsize=10)
        ax.set_ylabel(ylabel, color="white")
        ax.tick_params(colors="white")
        for sp in ax.spines.values(): sp.set_edgecolor("#30363d")
        for i, (r, v) in enumerate(zip(regimes, values)):
            ax.text(i, v + (max(values) * 0.02 if v >= 0 else min(values) * 0.02),
                    f"{v:.1f}", ha="center", va="bottom" if v >= 0 else "top",
                    color="white", fontsize=8)

    plt.tight_layout()
    plt.savefig(args.output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"\nChart saved to {args.output}")


if __name__ == "__main__":
    main()
