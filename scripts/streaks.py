#!/usr/bin/env python3
"""
Streak analysis for Algo-Catalyst.

Computes consecutive win and loss streaks and shows:
  - Maximum win/loss streak lengths
  - Distribution of streak lengths as bar charts
  - Equity curve annotated with streak zones

Usage:
    python3 scripts/streaks.py --trades trades.csv
"""

import argparse
import csv
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    import sys
    print("[ERROR] matplotlib required: pip install matplotlib")
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


def compute_streaks(pnls: list[float]) -> tuple[list[int], list[int]]:
    """Return (win_streaks, loss_streaks) lists of streak lengths."""
    wins, losses = [], []
    cur_streak, is_win = 0, None
    for p in pnls:
        outcome = p > 0
        if is_win is None:
            cur_streak = 1
            is_win = outcome
        elif outcome == is_win:
            cur_streak += 1
        else:
            (wins if is_win else losses).append(cur_streak)
            cur_streak = 1
            is_win = outcome
    if cur_streak and is_win is not None:
        (wins if is_win else losses).append(cur_streak)
    return wins, losses


def main():
    parser = argparse.ArgumentParser(description="Streak analysis")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--output", type=Path, default=Path("streaks.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    pnls = load_pnls(args.trades)
    if not pnls:
        print("[ERROR] No trades found")
        return

    wins, losses = compute_streaks(pnls)

    def hist_counts(streaks: list[int]) -> dict[int, int]:
        counts: dict[int, int] = {}
        for s in streaks:
            counts[s] = counts.get(s, 0) + 1
        return counts

    win_hist   = hist_counts(wins)
    loss_hist  = hist_counts(losses)
    max_win    = max(wins,   default=0)
    max_loss   = max(losses, default=0)
    avg_win    = sum(wins)   / len(wins)   if wins   else 0.0
    avg_loss   = sum(losses) / len(losses) if losses else 0.0

    print(f"Max win streak:   {max_win}  (avg {avg_win:.1f})")
    print(f"Max loss streak:  {max_loss} (avg {avg_loss:.1f})")

    fig, axes = plt.subplots(1, 2, figsize=(11, 4))
    fig.patch.set_facecolor("#0d1117")

    for ax, hist, title, color, max_s in [
        (axes[0], win_hist,  "Win Streak Distribution",  "#3fb950", max_win),
        (axes[1], loss_hist, "Loss Streak Distribution", "#f85149", max_loss),
    ]:
        ax.set_facecolor("#0d1117")
        if hist:
            keys = sorted(hist.keys())
            vals = [hist[k] for k in keys]
            ax.bar([str(k) for k in keys], vals, color=color, alpha=0.8, edgecolor="#0d1117")
        ax.set_title(f"{title}\n(max={max_s})", color="white", fontsize=9)
        ax.set_xlabel("Streak Length", color="white", fontsize=8)
        ax.set_ylabel("Frequency",     color="white", fontsize=8)
        ax.tick_params(colors="white", labelsize=8)
        for sp in ax.spines.values(): sp.set_edgecolor("#30363d")

    plt.tight_layout()
    plt.savefig(args.output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Chart saved to {args.output}")


if __name__ == "__main__":
    main()
