#!/usr/bin/env python3
"""
Algo-Catalyst Performance Visualizer
Generates a multi-panel chart: price action, equity curve, drawdown, and trade stats.
"""

import argparse
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # non-interactive backend for CI/headless environments

import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np
import pandas as pd


# ─── Data loaders ────────────────────────────────────────────────────────────

def load_trades(csv_path: str) -> pd.DataFrame:
    path = Path(csv_path)
    if not path.exists():
        print(f"[ERROR] Trade log not found: {csv_path}\nRun the backtest first.", file=sys.stderr)
        sys.exit(1)
    df = pd.read_csv(csv_path)
    for col in ("Entry_Time_US", "Exit_Time_US"):
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    return df


def load_tick_data(csv_path: str) -> pd.DataFrame | None:
    path = Path(csv_path)
    if not path.exists():
        print(f"[WARN] Tick data not found: {csv_path} — price panel will be omitted.")
        return None
    df = pd.read_csv(csv_path)
    df["Timestamp"] = pd.to_numeric(df["Timestamp"], errors="coerce")
    df = df.sort_values("Timestamp").reset_index(drop=True)
    return df


# ─── Stats helpers ────────────────────────────────────────────────────────────

def compute_stats(trades: pd.DataFrame) -> dict:
    pnls = trades["PnL"].values
    wins = pnls[pnls > 0]
    losses = pnls[pnls < 0]

    equity = np.cumsum(pnls)
    running_max = np.maximum.accumulate(equity)
    drawdown = running_max - equity
    max_dd = drawdown.max() if len(drawdown) else 0.0

    sharpe = 0.0
    if len(pnls) > 1 and pnls.std() > 0:
        sharpe = pnls.mean() / pnls.std()

    return {
        "total_pnl": float(pnls.sum()),
        "num_trades": len(pnls),
        "win_rate": 100.0 * len(wins) / len(pnls) if len(pnls) else 0.0,
        "avg_win": float(wins.mean()) if len(wins) else 0.0,
        "avg_loss": float(losses.mean()) if len(losses) else 0.0,
        "profit_factor": abs(wins.sum() / losses.sum()) if losses.sum() != 0 else float("inf"),
        "max_drawdown": float(max_dd),
        "sharpe": float(sharpe),
        "equity": equity,
        "drawdown": drawdown,
    }


# ─── Plot ────────────────────────────────────────────────────────────────────

def plot_performance(trades_df: pd.DataFrame, ticks_df: pd.DataFrame | None = None,
                     output_path: str = "performance_plot.png") -> None:
    stats = compute_stats(trades_df)

    fig = plt.figure(figsize=(16, 14), facecolor="#0d1117")
    fig.suptitle("Algo-Catalyst — Backtest Performance Report",
                 fontsize=16, fontweight="bold", color="white", y=0.99)

    gs = gridspec.GridSpec(3, 2, figure=fig, hspace=0.45, wspace=0.35)

    ax_price  = fig.add_subplot(gs[0, :])   # full-width price panel
    ax_equity = fig.add_subplot(gs[1, :])   # full-width equity panel
    ax_dd     = fig.add_subplot(gs[2, 0])   # drawdown
    ax_dist   = fig.add_subplot(gs[2, 1])   # PnL distribution

    dark_style = {
        "facecolor": "#161b22",
        "label.color": "white",
        "title.color": "white",
    }
    for ax in (ax_price, ax_equity, ax_dd, ax_dist):
        ax.set_facecolor("#161b22")
        ax.tick_params(colors="white")
        for spine in ax.spines.values():
            spine.set_edgecolor("#30363d")

    # ── Price panel ──────────────────────────────────────────────────────────
    if ticks_df is not None and len(ticks_df) > 0:
        ts = ticks_df["Timestamp"].values
        px = ticks_df["Price"].values
        ax_price.plot(ts, px, color="#58a6ff", linewidth=0.7, alpha=0.9, label="Price")

    time_col_entry = "Entry_Time_US" if "Entry_Time_US" in trades_df.columns else "Entry_Time"
    time_col_exit  = "Exit_Time_US"  if "Exit_Time_US"  in trades_df.columns else "Exit_Time"

    label_buy_done = label_sell_done = False
    for _, trade in trades_df.iterrows():
        et, xt = trade[time_col_entry], trade[time_col_exit]
        ep, xp = trade["Entry_Price"], trade["Exit_Price"]
        regime = trade.get("Regime", "UNKNOWN")
        span_color = "#238636" if regime == "TRENDING" else "#da3633"

        ax_price.axvspan(et, xt, alpha=0.12, color=span_color)
        kw_buy  = {"label": "Buy"}  if not label_buy_done  else {}
        kw_sell = {"label": "Sell"} if not label_sell_done else {}
        ax_price.scatter(et, ep, color="#2ea043", marker="^", s=80, zorder=5, **kw_buy)
        ax_price.scatter(xt, xp, color="#da3633", marker="v", s=80, zorder=5, **kw_sell)
        ax_price.plot([et, xt], [ep, xp], color="white", linewidth=0.6, alpha=0.25)
        label_buy_done = label_sell_done = True

    ax_price.set_title("Price Action with Trade Entries / Exits", color="white", fontsize=13)
    ax_price.set_ylabel("Price ($)", color="white")
    ax_price.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="white", fontsize=9)
    ax_price.grid(True, alpha=0.15, color="#30363d")

    # ── Equity curve ─────────────────────────────────────────────────────────
    if len(trades_df) > 0:
        times = [trades_df[time_col_entry].min()] + list(trades_df[time_col_exit].sort_values())
        equity = [0.0] + list(stats["equity"])

        times = times[:len(equity)]
        pos_mask = np.array(equity) >= 0
        ax_equity.plot(times, equity, color="#58a6ff", linewidth=1.8, label="Equity")
        ax_equity.fill_between(times, 0, equity, where=pos_mask,   alpha=0.25, color="#2ea043")
        ax_equity.fill_between(times, 0, equity, where=~pos_mask,  alpha=0.25, color="#da3633")
        ax_equity.axhline(0, color="#8b949e", linestyle="--", linewidth=0.8)
        ax_equity.text(times[-1], equity[-1],
                       f"  ${equity[-1]:.2f}", color="#58a6ff", fontsize=10, va="center")

    ax_equity.set_title("Equity Curve", color="white", fontsize=13)
    ax_equity.set_ylabel("Cumulative PnL ($)", color="white")
    ax_equity.set_xlabel("Timestamp (µs)", color="white")
    ax_equity.grid(True, alpha=0.15, color="#30363d")

    # ── Drawdown ──────────────────────────────────────────────────────────────
    if len(trades_df) > 0:
        dd_times = list(trades_df[time_col_exit].sort_values())
        dd_vals  = list(-stats["drawdown"])
        if len(dd_times) == len(dd_vals):
            ax_dd.fill_between(dd_times, 0, dd_vals, color="#da3633", alpha=0.6)
            ax_dd.plot(dd_times, dd_vals, color="#da3633", linewidth=1)
            ax_dd.axhline(0, color="#8b949e", linewidth=0.5)

    ax_dd.set_title("Drawdown", color="white", fontsize=12)
    ax_dd.set_ylabel("Drawdown ($)", color="white")
    ax_dd.set_xlabel("Timestamp (µs)", color="white")
    ax_dd.grid(True, alpha=0.15, color="#30363d")

    # ── PnL distribution ──────────────────────────────────────────────────────
    pnls = trades_df["PnL"].values
    if len(pnls) > 0:
        wins_arr   = pnls[pnls > 0]
        losses_arr = pnls[pnls < 0]
        bins = max(10, len(pnls) // 3)
        if len(wins_arr):
            ax_dist.hist(wins_arr,   bins=bins, color="#2ea043", alpha=0.75, label="Win")
        if len(losses_arr):
            ax_dist.hist(losses_arr, bins=bins, color="#da3633", alpha=0.75, label="Loss")
        ax_dist.axvline(0, color="white", linewidth=1, linestyle="--")

    ax_dist.set_title("PnL Distribution", color="white", fontsize=12)
    ax_dist.set_xlabel("PnL ($)", color="white")
    ax_dist.set_ylabel("Count", color="white")
    ax_dist.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="white", fontsize=9)
    ax_dist.grid(True, alpha=0.15, color="#30363d")

    # ── Stats text box ────────────────────────────────────────────────────────
    stats_text = (
        f"Trades: {stats['num_trades']}   Win Rate: {stats['win_rate']:.1f}%\n"
        f"Total PnL: ${stats['total_pnl']:.2f}   Max DD: ${stats['max_drawdown']:.2f}\n"
        f"Avg Win: ${stats['avg_win']:.2f}   Avg Loss: ${stats['avg_loss']:.2f}\n"
        f"Profit Factor: {stats['profit_factor']:.2f}   Sharpe: {stats['sharpe']:.3f}"
    )
    fig.text(0.5, 0.005, stats_text, ha="center", va="bottom", fontsize=10,
             color="#c9d1d9", bbox=dict(facecolor="#21262d", edgecolor="#30363d",
                                        boxstyle="round,pad=0.4"))

    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out, dpi=200, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"[OK] Plot saved → {out}")

    # Also mirror to docs/images for README
    docs_out = Path("docs/images") / out.name
    docs_out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(docs_out, dpi=150, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"[OK] Docs copy saved → {docs_out}")

    plt.close(fig)


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Algo-Catalyst performance plotter")
    parser.add_argument("--trades",  default="trades.csv",         help="Trade log CSV")
    parser.add_argument("--ticks",   default="data/tick_data.csv", help="Tick data CSV")
    parser.add_argument("--output",  default="performance_plot.png", help="Output PNG path")
    # Legacy positional support
    parser.add_argument("trades_pos", nargs="?", help=argparse.SUPPRESS)
    parser.add_argument("ticks_pos",  nargs="?", help=argparse.SUPPRESS)
    args = parser.parse_args()

    trades_csv = args.trades_pos or args.trades
    ticks_csv  = args.ticks_pos  or args.ticks

    # Auto-discover trade log
    if not Path(trades_csv).exists():
        for candidate in ("build/trades.csv", "trades.csv"):
            if Path(candidate).exists():
                trades_csv = candidate
                break

    # Auto-discover tick data
    if not Path(ticks_csv).exists():
        for candidate in ("data/synthetic_catalyst.csv", "data/tick_data.csv"):
            if Path(candidate).exists():
                ticks_csv = candidate
                break

    print(f"Loading trades  : {trades_csv}")
    print(f"Loading tick data: {ticks_csv}")

    trades = load_trades(trades_csv)
    ticks  = load_tick_data(ticks_csv)

    if trades.empty:
        print("[ERROR] No trades found — cannot plot.", file=sys.stderr)
        sys.exit(1)

    print(f"Plotting {len(trades)} trades...")
    plot_performance(trades, ticks, output_path=args.output)


if __name__ == "__main__":
    main()
