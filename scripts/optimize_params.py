#!/usr/bin/env python3
"""
Parameter sweep optimizer for Algo-Catalyst.

Runs a grid search over stop-loss, take-profit, and trailing-stop parameters
and ranks results by Sharpe ratio (or any other metric you choose).

Usage:
    python3 scripts/optimize_params.py --scenario catalyst --ticks 10000 \
        --metric sharpe --top 10
"""

import argparse
import subprocess
import sys
import csv
import itertools
from pathlib import Path

ROOT    = Path(__file__).resolve().parent.parent
BINARY  = ROOT / "build" / "AlgoCatalyst"
DATA_DIR = ROOT / "data"
SCRIPTS_DIR = ROOT / "scripts"


def ensure_data(scenario: str, ticks: int, out_file: Path) -> bool:
    if out_file.exists():
        return True
    result = subprocess.run(
        [sys.executable, str(SCRIPTS_DIR / "generate_data.py"),
         "--scenario", scenario, "--ticks", str(ticks), "--output", str(out_file)],
        capture_output=True, text=True
    )
    return result.returncode == 0


def run_backtest(data_file: Path, trades_file: Path,
                 stop_loss: float, take_profit: float,
                 trailing: float, slippage: float) -> dict:
    if not BINARY.exists():
        return {}

    result = subprocess.run(
        [str(BINARY),
         "--data", str(data_file),
         "--symbol", "OPT",
         "--output", str(trades_file),
         "--stop-loss",    str(stop_loss),
         "--take-profit",  str(take_profit),
         "--trailing",     str(trailing),
         "--slippage",     str(slippage)],
        capture_output=True, text=True
    )

    metrics: dict = {
        "stop_loss": stop_loss, "take_profit": take_profit,
        "trailing": trailing, "slippage": slippage,
    }
    label_map = {
        "Total PnL":     "pnl",
        "Win Rate":      "win_rate",
        "Profit Factor": "profit_factor",
        "Sharpe Ratio":  "sharpe",
        "Max Drawdown":  "max_dd",
        "Trades":        "num_trades",
    }
    for line in result.stdout.splitlines():
        for label, key in label_map.items():
            if label in line:
                parts = line.split(":")
                if len(parts) >= 2:
                    try:
                        val = float(parts[-1].strip().replace("$", "")
                                    .replace("%", "").split()[0])
                        metrics[key] = val
                    except (ValueError, IndexError):
                        pass
    return metrics


def main():
    parser = argparse.ArgumentParser(description="Parameter sweep optimizer")
    parser.add_argument("--scenario", default="catalyst",
                        choices=["catalyst", "meanrev", "trending", "volatile"])
    parser.add_argument("--ticks", type=int, default=10000)
    parser.add_argument("--metric", default="sharpe",
                        choices=["sharpe", "pnl", "profit_factor", "win_rate"])
    parser.add_argument("--top", type=int, default=10,
                        help="Show top N parameter combos")
    args = parser.parse_args()

    data_file = DATA_DIR / f"opt_{args.scenario}.csv"
    ensure_data(args.scenario, args.ticks, data_file)

    stop_loss_values   = [1.0, 1.5, 2.0, 2.5, 3.0]
    take_profit_values = [3.0, 4.5, 6.0, 7.5, 9.0]
    trailing_values    = [1.5, 2.0, 3.0, 4.0]
    slippage_values    = [3.0, 5.0, 8.0]

    combos = list(itertools.product(
        stop_loss_values, take_profit_values, trailing_values, slippage_values
    ))

    print(f"Sweeping {len(combos)} parameter combinations "
          f"(scenario={args.scenario}, metric={args.metric})...")

    results = []
    tmp_trades = DATA_DIR / "_opt_trades_tmp.csv"

    for i, (sl, tp, tr, slip) in enumerate(combos):
        if tp <= sl:
            continue
        m = run_backtest(data_file, tmp_trades, sl, tp, tr, slip)
        if m:
            results.append(m)
        if (i + 1) % 20 == 0:
            print(f"  {i+1}/{len(combos)} done...")

    if tmp_trades.exists():
        tmp_trades.unlink()

    if not results:
        print("[ERROR] No results. Is the binary built?")
        return

    results.sort(key=lambda r: r.get(args.metric, float("-inf")), reverse=True)

    print(f"\nTop {args.top} results by {args.metric}:\n")
    header = f"{'SL':>6} {'TP':>6} {'Trail':>6} {'Slip':>6} | "
    header += f"{'Sharpe':>8} {'PnL':>10} {'PF':>8} {'WR%':>8} {'Trades':>7}"
    print(header)
    print("-" * len(header))

    for r in results[:args.top]:
        print(
            f"{r.get('stop_loss',0):>6.1f} "
            f"{r.get('take_profit',0):>6.1f} "
            f"{r.get('trailing',0):>6.1f} "
            f"{r.get('slippage',0):>6.1f} | "
            f"{r.get('sharpe',0):>8.3f} "
            f"${r.get('pnl',0):>9.2f} "
            f"{r.get('profit_factor',0):>8.2f} "
            f"{r.get('win_rate',0):>8.1f} "
            f"{int(r.get('num_trades',0)):>7}"
        )

    out_csv = ROOT / "optimization_results.csv"
    with open(out_csv, "w", newline="") as f:
        if results:
            writer = csv.DictWriter(f, fieldnames=results[0].keys())
            writer.writeheader()
            writer.writerows(results)
    print(f"\nFull results saved to {out_csv}")


if __name__ == "__main__":
    main()
