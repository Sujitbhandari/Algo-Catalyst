# Strategies

Three strategies ship with Algo-Catalyst. Select one with `--strategy <name>` or via `config.json`.

---

## NewsMomentumStrategy (`momentum`)

Designed for stocks with a large gap-up on elevated volume — typical news catalyst situations.

### Entry conditions (all must be true)

| Condition | Detail |
|-----------|--------|
| Volume spike | Relative volume ≥ `min_relative_volume` (default 5×) |
| Gap up | Price above open by ≥ `min_gap_up_pct` (default 10%) |
| EMA trend | Price above both 90-EMA and 200-EMA, and 90-EMA > 200-EMA |
| EMA crossover | 9-EMA above or crossing above 90-EMA |
| VWAP | Price above VWAP |
| MACD | Histogram expanding (positive and growing) |
| Order book | Bid/Ask size ratio ≥ `min_bid_ask_ratio` (default 1.5) |
| Regime | Must be `TRENDING` (k-means classifier) |
| Market hours | Within configured NYSE session window (default 09:30–16:00 ET) |

### Position sizing

Position size = `base_position_size × regime_multiplier × kelly_fraction × atr_volatility_scale`

- **Regime multiplier**: 1.5× in TRENDING, 0.5× in VOLATILE, 0.0 in CHOPPY
- **Kelly fraction**: half-Kelly based on rolling win rate and win/loss ratio (kicks in after 10 trades)
- **ATR scale**: `clamp(2% / ATR%, 0.5, 1.5)` — smaller when volatile, larger when quiet

### Exit conditions (first to trigger wins)

- Hard stop-loss: price drops `stop_loss_pct` below entry
- Trailing stop: price pulls back `trailing_stop_pct` from the highest-since-entry
- Take-profit: price rises `take_profit_pct` above entry
- VWAP break: price drops below VWAP
- MACD contraction: histogram negative and contracting
- Regime change: regime turns CHOPPY or VOLATILE

---

## MeanReversionStrategy (`meanrev`)

Targets choppy, range-bound sessions where price tends to revert to the mean.

### Entry conditions

| Condition | Detail |
|-----------|--------|
| RSI oversold | RSI(14) < `oversold_threshold` (default 30) |
| Bollinger Band | Price below lower band |
| **OR** RSI divergence | Price makes lower low while RSI makes higher low, price below middle band |
| Regime | Must be `CHOPPY` |

### Exit conditions

- Hard stop-loss: price drops `stop_loss_pct` from entry
- Take-profit: price rises `take_profit_pct` from entry
- Mean reversion achieved: RSI recovers above oversold threshold AND price above middle Bollinger Band

---

## BreakoutStrategy (`breakout`)

Trades range expansions confirmed by multiple filters to avoid false breakouts.

### Entry conditions

| Condition | Detail |
|-----------|--------|
| Donchian upper breakout | Price > highest high of last `donchian_period` (default 20) bars |
| CCI confirmation | CCI > 100 (strong buying pressure) |
| Volume confirmation | Relative volume ≥ `min_relative_volume` (default 1.5×) |
| VWAP | Price above VWAP |
| Regime | TRENDING or CHOPPY (not VOLATILE) |

### Exit conditions

- Hard stop-loss: price drops `stop_loss_pct` from entry
- Take-profit: price rises `take_profit_pct` from entry
- Trailing stop: price pulls back `trailing_stop_pct` from high since entry
- Regime turns VOLATILE

---

## Regime classifier

All strategies share a k-means classifier that updates on every tick:

| Regime | Characteristics | Effect |
|--------|----------------|--------|
| TRENDING | High volatility + clear direction | Momentum entries allowed; position size ×1.5 |
| VOLATILE | High volatility + no direction | All entries blocked; position size ×0.5 |
| CHOPPY | Low volatility | Only mean reversion allowed; momentum blocked |

Features used for clustering: rolling volatility (std dev of returns), directional strength, and normalised volume.
