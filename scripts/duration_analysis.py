#!/usr/bin/env python3
"""
Trade duration analysis for Algo-Catalyst.

Computes trade holding time in seconds and shows how duration correlates
with PnL, win rate, and trade frequency across duration buckets.

Usage:
    python3 scripts/duration_analysis.py --trades trades.csv
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


def duration_sec(trade: dict) -> float:
    entry = trade.get("Entry_Time_US", trade.get("entry_time_us", 0))
    exit_ = trade.get("Exit_Time_US",  trade.get("exit_time_us",  0))
    try:
        return max(0.0, (float(exit_) - float(entry)) / 1e6)
    except (ValueError, TypeError):
        return 0.0


def bucket(secs: float) -> str:
    if secs < 60:      return "<1 min"
    if secs < 300:     return "1–5 min"
    if secs < 900:     return "5–15 min"
    if secs < 3600:    return "15–60 min"
    return ">1 hr"


BUCKET_ORDER = ["<1 min", "1–5 min", "5–15 min", "15–60 min", ">1 hr"]


def main():
    parser = argparse.ArgumentParser(description="Trade duration analysis")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--output", type=Path, default=Path("duration_analysis.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    trades  = load_trades(args.trades)
    durations = [duration_sec(t) for t in trades]
    pnls      = [float(t.get("PnL", 0)) for t in trades]

    buckets: dict[str, list[float]] = defaultdict(list)
    for d, p in zip(durations, pnls):
        buckets[bucket(d)].append(p)

    present = [b for b in BUCKET_ORDER if b in buckets]
    totals  = [sum(buckets[b]) for b in present]
    winrates = [100.0 * sum(1 for p in buckets[b] if p > 0) / len(buckets[b]) for b in present]
    counts   = [len(buckets[b]) for b in present]

    print(f"\n{'Bucket':<12} {'Count':>7} {'Total PnL':>12} {'Win%':>8}")
    print("-" * 44)
    for b, tot, wr, ct in zip(present, totals, winrates, counts):
        print(f"{b:<12} {ct:>7} ${tot:>10.2f} {wr:>7.1f}%")

    fig, axes = plt.subplots(1, 3, figsize=(14, 5))
    fig.patch.set_facecolor("#0d1117")

    charts = [
        (totals,   "Total PnL by Duration",       "PnL ($)"),
        (winrates, "Win Rate by Duration (%)",     "Win Rate (%)"),
        (counts,   "Trade Count by Duration",      "# Trades"),
    ]
    for ax, (vals, title, ylabel) in zip(axes, charts):
        ax.set_facecolor("#0d1117")
        colors = ["#3fb950" if v >= 0 else "#f85149" for v in vals]
        ax.bar(present, vals, color=colors, alpha=0.8, edgecolor="#0d1117")
        ax.axhline(0, color="#30363d", linewidth=0.8)
        ax.set_title(title, color="white", fontsize=9)
        ax.set_ylabel(ylabel, color="white", fontsize=8)
        ax.tick_params(colors="white", labelsize=7)
        plt.setp(ax.get_xticklabels(), rotation=20, ha="right")
        for sp in ax.spines.values(): sp.set_edgecolor("#30363d")

    plt.tight_layout()
    plt.savefig(args.output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"\nChart saved to {args.output}")


if __name__ == "__main__":
    main()
