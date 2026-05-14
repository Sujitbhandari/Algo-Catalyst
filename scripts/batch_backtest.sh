#!/usr/bin/env bash
# batch_backtest.sh — run the same backtest across multiple config files
# and collect the results into a summary CSV.
#
# Usage:
#   ./scripts/batch_backtest.sh configs/aggressive.json configs/conservative.json
#   ./scripts/batch_backtest.sh configs/*.json

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BINARY="$ROOT/build/AlgoCatalyst"
SUMMARY="$ROOT/batch_results.csv"

if [[ ! -f "$BINARY" ]]; then
    echo "[INFO] Binary not found — building..."
    cmake -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -S "$ROOT" -q
    cmake --build "$ROOT/build" -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
fi

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <config1.json> [config2.json ...]"
    exit 1
fi

echo "Config,Total_PnL,Win_Rate,Sharpe,Profit_Factor,Max_DD,Trades" > "$SUMMARY"

for cfg in "$@"; do
    name="$(basename "$cfg" .json)"
    trades_file="$ROOT/batch_${name}_trades.csv"

    echo -n "  Running $name ... "

    "$BINARY" --config "$cfg" --output "$trades_file" > /tmp/_batch_out.txt 2>&1 || true

    # Extract metrics from output
    pnl=$(grep "Total PnL"     /tmp/_batch_out.txt | grep -o '[0-9.-]*' | tail -1 || echo "0")
    wr=$(grep  "Win Rate"      /tmp/_batch_out.txt | grep -o '[0-9.]*'  | head -1 || echo "0")
    sh=$(grep  "Sharpe Ratio"  /tmp/_batch_out.txt | grep -o '[0-9.-]*' | tail -1 || echo "0")
    pf=$(grep  "Profit Factor" /tmp/_batch_out.txt | grep -o '[0-9.]*'  | head -1 || echo "0")
    dd=$(grep  "Max Drawdown"  /tmp/_batch_out.txt | grep -o '[0-9.]*'  | tail -1 || echo "0")
    tr=$(grep  "Trades:"       /tmp/_batch_out.txt | grep -o '[0-9]*'   | head -1 || echo "0")

    echo "$name,$pnl,$wr,$sh,$pf,$dd,$tr" >> "$SUMMARY"
    echo "PnL=\$$pnl  WR=${wr}%  Sharpe=$sh"
done

echo ""
echo "Summary written to $SUMMARY"
