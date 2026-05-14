#!/usr/bin/env python3
"""
MAE/MFE scatter plot for Algo-Catalyst.

Plots Maximum Adverse Excursion (MAE) vs Maximum Favorable Excursion (MFE)
for every trade. Winners appear green, losers red. Quadrant lines show
whether the trade captured most of its MFE or gave back too much of its MAE.

Usage:
    python3 scripts/plot_mae_mfe.py --trades trades.csv
"""

import argparse
import csv
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


def main():
    parser = argparse.ArgumentParser(description="MAE/MFE scatter plot")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--output", type=Path, default=Path("mae_mfe.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    trades = load_trades(args.trades)

    # Check if MAE/MFE columns exist
    if trades and "MAE" not in trades[0] and "mae" not in trades[0]:
        print("[WARN] MAE/MFE columns not found — rebuild with v1.3.0+ and re-run backtest.")
        return

    mae_key = "MAE" if "MAE" in trades[0] else "mae"
    mfe_key = "MFE" if "MFE" in trades[0] else "mfe"

    wins   = [t for t in trades if t.get("PnL", 0) > 0]
    losses = [t for t in trades if t.get("PnL", 0) <= 0]

    fig, axes = plt.subplots(1, 2, figsize=(13, 6))
    fig.patch.set_facecolor("#0d1117")
    fig.suptitle("MAE / MFE Analysis", color="white", fontsize=13, fontweight="bold")

    for ax, group, label, color in [
        (axes[0], wins,   "Winners", "#3fb950"),
        (axes[1], losses, "Losers",  "#f85149"),
    ]:
        ax.set_facecolor("#0d1117")
        if group:
            maes = [abs(t.get(mae_key, 0)) for t in group]
            mfes = [t.get(mfe_key, 0) for t in group]
            pnls = [t.get("PnL", 0) for t in group]
            sizes = [max(20, abs(p) * 3) for p in pnls]
            ax.scatter(maes, mfes, c=color, alpha=0.6, s=sizes, edgecolors="none")

            # Diagonal: MFE = MAE (breakeven line)
            lim = max(max(maes, default=1), max(mfes, default=1)) * 1.1
            ax.plot([0, lim], [0, lim], color="#8b949e", linewidth=0.8,
                    linestyle="--", label="MFE = MAE")
            ax.set_xlim(0, lim)
            ax.set_ylim(0, lim)

        ax.set_title(f"{label} ({len(group)})", color="white")
        ax.set_xlabel("MAE (adverse $ move)", color="white")
        ax.set_ylabel("MFE (favorable $ move)", color="white")
        ax.tick_params(colors="white")
        ax.legend(facecolor="#161b22", edgecolor="#30363d", labelcolor="white", fontsize=8)
        for sp in ax.spines.values(): sp.set_edgecolor("#30363d")

    if wins:
        avg_mfe_w = sum(t.get(mfe_key, 0) for t in wins) / len(wins)
        avg_pnl_w = sum(t.get("PnL", 0) for t in wins) / len(wins)
        capture = avg_pnl_w / avg_mfe_w * 100 if avg_mfe_w else 0
        print(f"Winners  — avg MFE: ${avg_mfe_w:.2f}  avg PnL: ${avg_pnl_w:.2f}  "
              f"MFE capture: {capture:.1f}%")
    if losses:
        avg_mae_l = abs(sum(t.get(mae_key, 0) for t in losses) / len(losses))
        avg_pnl_l = sum(t.get("PnL", 0) for t in losses) / len(losses)
        print(f"Losers   — avg MAE: ${avg_mae_l:.2f}  avg PnL: ${avg_pnl_l:.2f}")

    plt.tight_layout()
    plt.savefig(args.output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Chart saved to {args.output}")


if __name__ == "__main__":
    main()
