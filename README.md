# Algo-Catalyst

[![CI](https://github.com/sujitbhandari/Algo-Catalyst/actions/workflows/ci.yml/badge.svg)](https://github.com/sujitbhandari/Algo-Catalyst/actions)

A C++20 event-driven backtesting engine for news catalyst and mean-reversion strategies. No external C++ libraries — all indicators, k-means clustering, and execution logic are written from scratch.

---

## Building

Requirements: C++20 compiler (GCC 10+, Clang 12+), CMake 3.20+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Or use the Makefile:

```bash
make release
```

---

## Quick start

The fastest way is the shell script — it builds, generates data, runs the backtest, and opens a chart:

```bash
./visualize.sh --scenario catalyst --ticks 10000 --open
```

Or step by step:

```bash
# Generate tick data
python3 scripts/generate_data.py --scenario catalyst --ticks 10000

# Run the backtest
./build/AlgoCatalyst --data data/catalyst.csv --symbol AAPL --output trades.csv

# Plot results
python3 scripts/plot_performance.py --trades trades.csv
```

---

## CLI options

```
--config <path>      JSON config file (values overridden by any CLI flag that follows)
--data <path>        path to tick CSV file (default: data/tick_data.csv)
--symbol <sym>       ticker symbol (default: TICKER)
--strategy <name>    momentum | meanrev | breakout (default: momentum)
--latency <ms>       fill latency in ms (default: 200)
--output <path>      trade log CSV path (default: trades.csv)
--json-output <path> also write trades as JSON (optional)
--stop-loss <pct>    hard stop loss % (default: 2.0)
--take-profit <pct>  take profit % (default: 6.0)
--trailing <pct>     trailing stop % (default: 3.0)
--slippage <bps>     slippage in basis points (default: 5)
--help               show this message
```

---

## Config file

Instead of typing CLI flags every time, drop a `config.json` at the project root:

```json
{
    "data": "data/tick_data.csv",
    "symbol": "AAPL",
    "strategy": "momentum",
    "stop_loss_pct": 2.0,
    "take_profit_pct": 6.0
}
```

Then run: `./build/AlgoCatalyst --config config.json`

---

## Strategies

**Momentum (news catalyst)** looks for a large gap-up with a volume spike, then filters on EMA alignment, MACD, VWAP, and order-book imbalance. Only enters in a trending regime. Position size uses fractional Kelly + ATR volatility scaling + regime multiplier. Exits on stop-loss, trailing stop, take-profit, VWAP break, or regime change.

**Mean reversion** targets choppy, low-volatility conditions. Enters when RSI drops below 30 and price falls under the lower Bollinger Band. Exits when price returns above the middle band.

**Breakout** fires on a Donchian Channel upper breakout confirmed by CCI > 100 and a volume spike above the threshold. Uses a trailing stop and take-profit exit. Avoids volatile regimes.

---

## Indicators

All implemented manually with no external libraries:

- EMA (with SMA warm-up), WMA, DEMA
- MACD (12/26/9)
- RSI (Wilder smoothing)
- Bollinger Bands
- ATR
- VWAP (resets each session automatically)
- Stochastic Oscillator %K/%D
- OBV (On-Balance Volume)
- Williams %R
- Donchian Channel
- CCI (Commodity Channel Index)
- CMF (Chaikin Money Flow)

---

## Regime detection

Uses a k-means classifier on recent volatility, directional movement, and volume to label the market as one of three states:

- **Trending** — high volatility with clear direction; position size +50%
- **Volatile** — high volatility but no direction; position size halved
- **Choppy** — low volatility; momentum entries disabled

---

## Performance metrics

The `PerformanceAnalyzer` computes after every run:

Sharpe, Sortino, Calmar, profit factor, win rate, expectancy, max drawdown, VaR 95%/99%, CVaR 95%, recovery factor, MAE/MFE per trade, consecutive win/loss streaks, and average hold time.

---

## Data generator

```bash
python3 scripts/generate_data.py --scenario catalyst  # news breakout
python3 scripts/generate_data.py --scenario meanrev   # choppy session
python3 scripts/generate_data.py --scenario trending  # slow drift
python3 scripts/generate_data.py --scenario volatile  # whipsaw
```

---

## Analysis tools

```bash
# Parameter grid search
python3 scripts/optimize_params.py --scenario catalyst --metric sharpe

# Parameter sensitivity heatmap (reads optimization_results.csv)
python3 scripts/heatmap.py --metric sharpe

# Walk-forward out-of-sample validation
python3 scripts/walk_forward.py --scenario catalyst --windows 5

# Side-by-side comparison across all scenarios
python3 scripts/compare_strategies.py --ticks 10000

# Export metrics to JSON
python3 scripts/export_metrics.py --trades trades.csv --output metrics.json
```

---

## Tests

```bash
# Build and run unit tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target AlgoCatalystTests
./build/AlgoCatalystTests

# Or via ctest
ctest --test-dir build
```

---

## Docker

```bash
docker build -t algo-catalyst .
docker run --rm algo-catalyst --help
```

---

## CSV format

```
Timestamp,Price,Volume,Bid_Size,Ask_Size
```

Timestamp can be Unix microseconds or ISO 8601. High and Low columns are optional (used by ATR if present).

---

## License

MIT
