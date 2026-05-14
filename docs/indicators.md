# Indicators

All indicators are implemented from scratch in `src/Indicators.cpp` with no external C++ libraries.

---

## EMA — Exponential Moving Average

Uses SMA warm-up for the first `period` ticks, then switches to:

```
EMA(t) = alpha * price(t) + (1 - alpha) * EMA(t-1)
alpha  = 2 / (period + 1)
```

Multiple periods share the same `Indicators` instance via a `std::map<size_t, EMAState>`.

---

## WMA — Weighted Moving Average

Assigns linearly increasing weight to recent prices:

```
WMA = sum(weight_i * price_i) / sum(weight_i)
weight_i = i + 1  (oldest = 1, newest = period)
```

---

## DEMA — Double Exponential Moving Average

Applies EMA twice and subtracts the lag:

```
DEMA = 2 * EMA1 - EMA2
```

Faster response to price changes than a single EMA of the same period.

---

## TRIX — Triple Exponential Average Rate of Change

Applies EMA three times then reports percentage change:

```
TRIX = (EMA3(t) - EMA3(t-1)) / EMA3(t-1) * 100
```

Filters out short-cycle noise. Crossing zero is used as a signal.

---

## MACD — Moving Average Convergence Divergence

Classic 12/26/9 configuration:

```
MACD Line    = EMA(12) - EMA(26)
Signal Line  = EMA(9) of MACD Line
Histogram    = MACD Line - Signal Line
```

---

## RSI — Relative Strength Index

Wilder smoothing (equivalent to an EMA with alpha = 1/period):

```
RS  = avg_gain / avg_loss
RSI = 100 - 100 / (1 + RS)
```

Default period: 14. Oversold < 30, overbought > 70.

---

## Bollinger Bands

Rolling mean ± N standard deviations:

```
Middle = SMA(period)
Upper  = Middle + N * stddev
Lower  = Middle - N * stddev
```

Default: period = 20, N = 2.

---

## ATR — Average True Range

True range and Wilder smoothing:

```
TR  = max(H-L, |H - prev_C|, |L - prev_C|)
ATR = Wilder_EMA(TR, period)
```

Default period: 14.

---

## ADX — Average Directional Index

Measures trend strength 0–100 regardless of direction:

```
+DM  = max(High - prev_High, 0) if > |prev_Low - Low| else 0
-DM  = max(prev_Low - Low, 0)   if > |High - prev_High| else 0
+DI  = 100 * Smooth(+DM) / Smooth(TR)
-DI  = 100 * Smooth(-DM) / Smooth(TR)
DX   = 100 * |+DI - -DI| / (+DI + -DI)
ADX  = Wilder_EMA(DX, period)
```

ADX > 25 is considered trending.

---

## Stochastic Oscillator %K / %D

```
%K = 100 * (Close - Lowest_Low) / (Highest_High - Lowest_Low)
%D = SMA(%K, d_period)
```

Default: k_period = 14, d_period = 3.

---

## Williams %R

```
%R = -100 * (Highest_High - Close) / (Highest_High - Lowest_Low)
```

Range: [-100, 0]. Oversold below -80, overbought above -20.

---

## Donchian Channel

```
Upper = max(High, N periods)
Lower = min(Low, N periods)
Mid   = (Upper + Lower) / 2
```

Default period: 20. Used by BreakoutStrategy.

---

## VWAP — Volume Weighted Average Price

Resets at the start of each new trading day:

```
VWAP = sum(Price * Volume) / sum(Volume)
```

---

## OBV — On-Balance Volume

```
if Close > prev_Close: OBV += Volume
if Close < prev_Close: OBV -= Volume
```

Rising OBV with rising price confirms the trend.

---

## MFI — Money Flow Index

Volume-weighted RSI:

```
Typical Price = (H + L + C) / 3
Raw Money Flow = TP * Volume
MFI = 100 - 100 / (1 + Positive_MF / Negative_MF)
```

Oversold < 20, overbought > 80.

---

## CCI — Commodity Channel Index

```
TP   = (H + L + C) / 3
Mean = SMA(TP, period)
MeanDev = SMA(|TP - Mean|, period)
CCI  = (TP - Mean) / (0.015 * MeanDev)
```

Overbought > +100, oversold < -100.

---

## CMF — Chaikin Money Flow

```
MFM = ((C - L) - (H - C)) / (H - L)
MFV = MFM * Volume
CMF = sum(MFV, period) / sum(Volume, period)
```

Positive CMF indicates buying pressure.

---

## KAMA — Kaufman Adaptive Moving Average

Adjusts smoothing constant based on the Efficiency Ratio:

```
ER  = |direction| / volatility
SC  = (ER * (fast_sc - slow_sc) + slow_sc)^2
KAMA = KAMA + SC * (Price - KAMA)
```

Moves quickly in trending markets and slowly in choppy ones.

---

## Pivot Points (Classic floor trader)

```
PP = (prev_H + prev_L + prev_C) / 3
R1 = 2 * PP - prev_L
R2 = PP + (prev_H - prev_L)
R3 = prev_H + 2 * (PP - prev_L)
S1 = 2 * PP - prev_H
S2 = PP - (prev_H - prev_L)
S3 = prev_L - 2 * (prev_H - PP)
```
