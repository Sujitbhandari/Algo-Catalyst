#!/usr/bin/env bash
# backtest.sh — run a full backtest from a config file with one command.
#
# Usage:
#   ./backtest.sh [config.json] [extra args...]
#
# Examples:
#   ./backtest.sh                          # uses config.json and defaults
#   ./backtest.sh config.json --ticks 20000
#   ./backtest.sh config.json --strategy breakout --json-output trades.json

set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BINARY="$ROOT/build/AlgoCatalyst"
CONFIG="${1:-$ROOT/config.json}"

# Build if needed
if [[ ! -f "$BINARY" ]]; then
    echo "[INFO] Binary not found — building..."
    cmake -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -S "$ROOT" -q
    cmake --build "$ROOT/build" -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
fi

# Parse --ticks from extra args (for data generation)
TICKS=10000
SCENARIO="catalyst"
ARGS=()
i=1
while (( i <= $# )); do
    arg="${!i}"
    if [[ "$arg" == "--ticks" ]]; then
        i=$((i+1)); TICKS="${!i}"
    elif [[ "$arg" == "--scenario" ]]; then
        i=$((i+1)); SCENARIO="${!i}"
    else
        ARGS+=("$arg")
    fi
    i=$((i+1))
done

# Generate data
echo "[INFO] Generating $TICKS ticks for scenario '$SCENARIO'..."
python3 "$ROOT/scripts/generate_data.py" --scenario "$SCENARIO" --ticks "$TICKS"

# Run backtest
shift || true   # drop config arg from positional
echo "[INFO] Running backtest with config: $CONFIG"
"$BINARY" --config "$CONFIG" "${ARGS[@]:-}"

# Generate report if trades.csv exists
TRADES="${TRADES_FILE:-trades.csv}"
if [[ -f "$ROOT/$TRADES" ]]; then
    echo "[INFO] Generating HTML report..."
    python3 "$ROOT/scripts/backtest_report.py" \
        --trades "$ROOT/$TRADES" --output "$ROOT/report.html"
    echo "[INFO] Report: $ROOT/report.html"
fi
