# Performance Metrics

`PerformanceAnalyzer::compute()` calculates the following metrics from a trade log.

---

## Trade counts

| Metric | Description |
|--------|-------------|
| `num_trades` | Total number of closed trades |
| `num_wins` | Trades with PnL > 0 |
| `num_losses` | Trades with PnL ≤ 0 |
| `win_rate` | `num_wins / num_trades × 100` |

---

## PnL metrics

| Metric | Description |
|--------|-------------|
| `total_pnl` | Sum of all trade PnLs |
| `gross_profit` | Sum of winning trade PnLs |
| `gross_loss` | Absolute sum of losing trade PnLs |
| `profit_factor` | `gross_profit / gross_loss` |
| `avg_win` | Average PnL of winning trades |
| `avg_loss` | Average PnL of losing trades |
| `best_trade` | Highest single-trade PnL |
| `worst_trade` | Lowest single-trade PnL |
| `median_pnl` | Median trade PnL (robust to outliers) |
| `pnl_std_dev` | Sample standard deviation of trade PnLs |

---

## Risk metrics

| Metric | Description |
|--------|-------------|
| `max_drawdown` | Peak-to-trough cumulative PnL drop in $ |
| `max_drawdown_pct` | Max drawdown as a percentage of the peak equity |
| `var_95` | 95% Value at Risk — worst loss not exceeded 95% of the time |
| `cvar_95` | Conditional VaR (Expected Shortfall) at 95% — avg of the worst 5% |
| `var_99` | 99% Value at Risk |
| `ulcer_index` | RMS of percentage drawdown — penalises prolonged drawdowns |

---

## Risk-adjusted returns

| Metric | Formula | Notes |
|--------|---------|-------|
| `sharpe_ratio` | `mean_pnl / std_dev_pnl` | Per-trade, risk-free rate = 0 |
| `sortino_ratio` | `mean_pnl / downside_dev` | Only penalises negative returns |
| `calmar_ratio` | `total_pnl / max_drawdown` | Higher = better recovery |
| `omega_ratio` | `gross_profit / gross_loss` | Equivalent to profit factor |
| `recovery_factor` | `total_pnl / max_drawdown` | Same as Calmar (no annualisation) |

---

## Expectancy

```
expectancy = (win_rate × avg_win) + (loss_rate × avg_loss)
```

Positive expectancy means the strategy makes money on average per trade. Target > $0.

---

## Streak metrics

| Metric | Description |
|--------|-------------|
| `max_consec_wins` | Longest run of consecutive winning trades |
| `max_consec_losses` | Longest run of consecutive losing trades |
| `avg_hold_time_s` | Average time between entry and exit in seconds |
