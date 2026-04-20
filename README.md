# Algo-Catalyst

**High-Performance Event-Driven Trading Backtesting Engine — C++20**

[![CI](https://github.com/sujitbhandari/Algo-Catalyst/actions/workflows/ci.yml/badge.svg)](https://github.com/sujitbhandari/Algo-Catalyst/actions)

A zero-dependency C++20 backtesting engine for high-volatility **news catalyst** and **mean-reversion** strategies. All indicators, k-Means clustering, and execution simulation are implemented from scratch.

---

## Architecture

```
CSV Tick Data
     │
     ▼
 TickLoader ──► priority_queue<Event>
                       │
          ┌────────────▼────────────┐
          │      Backtester         │
          │  (event-driven loop)    │
          └──┬──────────┬───────────┘
             │          │
   MarketUpdate      FillEvent
        │                │
        ▼                ▼
   Strategy         Position tracker
 (Indicators,       (PnL, drawdown,
  RegimeClassifier)  circuit breakers)
        │
    SignalEvent → OrderEvent → FillEvent (+ latency + slippage)
                                  │
                             trades.csv
                                  │
                          plot_performance.py
```

---

## Features

### Event-Driven Engine
- **Priority-queue** event loop with O(log n) insertion
- **Latency simulation**: configurable fill delay (default 200 ms)
- **Slippage model**: directional basis-point slippage on fills
- **Commission model**: per-share + minimum commission
- **Risk circuit breakers**: max drawdown & daily loss limits

### Indicators (all hand-rolled, zero dependencies)
| Indicator | Details |
|-----------|---------|
| EMA | SMA warm-up for first `period` ticks, then exponential smoothing |
| MACD | 12/26 EMA lines + 9-period EMA signal with histogram |
| RSI | Wilder smoothing, configurable period |
| Bollinger Bands | SMA ± N × σ with bandwidth metric |
| ATR | Wilder smoothing over True Range |
| VWAP | Session-based with automatic midnight reset |

### Market Regime Classifier
k-Means clustering (written from scratch) over volatility, directional strength, and relative volume:

| Regime | Condition | Position Multiplier |
|--------|-----------|---------------------|
| **TRENDING** | High vol + strong direction | 1.5× |
| **VOLATILE** | High vol + weak direction | 0.5× |
| **CHOPPY** | Low volatility | 0× (disabled) |

### Strategies
**NewsMomentumStrategy** — all conditions must align:
1. Volume spike > 5× 20-tick average
2. Gap up > 10% from session open
3. Price above 90-EMA and 200-EMA (bullish stack)
4. 9-EMA crossover above 90-EMA
5. Price above VWAP
6. MACD histogram expanding
7. Bid/Ask ratio > 1.5×
8. Regime = **TRENDING**

Exits: stop-loss · trailing stop · take-profit · VWAP break · regime change

**MeanReversionStrategy** — for CHOPPY regimes:
- Enter when RSI < 30 AND price below lower Bollinger Band
- Exit when RSI recovers OR price crosses above middle band

### Position Sizing
- Fractional **Kelly criterion** (half-Kelly) computed from live win rate & avg win/loss
- Regime multiplier applied on top

---

## Quick Start

### One-command pipeline

```bash
./visualize.sh --scenario catalyst --ticks 10000 --open
```

This builds the engine, generates synthetic data, runs the backtest, and opens the chart.

### Manual steps

```bash
# 1. Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 2. Generate synthetic data
python3 scripts/generate_data.py --scenario catalyst --ticks 10000

# 3. Run backtest
./build/AlgoCatalyst \
    --data data/catalyst.csv \
    --symbol AAPL \
    --latency 200 \
    --stop-loss 2.0 \
    --take-profit 6.0 \
    --trailing 3.0 \
    --output trades.csv

# 4. Visualize
python3 scripts/plot_performance.py --trades trades.csv --ticks data/catalyst.csv
```

---

## CLI Reference

```
./AlgoCatalyst [OPTIONS]

  --data <path>        Tick CSV file           (default: data/tick_data.csv)
  --symbol <sym>       Ticker symbol           (default: TICKER)
  --latency <ms>       Fill latency in ms      (default: 200)
  --output <path>      Trade log CSV path      (default: trades.csv)
  --stop-loss <pct>    Hard stop-loss %        (default: 2.0)
  --take-profit <pct>  Take-profit %           (default: 6.0)
  --trailing <pct>     Trailing stop %         (default: 3.0)
  --slippage <bps>     Slippage basis points   (default: 5)
  --help               Show this message
```

---

## CSV Data Format

```csv
Timestamp,Price,Volume,Bid_Size,Ask_Size[,High,Low]
```

- `Timestamp`: Unix microseconds **or** ISO 8601 (`2024-01-15T09:30:00.123456`)
- `High` / `Low` columns are optional (used by ATR)

---

## Data Generator

Four built-in market scenarios:

| Scenario | Description |
|----------|-------------|
| `catalyst` | News-driven gap-up → strong trend |
| `meanrev` | Choppy, oscillating around a mean |
| `trending` | Slow steady drift, no catalyst |
| `volatile` | High-sigma random walk, no direction |

```bash
python3 scripts/generate_data.py --scenario catalyst --ticks 20000 --seed 99
```

---

## Performance Metrics

The engine prints after each backtest:

```
========== PERFORMANCE SUMMARY ==========
Total Trades:    42
Total PnL:       $3,847.12
Win Rate:        64.29%
Profit Factor:   2.31
Avg Win:         $198.45
Avg Loss:        $-86.20
Max Drawdown:    $412.00
Sharpe Ratio:    1.847
=========================================
```

---

## Building

**Requirements**: C++20 (GCC 10+, Clang 12+, MSVC 2022), CMake 3.20+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --config Release
```

Debug build with sanitizers:
```bash
cmake -B build-dbg -DCMAKE_BUILD_TYPE=Debug
cmake --build build-dbg
```

---

## Python Visualization

```bash
pip install -r requirements.txt
python3 scripts/plot_performance.py --trades trades.csv --output chart.png
```

The chart includes:
- Price action with trade markers & regime shading
- Equity curve with profit/loss fill
- Drawdown panel
- PnL distribution histogram
- Summary statistics overlay

---

## Project Structure

```
Algo-Catalyst/
├── include/
│   ├── Events.h          # Tick, Event hierarchy
│   ├── Engine.h          # Backtester, TickLoader
│   ├── Indicators.h      # EMA, MACD, RSI, BB, ATR, VWAP
│   ├── Strategy.h        # Strategy interface + NewsMomentum + MeanReversion
│   ├── AI_Regime.h       # k-Means RegimeClassifier
│   └── Version.h         # Compile-time version constants
├── src/
│   ├── main.cpp          # CLI entry point
│   ├── Engine.cpp
│   ├── Indicators.cpp
│   ├── Strategy.cpp
│   └── AI_Regime.cpp
├── scripts/
│   ├── generate_data.py  # Synthetic market data generator
│   └── plot_performance.py
├── data/
│   └── tick_data.csv
├── .github/workflows/ci.yml
├── CMakeLists.txt
├── requirements.txt
├── .clang-format
└── visualize.sh
```

---

## License

MIT
