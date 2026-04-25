#!/usr/bin/env python3
"""
Side-by-side strategy comparison for Algo-Catalyst.

Generates data for each scenario, runs the backtest, loads the resulting
trade CSVs, and prints a comparison table with the key metrics for each.

Usage:
    python3 scripts/compare_strategies.py --ticks 10000
"""

import argparse
import subprocess
import sys
import csv
from pathlib import Path

ROOT       = Path(__file__).resolve().parent.parent
BINARY     = ROOT / "build" / "AlgoCatalyst"
DATA_DIR   = ROOT / "data"
SCRIPTS_DIR = ROOT / "scripts"

SCENARIOS = ["catalyst", "meanrev", "trending", "volatile"]


def generate_data(scenario: str, ticks: int) -> Path:
    out = DATA_DIR / f"{scenario}.csv"
    subprocess.run(
        [sys.executable, str(SCRIPTS_DIR / "generate_data.py"),
         "--scenario", scenario, "--ticks", str(ticks), "--output", str(out)],
        capture_output=True, text=True
    )
    return out


def run_and_parse(data_file: Path, trades_file: Path) -> dict:
    if not BINARY.exists():
        return {}

    result = subprocess.run(
        [str(BINARY),
         "--data", str(data_file),
         "--symbol", "CMP",
         "--output", str(trades_file)],
        capture_output=True, text=True
    )

    metrics: dict = {}
    label_map = {
        "Total PnL":     "pnl",
        "Win Rate":      "win_rate",
        "Profit Factor": "profit_factor",
        "Sharpe Ratio":  "sharpe",
        "Max Drawdown":  "max_dd",
        "Trades":        "num_trades",
        "Avg Hold":      "avg_hold_s",
        "Sortino":       "sortino",
        "Calmar":        "calmar",
    }
    for line in result.stdout.splitlines():
        for label, key in label_map.items():
            if label in line:
                parts = line.split(":")
                if len(parts) >= 2:
                    try:
                        metrics[key] = float(
                            parts[-1].strip().replace("$", "")
                            .replace("%", "").split()[0]
                        )
                    except (ValueError, IndexError):
                        pass

    # Augment with trade-log stats if the CSV exists
    if trades_file.exists():
        pnls = []
        try:
            with open(trades_file) as f:
                reader = csv.DictReader(f)
                for row in reader:
                    try:
                        pnls.append(float(row.get("PnL", 0)))
                    except ValueError:
                        pass
        except Exception:
            pass
        metrics["num_trades"] = len(pnls)

    return metrics


def fmt(val, prefix="", suffix="", decimals=2) -> str:
    if val is None:
        return "N/A"
    return f"{prefix}{val:.{decimals}f}{suffix}"


def main():
    parser = argparse.ArgumentParser(description="Strategy comparison across scenarios")
    parser.add_argument("--ticks", type=int, default=10000)
    args = parser.parse_args()

    if not BINARY.exists():
        print(f"[ERROR] Binary not found at {BINARY}. Run: cmake --build build")
        return

    all_results = {}
    for scenario in SCENARIOS:
        print(f"Running {scenario}...", end=" ", flush=True)
        data = generate_data(scenario, args.ticks)
        trades = DATA_DIR / f"cmp_{scenario}_trades.csv"
        m = run_and_parse(data, trades)
        all_results[scenario] = m
        print("done")

    print()
    col_w = 14
    header = f"{'Metric':<22}" + "".join(f"{s:>{col_w}}" for s in SCENARIOS)
    print(header)
    print("-" * len(header))

    rows = [
        ("Trades",          "num_trades",   "",  "",  0),
        ("Total PnL ($)",   "pnl",          "$", "",  2),
        ("Win Rate (%)",     "win_rate",     "",  "%", 1),
        ("Profit Factor",   "profit_factor","",  "",  2),
        ("Sharpe Ratio",    "sharpe",       "",  "",  3),
        ("Sortino Ratio",   "sortino",      "",  "",  3),
        ("Calmar Ratio",    "calmar",       "",  "",  3),
        ("Max Drawdown ($)","max_dd",       "$", "",  2),
        ("Avg Hold (s)",    "avg_hold_s",   "",  "s", 1),
    ]

    for label, key, prefix, suffix, decs in rows:
        row = f"{label:<22}"
        for scenario in SCENARIOS:
            val = all_results[scenario].get(key)
            row += f"{fmt(val, prefix, suffix, decs):>{col_w}}"
        print(row)

    print()
    print("Data files: data/<scenario>.csv  |  Trade logs: data/cmp_<scenario>_trades.csv")


if __name__ == "__main__":
    main()
