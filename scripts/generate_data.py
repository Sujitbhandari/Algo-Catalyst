#!/usr/bin/env python3
"""
Algo-Catalyst Synthetic Data Generator
Produces realistic intraday tick data with configurable market scenarios.

Scenarios:
  catalyst  – news-driven gap-up breakout (default)
  meanrev   – choppy, mean-reverting session
  trending  – slow steady trend without a catalyst
  volatile  – high-volatility session with whipsaw moves
"""

import argparse
import csv
import random
import math
from pathlib import Path


# ─── Base tick generator ──────────────────────────────────────────────────────

def _gbm_step(price: float, mu: float, sigma: float, dt: float = 1.0) -> float:
    """One Geometric Brownian Motion step."""
    z = random.gauss(0, 1)
    return price * math.exp((mu - 0.5 * sigma**2) * dt + sigma * math.sqrt(dt) * z)


def _volume_burst(base_vol: int, multiplier: float, noise: float = 0.3) -> int:
    vol = int(base_vol * multiplier * random.uniform(1 - noise, 1 + noise))
    return max(1, vol)


def _bid_ask(volume: int, pressure: float) -> tuple[int, int]:
    """Return (bid_size, ask_size) given order flow pressure in [-1, 1]."""
    total = volume * 5
    bid = int(total * (0.5 + pressure * 0.4))
    ask = total - bid
    return max(1, bid), max(1, ask)


# ─── Scenario generators ──────────────────────────────────────────────────────

def _phase_choppy(n: int, start_price: float, base_vol: int,
                  ts_start: int, ts_step: int) -> list[dict]:
    ticks = []
    price = start_price
    for i in range(n):
        price = max(price * 0.98, min(price * 1.02,
                    _gbm_step(price, mu=0.0, sigma=0.005)))
        vol   = _volume_burst(base_vol, multiplier=1.0, noise=0.4)
        bid, ask = _bid_ask(vol, pressure=random.uniform(-0.1, 0.1))
        ticks.append({
            "ts": ts_start + i * ts_step,
            "price": round(price, 4),
            "vol": vol,
            "bid": bid,
            "ask": ask,
            "high": round(price * random.uniform(1.0, 1.003), 4),
            "low":  round(price * random.uniform(0.997, 1.0), 4),
        })
    return ticks


def _phase_catalyst(n: int, start_price: float, base_vol: int,
                    ts_start: int, ts_step: int,
                    gap_pct: float = 0.12) -> list[dict]:
    ticks = []
    price = start_price * (1 + gap_pct)  # gap up
    ramp_ticks = n // 3

    for i in range(n):
        if i < ramp_ticks:
            # Strong uptrend, massive volume, high buying pressure
            mu = 0.04
            sigma = 0.015
            vol_mult = random.uniform(8, 14)
            pressure = random.uniform(0.3, 0.6)
        elif i < 2 * ramp_ticks:
            # Mid-trend: slowing but still bullish
            mu = 0.015
            sigma = 0.012
            vol_mult = random.uniform(4, 8)
            pressure = random.uniform(0.1, 0.4)
        else:
            # Late trend: consolidation with slight drift up
            mu = 0.005
            sigma = 0.010
            vol_mult = random.uniform(2, 5)
            pressure = random.uniform(-0.1, 0.25)

        price = _gbm_step(price, mu=mu, sigma=sigma)
        vol   = _volume_burst(base_vol, vol_mult)
        bid, ask = _bid_ask(vol, pressure=pressure)
        ticks.append({
            "ts": ts_start + i * ts_step,
            "price": round(price, 4),
            "vol": vol,
            "bid": bid,
            "ask": ask,
            "high": round(price * random.uniform(1.0, 1.005), 4),
            "low":  round(price * random.uniform(0.995, 1.0), 4),
        })
    return ticks


def _phase_trending(n: int, start_price: float, base_vol: int,
                    ts_start: int, ts_step: int, direction: float = 1.0) -> list[dict]:
    ticks = []
    price = start_price
    mu = 0.008 * direction
    for i in range(n):
        price = _gbm_step(price, mu=mu, sigma=0.008)
        vol   = _volume_burst(base_vol, multiplier=1.5, noise=0.3)
        bid, ask = _bid_ask(vol, pressure=0.15 * direction)
        ticks.append({
            "ts": ts_start + i * ts_step,
            "price": round(price, 4),
            "vol": vol,
            "bid": bid,
            "ask": ask,
            "high": round(price * random.uniform(1.0, 1.004), 4),
            "low":  round(price * random.uniform(0.996, 1.0), 4),
        })
    return ticks


def _phase_volatile(n: int, start_price: float, base_vol: int,
                    ts_start: int, ts_step: int) -> list[dict]:
    ticks = []
    price = start_price
    for i in range(n):
        price = _gbm_step(price, mu=0.0, sigma=0.025)
        vol   = _volume_burst(base_vol, multiplier=3.0, noise=0.6)
        bid, ask = _bid_ask(vol, pressure=random.uniform(-0.4, 0.4))
        ticks.append({
            "ts": ts_start + i * ts_step,
            "price": round(price, 4),
            "vol": vol,
            "bid": bid,
            "ask": ask,
            "high": round(price * random.uniform(1.0, 1.008), 4),
            "low":  round(price * random.uniform(0.992, 1.0), 4),
        })
    return ticks


# ─── Scenario builders ────────────────────────────────────────────────────────

def build_catalyst(n: int = 10_000, seed: int = 42) -> list[dict]:
    random.seed(seed)
    base_ts   = 1_609_459_200_000_000  # 2021-01-01 00:00:00 UTC (us)
    ts_step   = 1_000_000              # 1 second per tick
    base_vol  = 1_000
    base_price = 10.00

    choppy  = _phase_choppy  (n // 10,       base_price,  base_vol, base_ts,                          ts_step)
    cat     = _phase_catalyst(n * 4 // 10,   base_price,  base_vol, base_ts + len(choppy) * ts_step,  ts_step)
    trend   = _phase_trending(n - len(choppy) - len(cat), cat[-1]["price"], base_vol,
                               base_ts + (len(choppy) + len(cat)) * ts_step, ts_step)
    return choppy + cat + trend


def build_meanrev(n: int = 10_000, seed: int = 42) -> list[dict]:
    random.seed(seed)
    base_ts  = 1_609_459_200_000_000
    return _phase_choppy(n, 25.00, 2_000, base_ts, 500_000)


def build_trending(n: int = 10_000, seed: int = 42) -> list[dict]:
    random.seed(seed)
    base_ts = 1_609_459_200_000_000
    return _phase_trending(n, 50.00, 3_000, base_ts, 500_000, direction=1.0)


def build_volatile(n: int = 10_000, seed: int = 42) -> list[dict]:
    random.seed(seed)
    base_ts = 1_609_459_200_000_000
    return _phase_volatile(n, 20.00, 5_000, base_ts, 250_000)


SCENARIOS = {
    "catalyst": build_catalyst,
    "meanrev":  build_meanrev,
    "trending": build_trending,
    "volatile": build_volatile,
}


# ─── Writer ───────────────────────────────────────────────────────────────────

def write_csv(ticks: list[dict], output_path: str) -> None:
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["Timestamp", "Price", "Volume", "Bid_Size", "Ask_Size", "High", "Low"])
        for t in ticks:
            writer.writerow([t["ts"], t["price"], t["vol"], t["bid"], t["ask"], t["high"], t["low"]])

    prices = [t["price"] for t in ticks]
    vols   = [t["vol"]   for t in ticks]
    print(f"[OK] Wrote {len(ticks)} ticks → {output_path}")
    print(f"     Price range : ${min(prices):.4f} – ${max(prices):.4f}")
    print(f"     Volume range: {min(vols)} – {max(vols)}")


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Algo-Catalyst synthetic data generator")
    parser.add_argument("--scenario", choices=list(SCENARIOS), default="catalyst",
                        help="Market scenario to simulate (default: catalyst)")
    parser.add_argument("--ticks",  type=int, default=10_000,
                        help="Number of ticks to generate (default: 10000)")
    parser.add_argument("--seed",   type=int, default=42, help="RNG seed")
    parser.add_argument("--output", default=None,
                        help="Output CSV path (default: data/<scenario>.csv)")
    args = parser.parse_args()

    output = args.output or f"data/{args.scenario}.csv"
    print(f"Generating '{args.scenario}' scenario: {args.ticks} ticks (seed={args.seed})…")

    ticks = SCENARIOS[args.scenario](n=args.ticks, seed=args.seed)
    write_csv(ticks, output)

    # Also write as the canonical tick_data.csv so the engine picks it up by default
    if args.scenario == "catalyst" and args.output is None:
        write_csv(ticks, "data/synthetic_catalyst.csv")


if __name__ == "__main__":
    main()
