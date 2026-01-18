#!/bin/bash

# Quick script to visualize backtest results
# Run from project root, not from build directory

# Get script directory (project root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Check for trades.csv in current directory or build directory
TRADES_FILE="trades.csv"
if [ ! -f "$TRADES_FILE" ] && [ -f "build/trades.csv" ]; then
    TRADES_FILE="build/trades.csv"
    echo "Found trades.csv in build directory"
fi

if [ ! -f "$TRADES_FILE" ]; then
    echo "Error: trades.csv not found."
    echo "Please run the backtest first: cd build && ./AlgoCatalyst ../data/tick_data.csv TICKER"
    exit 1
fi

# Copy trades.csv to project root if it's in build directory
if [ "$TRADES_FILE" == "build/trades.csv" ]; then
    cp build/trades.csv trades.csv
fi

# Setup virtual environment if needed
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
    python3 -m pip install pandas matplotlib numpy
else
    source venv/bin/activate
fi

python3 scripts/plot_performance.py

