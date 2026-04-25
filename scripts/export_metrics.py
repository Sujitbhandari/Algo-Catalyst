#!/usr/bin/env python3
"""
Export backtest metrics to JSON.

Reads a trades CSV file, computes the full set of performance metrics, and
writes them to a JSON file. Useful for dashboards, CI checks, or downstream
analysis in other tools.

Usage:
    python3 scripts/export_metrics.py --trades trades.csv --output metrics.json
"""

import argparse
import csv
import json
import math
import sys
from pathlib import Path
from datetime import datetime


def load_trades(path: Path) -> list[dict]:
    trades = []
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            t = {}
            for k, v in row.items():
                try:
                    t[k] = float(v)
                except (ValueError, TypeError):
                    t[k] = v
            trades.append(t)
    return trades


def compute_metrics(trades: list[dict]) -> dict:
    if not trades:
        return {"error": "no trades"}

    pnls = [t.get("PnL", 0.0) for t in trades]
    n = len(pnls)

    wins  = [p for p in pnls if p > 0]
    losses = [p for p in pnls if p <= 0]

    total_pnl   = sum(pnls)
    gross_win   = sum(wins)
    gross_loss  = abs(sum(losses))
    win_rate    = 100.0 * len(wins) / n if n else 0.0
    avg_win     = sum(wins) / len(wins) if wins else 0.0
    avg_loss    = sum(losses) / len(losses) if losses else 0.0
    profit_factor = gross_win / gross_loss if gross_loss else 0.0

    mean = total_pnl / n
    variance = sum((p - mean) ** 2 for p in pnls) / max(n - 1, 1)
    std_dev = math.sqrt(variance) if variance > 0 else 0.0
    sharpe  = mean / std_dev if std_dev else 0.0

    down_sq = sum(p * p for p in pnls if p < 0)
    down_std = math.sqrt(down_sq / max(len(losses), 1)) if losses else 0.0
    sortino = mean / down_std if down_std else 0.0

    peak = eq = max_dd = 0.0
    for p in pnls:
        eq += p
        peak = max(peak, eq)
        max_dd = max(max_dd, peak - eq)

    calmar = total_pnl / max_dd if max_dd else 0.0
    recovery = total_pnl / max_dd if max_dd else 0.0

    sorted_pnls = sorted(pnls)
    var95_idx = max(0, int(0.05 * n) - 1)
    var99_idx = max(0, int(0.01 * n) - 1)
    var95 = -sorted_pnls[var95_idx]
    var99 = -sorted_pnls[var99_idx]
    cvar95_vals = sorted_pnls[:max(1, int(0.05 * n))]
    cvar95 = -sum(cvar95_vals) / len(cvar95_vals)

    best  = max(pnls)
    worst = min(pnls)
    expectancy = (win_rate / 100.0) * avg_win + (1 - win_rate / 100.0) * avg_loss

    hold_times = []
    for t in trades:
        entry = t.get("Entry_Time_US", 0)
        exit_ = t.get("Exit_Time_US", 0)
        if exit_ and entry:
            hold_times.append((exit_ - entry) / 1e6)
    avg_hold = sum(hold_times) / len(hold_times) if hold_times else 0.0

    max_consec_w = max_consec_l = cur_w = cur_l = 0
    for p in pnls:
        if p > 0:
            cur_w += 1; cur_l = 0
            max_consec_w = max(max_consec_w, cur_w)
        else:
            cur_l += 1; cur_w = 0
            max_consec_l = max(max_consec_l, cur_l)

    return {
        "generated_at": datetime.utcnow().isoformat() + "Z",
        "num_trades":       n,
        "num_wins":         len(wins),
        "num_losses":       len(losses),
        "win_rate_pct":     round(win_rate, 2),
        "total_pnl":        round(total_pnl, 4),
        "gross_profit":     round(gross_win, 4),
        "gross_loss":       round(gross_loss, 4),
        "profit_factor":    round(profit_factor, 4),
        "avg_win":          round(avg_win, 4),
        "avg_loss":         round(avg_loss, 4),
        "best_trade":       round(best, 4),
        "worst_trade":      round(worst, 4),
        "expectancy":       round(expectancy, 4),
        "max_drawdown":     round(max_dd, 4),
        "sharpe_ratio":     round(sharpe, 4),
        "sortino_ratio":    round(sortino, 4),
        "calmar_ratio":     round(calmar, 4),
        "recovery_factor":  round(recovery, 4),
        "var_95":           round(var95, 4),
        "var_99":           round(var99, 4),
        "cvar_95":          round(cvar95, 4),
        "avg_hold_sec":     round(avg_hold, 2),
        "max_consec_wins":  max_consec_w,
        "max_consec_losses":max_consec_l,
    }


def main():
    parser = argparse.ArgumentParser(description="Export backtest metrics to JSON")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--output", type=Path, default=Path("metrics.json"))
    parser.add_argument("--pretty", action="store_true",
                        help="Pretty-print the JSON (default: compact)")
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] Trades file not found: {args.trades}")
        sys.exit(1)

    trades = load_trades(args.trades)
    metrics = compute_metrics(trades)

    indent = 2 if args.pretty else None
    with open(args.output, "w") as f:
        json.dump(metrics, f, indent=indent)
        if not args.pretty:
            f.write("\n")

    print(f"Exported {len(trades)} trades → {args.output}")
    print(f"  Total PnL: ${metrics.get('total_pnl', 0):.2f}  "
          f"Sharpe: {metrics.get('sharpe_ratio', 0):.3f}  "
          f"Win rate: {metrics.get('win_rate_pct', 0):.1f}%")


if __name__ == "__main__":
    main()
