#!/usr/bin/env python3
"""
HTML backtest report generator for Algo-Catalyst.

Reads a trades CSV and produces a self-contained HTML file with an embedded
equity curve, drawdown chart, PnL distribution, and a full metrics table.
No external dependencies beyond matplotlib and numpy.

Usage:
    python3 scripts/backtest_report.py --trades trades.csv --output report.html
"""

import argparse
import base64
import csv
import io
import json
import math
from datetime import datetime
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    import sys
    print("[ERROR] matplotlib and numpy are required: pip install matplotlib numpy")
    sys.exit(1)


def load_trades(path: Path) -> list[dict]:
    rows = []
    with open(path) as f:
        for row in csv.DictReader(f):
            parsed = {}
            for k, v in row.items():
                try:
                    parsed[k] = float(v)
                except (ValueError, TypeError):
                    parsed[k] = v
            rows.append(parsed)
    return rows


def compute_equity_drawdown(pnls):
    equity = []
    drawdown = []
    eq = peak = 0.0
    for p in pnls:
        eq += p
        equity.append(eq)
        if eq > peak:
            peak = eq
        drawdown.append(-(peak - eq))
    return equity, drawdown


def fig_to_b64(fig) -> str:
    buf = io.BytesIO()
    fig.savefig(buf, format="png", dpi=110, bbox_inches="tight",
                facecolor=fig.get_facecolor())
    buf.seek(0)
    return base64.b64encode(buf.read()).decode()


def make_chart(trades: list[dict]) -> str:
    pnls = [t.get("PnL", 0.0) for t in trades]
    if not pnls:
        return ""

    equity, drawdown = compute_equity_drawdown(pnls)
    xs = list(range(1, len(pnls) + 1))

    fig, axes = plt.subplots(3, 1, figsize=(12, 9),
                             gridspec_kw={"height_ratios": [2, 1, 1]})
    fig.patch.set_facecolor("#0d1117")
    plt.subplots_adjust(hspace=0.35)

    colors = {"up": "#3fb950", "down": "#f85149", "neutral": "#8b949e"}

    ax1 = axes[0]
    ax1.set_facecolor("#0d1117")
    ax1.plot(xs, equity, color="#58a6ff", linewidth=1.5, label="Equity")
    ax1.axhline(0, color="#30363d", linewidth=0.8)
    ax1.fill_between(xs, 0, equity,
                     where=[e >= 0 for e in equity],
                     alpha=0.15, color="#3fb950")
    ax1.fill_between(xs, 0, equity,
                     where=[e < 0 for e in equity],
                     alpha=0.15, color="#f85149")
    ax1.set_ylabel("Equity ($)", color="white")
    ax1.set_title("Equity Curve", color="white", fontsize=11)
    ax1.tick_params(colors="white")
    for spine in ax1.spines.values():
        spine.set_edgecolor("#30363d")

    ax2 = axes[1]
    ax2.set_facecolor("#0d1117")
    ax2.fill_between(xs, drawdown, 0, alpha=0.6, color="#f85149")
    ax2.plot(xs, drawdown, color="#f85149", linewidth=0.8)
    ax2.set_ylabel("Drawdown ($)", color="white")
    ax2.set_title("Drawdown", color="white", fontsize=11)
    ax2.tick_params(colors="white")
    for spine in ax2.spines.values():
        spine.set_edgecolor("#30363d")

    ax3 = axes[2]
    ax3.set_facecolor("#0d1117")
    pos = [p for p in pnls if p >= 0]
    neg = [p for p in pnls if p < 0]
    bins = np.linspace(min(pnls) * 1.1, max(pnls) * 1.1, 30)
    if pos:
        ax3.hist(pos, bins=bins, color="#3fb950", alpha=0.7, label="Wins")
    if neg:
        ax3.hist(neg, bins=bins, color="#f85149", alpha=0.7, label="Losses")
    ax3.axvline(0, color="white", linewidth=0.8)
    ax3.set_ylabel("Frequency", color="white")
    ax3.set_title("PnL Distribution", color="white", fontsize=11)
    ax3.tick_params(colors="white")
    ax3.legend(facecolor="#161b22", edgecolor="#30363d",
               labelcolor="white", fontsize=8)
    for spine in ax3.spines.values():
        spine.set_edgecolor("#30363d")

    return fig_to_b64(fig)


def compute_metrics(trades: list[dict]) -> dict:
    pnls = [t.get("PnL", 0.0) for t in trades]
    if not pnls:
        return {}
    n = len(pnls)
    wins = [p for p in pnls if p > 0]
    losses = [p for p in pnls if p <= 0]
    total = sum(pnls)
    mean = total / n
    var = sum((p - mean)**2 for p in pnls) / max(n - 1, 1)
    std = math.sqrt(var) if var > 0 else 0.0
    peak = eq = max_dd = 0.0
    for p in pnls:
        eq += p
        peak = max(peak, eq)
        max_dd = max(max_dd, peak - eq)
    sorted_p = sorted(pnls)
    median = sorted_p[n // 2] if n % 2 else (sorted_p[n//2-1] + sorted_p[n//2]) / 2
    return {
        "Trades": n,
        "Wins": len(wins),
        "Losses": len(losses),
        "Win Rate": f"{100*len(wins)/n:.1f}%",
        "Total PnL": f"${total:.2f}",
        "Avg Win": f"${sum(wins)/len(wins):.2f}" if wins else "N/A",
        "Avg Loss": f"${sum(losses)/len(losses):.2f}" if losses else "N/A",
        "Median PnL": f"${median:.2f}",
        "Best Trade": f"${max(pnls):.2f}",
        "Worst Trade": f"${min(pnls):.2f}",
        "Profit Factor": f"{sum(wins)/abs(sum(losses)):.2f}" if losses and sum(losses) != 0 else "∞",
        "Max Drawdown": f"${max_dd:.2f}",
        "Sharpe Ratio": f"{mean/std:.3f}" if std else "N/A",
        "PnL Std Dev": f"${std:.2f}",
    }


def render_html(chart_b64: str, metrics: dict, trades_path: str) -> str:
    rows = "".join(
        f"<tr><td>{k}</td><td>{v}</td></tr>" for k, v in metrics.items()
    )
    img = f'<img src="data:image/png;base64,{chart_b64}" style="width:100%;max-width:900px">' if chart_b64 else ""
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Algo-Catalyst Backtest Report</title>
<style>
  body {{font-family:monospace;background:#0d1117;color:#c9d1d9;margin:0;padding:24px}}
  h1 {{color:#58a6ff;font-size:1.4em;margin-bottom:4px}}
  .sub {{color:#8b949e;font-size:.85em;margin-bottom:24px}}
  table {{border-collapse:collapse;min-width:340px}}
  td {{padding:6px 14px;border-bottom:1px solid #21262d}}
  td:first-child {{color:#8b949e;width:160px}}
  td:last-child {{color:#e6edf3;font-weight:bold}}
  .chart {{margin:24px 0}}
</style>
</head>
<body>
<h1>Algo-Catalyst — Backtest Report</h1>
<div class="sub">Generated {datetime.utcnow().strftime('%Y-%m-%d %H:%M')} UTC &nbsp;·&nbsp; {trades_path}</div>
<table>{rows}</table>
<div class="chart">{img}</div>
</body>
</html>"""


def main():
    parser = argparse.ArgumentParser(description="Generate HTML backtest report")
    parser.add_argument("--trades", type=Path, default=Path("trades.csv"))
    parser.add_argument("--output", type=Path, default=Path("report.html"))
    args = parser.parse_args()

    if not args.trades.exists():
        print(f"[ERROR] {args.trades} not found")
        return

    trades = load_trades(args.trades)
    metrics = compute_metrics(trades)
    chart = make_chart(trades)
    html = render_html(chart, metrics, str(args.trades))

    args.output.write_text(html)
    print(f"Report written to {args.output}  ({len(trades)} trades)")


if __name__ == "__main__":
    main()
