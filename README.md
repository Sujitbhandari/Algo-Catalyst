# Algo-Catalyst: High-Performance Event-Driven Trading Engine

An Event-Driven C++20 Trading Engine designed for high-volatility news strategies.

## Features

Event-Driven Architecture: Implements a Backtester class using a priority queue-based event loop.

News Momentum Algorithm filters trades using:
- 90/200 EMA trends for long-term bullish trend detection
- 9-EMA crossover above 90-EMA
- VWAP holds where price must be above Volume Weighted Average Price
- MACD confirmation with expanding histogram for momentum
- Level 2 Bid-Ask imbalances requiring greater than 1.5x buying pressure

AI Regime Detector uses custom k-Means clustering written from scratch to classify market regimes:
- Regime 0 (Choppy): Low volatility, mean-reverting. Disables trading.
- Regime 1 (Trending): High directed volatility. Enables trading with increased position size.

Latency Simulation: Fixed 200ms latency between signal generation and fill execution to simulate real-world execution delays.

All technical indicators (EMA, MACD, VWAP) and k-Means clustering are implemented manually with zero external dependencies.

## Building

Requirements: C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+) and CMake 3.20+

Build Instructions:

```bash
mkdir build
cd build
cmake ..
make
```

Run the executable:

```bash
./AlgoCatalyst [csv_file] [symbol]
```

Example:
```bash
./AlgoCatalyst ../data/tick_data.csv TICKER
```

## CSV Data Format

The CSV file must contain the following columns:
```
Timestamp,Price,Volume,Bid_Size,Ask_Size
```

- Timestamp: Unix timestamp in microseconds
- Price: Tick price
- Volume: Trade volume for this tick
- Bid_Size: Total bid size at best bid
- Ask_Size: Total ask size at best ask

Example:
```csv
Timestamp,Price,Volume,Bid_Size,Ask_Size
1609459200000000,100.00,1000,5000,3000
1609459201000000,101.50,1500,6000,3500
```

## Strategy Logic

Entry Conditions (all must be true):

1. Volume Spike: Relative volume greater than 5x average using 20-tick rolling average
2. Gap Up: Price gap greater than 10% from previous close
3. EMA Trend: Price above both 90-EMA and 200-EMA, with 90-EMA greater than 200-EMA
4. EMA Crossover: 9-EMA crosses above 90-EMA
5. VWAP Hold: Price above VWAP
6. MACD Expansion: MACD histogram is expanding indicating momentum
7. Order Book Imbalance: Bid Size greater than 1.5x Ask Size
8. Regime: Must be in TRENDING regime as classified by k-Means

Exit Conditions (any condition triggers exit):

- Price drops below VWAP
- MACD histogram contracts or turns negative
- Regime switches to CHOPPY

## Performance

The engine is designed to process over 1 million ticks efficiently using a priority queue for O(log n) event insertion, in-place indicator calculations, and minimal memory allocation during runtime.

## Technical Implementation Notes

EMA Calculation is manually implemented using alpha = 2/(period+1). MACD uses 12-EMA minus 26-EMA with a 9-EMA signal line. VWAP is calculated as cumulative (Price × Volume) divided by cumulative Volume. The k-Means implementation uses Euclidean distance with up to 10 iterations. Latency simulation timestamps events with a latency offset to model realistic execution delays.

## Example Output

```
Algo-Catalyst: High-Performance Trading Engine v1.0
Event-Driven Backtesting for News Catalyst Strategies

Loading tick data from: data/tick_data.csv
Processed 1000000 events...

TRADE LOG
Symbol          Entry Time          Exit Time            Entry Price  Exit Price  Quantity  PnL         Regime
----------------------------------------------------------------------------------------------------
TICKER          1609459203000000    1609459212000000     103.00       114.50       150.00    1725.00     TRENDING

Total Trades: 1
Total PnL: $1725.00
```

## Visualization

After running the backtest, you can visualize the strategy performance using the included Python script.

Requirements:
- Python 3.7+
- pandas
- matplotlib
- numpy

Setup (first time only):
```bash
# Create virtual environment (if not already created)
python3 -m venv venv

# Activate virtual environment
source venv/bin/activate

# Install Python dependencies
python3 -m pip install pandas matplotlib numpy
```

Run the backtest to generate trades.csv:
```bash
./AlgoCatalyst ../data/tick_data.csv TICKER
```

Then generate the visualization:
```bash
# Make sure virtual environment is activated
source venv/bin/activate

# Run the visualization script
python3 scripts/plot_performance.py
```

Important: Run the visualization from the project root directory (Algo-Catalyst), not from the build directory. 

If you're in the build directory, go back to the project root first:
```bash
cd ..  # Go back to project root
source venv/bin/activate
python3 scripts/plot_performance.py
```

Note: On macOS with Homebrew-managed Python, use a virtual environment. The venv folder is already set up with required packages.

The script generates two plots:
1. Price Action: Shows price movement with buy/sell markers and regime background coloring (green for TRENDING, red for CHOPPY)
2. Equity Curve: Displays cumulative profit/loss over time

The plot is saved as performance_plot.png and displayed interactively.


