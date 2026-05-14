#!/usr/bin/env python3
"""
PnL calendar heatmap for Algo-Catalyst.

Groups trades by date and shows daily PnL as a GitHub-style contribution
grid. Green = profitable day, red = losing day, white = no trades.

Usage:
    python3 scripts/pnl_calendar.py --trades trades.csv --year 2024
"""

import argparse
import csv
import math
from collections import defaultdict
from datetime import datetime, date, timedelta
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    import numpy as np
except ImportError:
    import sys
    print("[ERROR] matplotlib and numpy required: pip install matplotlib numpy")
    sys.exit(1)


def load_daily_pnl(path: Path) -> dict:
    daily = defaultdict(float)
    with open(path) as f:
        for row in csv.DictReader(f):
            ts = row.get("Entry_Time_US") or row.get("entry_time_us", "0")
            pnl = float(row.get("PnL", row.get("pnl", 0)))
            try:
                ts_us = int(float(ts))
                dt = datetime.utcfromtimestamp(ts_us / 1e6).date()
                daily[dt] += pnl
            except (ValueError, OSError):
                pass
    return dict(daily)


def main():
    parser = argparse.ArgumentParser(description="PnL calendar heatmap")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--year",   type=int,  default=None,
                        help="Year to display (default: most recent year in data)")
    parser.add_argument("--output", type=Path, default=Path("pnl_calendar.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    daily = load_daily_pnl(args.trades)
    if not daily:
        print("[WARN] No dated trades found — timestamps may not be Unix microseconds.")
        return

    year = args.year or max(d.year for d in daily)
    jan1 = date(year, 1, 1)
    dec31 = date(year, 12, 31)

    # Build week grid
    start = jan1 - timedelta(days=jan1.weekday())
    weeks = []
    cur = start
    while cur <= dec31:
        week = []
        for _ in range(7):
            week.append(cur if jan1 <= cur <= dec31 else None)
            cur += timedelta(days=1)
        weeks.append(week)

    fig, ax = plt.subplots(figsize=(max(14, len(weeks) * 0.45), 3.5))
    fig.patch.set_facecolor("#0d1117")
    ax.set_facecolor("#0d1117")
    ax.set_title(f"Daily PnL — {year}", color="white", fontsize=12, pad=12)

    max_abs = max((abs(v) for v in daily.values()), default=1.0)

    for w_idx, week in enumerate(weeks):
        for d_idx, d in enumerate(week):
            if d is None:
                continue
            pnl = daily.get(d, None)
            if pnl is None:
                color = "#161b22"
            elif pnl > 0:
                intensity = min(pnl / max_abs, 1.0)
                color = (0.10 + 0.14 * intensity, 0.37 + 0.37 * intensity, 0.13 + 0.17 * intensity)
            else:
                intensity = min(abs(pnl) / max_abs, 1.0)
                color = (0.55 + 0.32 * intensity, 0.06 + 0.08 * intensity, 0.04 + 0.05 * intensity)

            rect = mpatches.FancyBboxPatch(
                (w_idx * 0.95, (6 - d_idx) * 0.95),
                0.88, 0.88,
                boxstyle="round,pad=0.04",
                facecolor=color,
                edgecolor="#0d1117",
                linewidth=0.5,
            )
            ax.add_patch(rect)
            if pnl is not None:
                ax.text(w_idx * 0.95 + 0.44, (6 - d_idx) * 0.95 + 0.44,
                        f"{pnl:+.0f}", ha="center", va="center",
                        color="white", fontsize=4.5, fontweight="bold")

    day_labels = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
    for i, lbl in enumerate(reversed(day_labels)):
        ax.text(-0.6, i * 0.95 + 0.44, lbl, va="center",
                color="#8b949e", fontsize=7)

    ax.set_xlim(-1, len(weeks) * 0.95)
    ax.set_ylim(-0.3, 7 * 0.95)
    ax.axis("off")

    total = sum(daily.get(d, 0) for d in daily if d.year == year)
    trading_days = sum(1 for d in daily if d.year == year)
    ax.text(0, -0.2, f"Total: ${total:+.2f}  |  Trading days: {trading_days}",
            color="#8b949e", fontsize=8)

    plt.tight_layout()
    plt.savefig(args.output, dpi=140, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Calendar saved to {args.output}  (year={year}, total=${total:+.2f})")


if __name__ == "__main__":
    main()
