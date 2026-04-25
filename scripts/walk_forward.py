#!/usr/bin/env python3
"""
Walk-forward validation for Algo-Catalyst.

Splits generated tick data into in-sample (IS) and out-of-sample (OOS)
windows, runs the backtest on each, and reports how well the IS performance
predicts OOS performance.

Usage:
    python3 scripts/walk_forward.py --scenario catalyst --ticks 20000 \
        --windows 5 --is-ratio 0.7
"""

import argparse
import subprocess
import sys
import os
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BINARY = ROOT / "build" / "AlgoCatalyst"
DATA_DIR = ROOT / "data"
SCRIPTS_DIR = ROOT / "scripts"


def generate_data(scenario: str, ticks: int, output: Path) -> bool:
    result = subprocess.run(
        [sys.executable, str(SCRIPTS_DIR / "generate_data.py"),
         "--scenario", scenario, "--ticks", str(ticks), "--output", str(output)],
        capture_output=True, text=True
    )
    return result.returncode == 0


def run_backtest(data_file: Path, output_file: Path,
                 stop_loss: float = 2.0, take_profit: float = 6.0) -> dict:
    if not BINARY.exists():
        print(f"[ERROR] Binary not found: {BINARY}")
        return {}

    result = subprocess.run(
        [str(BINARY),
         "--data", str(data_file),
         "--symbol", "TEST",
         "--output", str(output_file),
         "--stop-loss", str(stop_loss),
         "--take-profit", str(take_profit)],
        capture_output=True, text=True
    )

    metrics = {}
    for line in result.stdout.splitlines():
        line = line.strip()
        for key in ("Total PnL", "Win Rate", "Profit Factor", "Sharpe Ratio",
                    "Max Drawdown", "Trades"):
            if key in line:
                parts = line.split(":")
                if len(parts) >= 2:
                    val_str = parts[-1].strip().replace("$", "").replace("%", "")
                    try:
                        metrics[key] = float(val_str.split()[0])
                    except (ValueError, IndexError):
                        pass
    return metrics


def split_csv(source: Path, dest: Path, start_ratio: float, end_ratio: float):
    with open(source) as f:
        lines = f.readlines()

    header = lines[0]
    data = lines[1:]
    n = len(data)
    start = int(n * start_ratio)
    end = int(n * end_ratio)
    chunk = data[start:end]

    with open(dest, "w") as f:
        f.write(header)
        f.writelines(chunk)


def main():
    parser = argparse.ArgumentParser(description="Walk-forward validation")
    parser.add_argument("--scenario", default="catalyst",
                        choices=["catalyst", "meanrev", "trending", "volatile"])
    parser.add_argument("--ticks", type=int, default=20000)
    parser.add_argument("--windows", type=int, default=5,
                        help="Number of walk-forward windows")
    parser.add_argument("--is-ratio", type=float, default=0.7,
                        help="In-sample fraction of each window (default 0.70)")
    args = parser.parse_args()

    print(f"Walk-forward validation — scenario={args.scenario} "
          f"ticks={args.ticks} windows={args.windows} IS={args.is_ratio:.0%}")
    print()

    full_data = DATA_DIR / f"wf_{args.scenario}.csv"
    print(f"Generating {args.ticks} ticks...")
    if not generate_data(args.scenario, args.ticks, full_data):
        print("[WARN] Data generation failed or script not found; using existing data.")
        if not full_data.exists():
            full_data = DATA_DIR / "tick_data.csv"

    window_size = 1.0 / args.windows
    results = []

    for w in range(args.windows):
        start = w * window_size
        end = min(start + window_size, 1.0)
        is_end = start + (end - start) * args.is_ratio

        is_file  = DATA_DIR / f"wf_is_{w}.csv"
        oos_file = DATA_DIR / f"wf_oos_{w}.csv"
        is_trades  = DATA_DIR / f"wf_is_trades_{w}.csv"
        oos_trades = DATA_DIR / f"wf_oos_trades_{w}.csv"

        split_csv(full_data, is_file,  start, is_end)
        split_csv(full_data, oos_file, is_end, end)

        is_metrics  = run_backtest(is_file,  is_trades)
        oos_metrics = run_backtest(oos_file, oos_trades)

        results.append({"window": w + 1, "is": is_metrics, "oos": oos_metrics})

        pnl_is  = is_metrics.get("Total PnL", 0.0)
        pnl_oos = oos_metrics.get("Total PnL", 0.0)
        print(f"  Window {w+1:2d}  IS PnL: ${pnl_is:8.2f}  "
              f"OOS PnL: ${pnl_oos:8.2f}  "
              f"{'✓' if pnl_oos > 0 else '✗'}")

    print()
    oos_pnls = [r["oos"].get("Total PnL", 0.0) for r in results]
    profitable_oos = sum(1 for p in oos_pnls if p > 0)
    print(f"OOS profitable windows: {profitable_oos}/{args.windows}")
    print(f"Average OOS PnL:        ${sum(oos_pnls)/len(oos_pnls):.2f}")

    # Clean up temp files
    for w in range(args.windows):
        for f in [DATA_DIR / f"wf_is_{w}.csv", DATA_DIR / f"wf_oos_{w}.csv",
                  DATA_DIR / f"wf_is_trades_{w}.csv", DATA_DIR / f"wf_oos_trades_{w}.csv"]:
            if f.exists():
                os.remove(f)


if __name__ == "__main__":
    main()
