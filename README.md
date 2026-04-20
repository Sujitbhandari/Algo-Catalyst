# Algo-Catalyst

[![CI](https://github.com/sujitbhandari/Algo-Catalyst/actions/workflows/ci.yml/badge.svg)](https://github.com/sujitbhandari/Algo-Catalyst/actions)

A C++20 backtesting engine for news catalyst and mean-reversion strategies. No external libraries — all indicators, k-means clustering, and execution logic are written from scratch.

---

## Building

Requirements: C++20 compiler (GCC 10+, Clang 12+) and CMake 3.20+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Running a backtest

The quickest way is to use the provided shell script, which handles everything — builds, generates data, runs the backtest, and produces a chart:

```bash
./visualize.sh --scenario catalyst --ticks 10000 --open
```

Or manually:

```bash
# Generate some synthetic tick data first
python3 scripts/generate_data.py --scenario catalyst --ticks 10000

# Run the backtest
./build/AlgoCatalyst --data data/catalyst.csv --symbol AAPL --output trades.csv

# Plot the results
python3 scripts/plot_performance.py --trades trades.csv
```

---

## Options

```
--data <path>        path to tick CSV file  (default: data/tick_data.csv)
--symbol <sym>       ticker symbol          (default: TICKER)
--latency <ms>       fill latency in ms     (default: 200)
--output <path>      output CSV path        (default: trades.csv)
--stop-loss <pct>    stop loss %            (default: 2.0)
--take-profit <pct>  take profit %          (default: 6.0)
--trailing <pct>     trailing stop %        (default: 3.0)
--slippage <bps>     slippage in bps        (default: 5)
--help               show this message
```

---

## How the strategies work

There are two strategies included.

The news momentum strategy looks for stocks with a big gap up and a volume spike, then checks that the trend is confirmed across a few filters — price above VWAP, EMA alignment, MACD expanding, and a bullish order book. It only enters in a trending regime as classified by the k-means detector. Exits happen on a stop-loss, trailing stop, take-profit, VWAP break, or when the regime turns choppy or volatile.

The mean reversion strategy does the opposite — it targets choppy, low-volatility conditions. It enters when RSI dips below 30 and price falls below the lower Bollinger Band, then exits when the price recovers back above the middle band.

---

## Indicators

All implemented manually with no external libraries:

- EMA (with SMA warm-up for the first N ticks)
- MACD (12/26 EMA lines + 9-period signal)
- RSI (Wilder smoothing)
- Bollinger Bands
- ATR
- VWAP (resets each trading day automatically)
- Stochastic Oscillator

---

## Regime detection

The engine uses a k-means classifier that looks at recent volatility, directional movement, and volume to classify the current market into one of three states:

- Trending — high volatility with a clear direction. Position size is increased by 50%.
- Volatile — high volatility but no clear direction. Position size is cut in half.
- Choppy — low volatility. Momentum trading is disabled.

---

## Data generator

There are four built-in scenarios you can generate data for:

```bash
python3 scripts/generate_data.py --scenario catalyst   # news breakout
python3 scripts/generate_data.py --scenario meanrev    # choppy session
python3 scripts/generate_data.py --scenario trending   # slow drift
python3 scripts/generate_data.py --scenario volatile   # whipsaw
```

---

## CSV format

```
Timestamp,Price,Volume,Bid_Size,Ask_Size
```

Timestamp can be Unix microseconds or ISO 8601 format. High and Low columns are optional and used by ATR if present.

---

## Visualization

```bash
pip install -r requirements.txt
python3 scripts/plot_performance.py --trades trades.csv
```

This produces a dark-themed chart with price action, trade markers, equity curve, drawdown, and a PnL distribution.

---

## License

MIT
