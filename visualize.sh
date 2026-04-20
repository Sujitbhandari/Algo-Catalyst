#!/usr/bin/env bash
# visualize.sh — build → run → plot pipeline for Algo-Catalyst
# Usage: ./visualize.sh [--scenario <name>] [--ticks <n>] [--skip-build] [--open]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ─── Defaults ─────────────────────────────────────────────────────────────────
SCENARIO="catalyst"
TICKS=10000
SKIP_BUILD=false
OPEN_PLOT=false
BUILD_DIR="build"
TRADES_CSV="trades.csv"
TICK_CSV="data/${SCENARIO}.csv"

# ─── Argument parsing ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --scenario) SCENARIO="$2"; TICK_CSV="data/${SCENARIO}.csv"; shift 2 ;;
        --ticks)    TICKS="$2";    shift 2 ;;
        --skip-build) SKIP_BUILD=true; shift ;;
        --open)     OPEN_PLOT=true;  shift ;;
        --help|-h)
            echo "Usage: $0 [--scenario catalyst|meanrev|trending|volatile]"
            echo "          [--ticks N] [--skip-build] [--open]"
            exit 0 ;;
        *) echo "[WARN] Unknown argument: $1"; shift ;;
    esac
done

# ─── Python venv ──────────────────────────────────────────────────────────────
VENV_DIR=".venv"
if [[ ! -d "$VENV_DIR" ]]; then
    echo "[INFO] Creating Python virtual environment in $VENV_DIR…"
    python3 -m venv "$VENV_DIR"
fi
# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

echo "[INFO] Installing / upgrading Python dependencies…"
pip install --quiet --upgrade pip
pip install --quiet -r requirements.txt

# ─── C++ build ────────────────────────────────────────────────────────────────
if [[ "$SKIP_BUILD" == false ]]; then
    echo "[INFO] Configuring CMake (Release)…"
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" > /dev/null 2>&1

    echo "[INFO] Building AlgoCatalyst…"
    cmake --build "$BUILD_DIR" --config Release -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
fi

BINARY="${BUILD_DIR}/AlgoCatalyst"
if [[ ! -x "$BINARY" ]]; then
    echo "[ERROR] Binary not found: $BINARY — run without --skip-build"
    exit 1
fi

# ─── Generate synthetic data ──────────────────────────────────────────────────
echo "[INFO] Generating ${TICKS} ticks (scenario: ${SCENARIO})…"
python3 scripts/generate_data.py --scenario "$SCENARIO" --ticks "$TICKS"

# ─── Run backtest ─────────────────────────────────────────────────────────────
echo "[INFO] Running backtest…"
"$BINARY" \
    --data    "$TICK_CSV" \
    --symbol  "SYM" \
    --output  "$TRADES_CSV" \
    --latency 200

# ─── Plot ─────────────────────────────────────────────────────────────────────
echo "[INFO] Generating performance chart…"
python3 scripts/plot_performance.py \
    --trades  "$TRADES_CSV" \
    --ticks   "$TICK_CSV" \
    --output  "performance_plot.png"

if [[ "$OPEN_PLOT" == true ]]; then
    if command -v open &>/dev/null; then
        open performance_plot.png
    elif command -v xdg-open &>/dev/null; then
        xdg-open performance_plot.png
    fi
fi

echo ""
echo "Done! Chart saved to performance_plot.png"
