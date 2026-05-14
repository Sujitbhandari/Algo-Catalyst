#!/usr/bin/env python3
"""
Rolling Sharpe ratio tearsheet for Algo-Catalyst.

Computes and plots a rolling Sharpe ratio alongside a rolling win-rate and
trade frequency bar chart so you can see how the strategy's risk-adjusted
performance evolves over time.

Usage:
    python3 scripts/sharpe_tearsheet.py --trades trades.csv --window 20
"""

import argparse
import csv
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


def rolling_sharpe(pnls: list[float], window: int) -> list[float]:
    result = [float("nan")] * len(pnls)
    for i in range(window - 1, len(pnls)):
        w = pnls[i - window + 1: i + 1]
        mean = sum(w) / window
        var  = sum((x - mean)**2 for x in w) / max(window - 1, 1)
        std  = math.sqrt(var) if var > 0 else 0.0
        result[i] = mean / std if std > 0 else 0.0
    return result


def rolling_winrate(pnls: list[float], window: int) -> list[float]:
    result = [float("nan")] * len(pnls)
    for i in range(window - 1, len(pnls)):
        w = pnls[i - window + 1: i + 1]
        result[i] = 100.0 * sum(1 for p in w if p > 0) / window
    return result


def main():
    parser = argparse.ArgumentParser(description="Rolling Sharpe tearsheet")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--window", type=int,  default=20,
                        help="Rolling window size in trades (default: 20)")
    parser.add_argument("--output", type=Path, default=Path("sharpe_tearsheet.png"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    trades = load_trades(args.trades)
    pnls   = [t.get("PnL", 0.0) for t in trades]

    if len(pnls) < args.window:
        print(f"[WARN] Only {len(pnls)} trades — need at least {args.window} for rolling window.")
        return

    xs    = list(range(1, len(pnls) + 1))
    sharpes = rolling_sharpe(pnls, args.window)
    winrates = rolling_winrate(pnls, args.window)

    equity = []
    eq = 0.0
    for p in pnls:
        eq += p
        equity.append(eq)

    fig, axes = plt.subplots(3, 1, figsize=(12, 9),
                             gridspec_kw={"height_ratios": [2, 1.5, 1.2]})
    fig.patch.set_facecolor("#0d1117")
    plt.subplots_adjust(hspace=0.4)

    def style(ax, title):
        ax.set_facecolor("#0d1117")
        ax.set_title(title, color="white", fontsize=10)
        ax.tick_params(colors="white")
        for sp in ax.spines.values(): sp.set_edgecolor("#30363d")

    ax1 = axes[0]
    style(ax1, "Equity Curve")
    ax1.plot(xs, equity, color="#58a6ff", linewidth=1.4)
    ax1.fill_between(xs, 0, equity, where=[e >= 0 for e in equity], alpha=0.12, color="#3fb950")
    ax1.fill_between(xs, 0, equity, where=[e <  0 for e in equity], alpha=0.12, color="#f85149")
    ax1.axhline(0, color="#30363d", linewidth=0.7)
    ax1.set_ylabel("Equity ($)", color="white")

    ax2 = axes[1]
    style(ax2, f"Rolling Sharpe Ratio (window={args.window})")
    valid_xs = [x for x, s in zip(xs, sharpes) if not math.isnan(s)]
    valid_s  = [s for s in sharpes if not math.isnan(s)]
    pos_xs = [x for x, s in zip(valid_xs, valid_s) if s >= 0]
    neg_xs = [x for x, s in zip(valid_xs, valid_s) if s <  0]
    pos_s  = [s for s in valid_s if s >= 0]
    neg_s  = [s for s in valid_s if s <  0]
    if pos_xs: ax2.fill_between(pos_xs, 0, pos_s, alpha=0.4, color="#3fb950")
    if neg_xs: ax2.fill_between(neg_xs, neg_s, 0, alpha=0.4, color="#f85149")
    ax2.plot(valid_xs, valid_s, color="white", linewidth=1.0)
    ax2.axhline(0, color="#30363d", linewidth=0.7)
    ax2.axhline(1, color="#3fb950", linewidth=0.6, linestyle="--", alpha=0.5)
    ax2.set_ylabel("Sharpe", color="white")

    ax3 = axes[2]
    style(ax3, f"Rolling Win Rate % (window={args.window})")
    valid_xs_w = [x for x, w in zip(xs, winrates) if not math.isnan(w)]
    valid_w    = [w for w in winrates if not math.isnan(w)]
    colors = ["#3fb950" if w >= 50 else "#f85149" for w in valid_w]
    ax3.bar(valid_xs_w, valid_w, color=colors, alpha=0.7, width=1.0)
    ax3.axhline(50, color="white", linewidth=0.7, linestyle="--")
    ax3.set_ylabel("Win Rate %", color="white")
    ax3.set_ylim(0, 100)

    all_valid = [s for s in sharpes if not math.isnan(s)]
    overall   = sum(pnls) / len(pnls)
    std_all   = math.sqrt(sum((p - overall)**2 for p in pnls) / max(len(pnls)-1, 1))
    overall_sharpe = overall / std_all if std_all > 0 else 0.0
    print(f"Overall Sharpe: {overall_sharpe:.3f}")
    print(f"Avg rolling Sharpe ({args.window}): {sum(all_valid)/len(all_valid):.3f}")
    print(f"Min rolling Sharpe: {min(all_valid):.3f}  Max: {max(all_valid):.3f}")

    plt.savefig(args.output, dpi=130, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Tearsheet saved to {args.output}")


if __name__ == "__main__":
    main()
