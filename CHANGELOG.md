# Changelog

All notable changes to this project will be documented here.

## [1.3.0] — 2026-04-24

### Added
- Seven new indicators: WMA, OBV, Williams %R, Donchian Channel, DEMA, CCI, CMF
- `BreakoutStrategy` — trades Donchian Channel breakouts confirmed by CCI and volume
- `ConfigLoader` — header-only flat JSON config parser, no external dependencies
- `config.json` — default parameter file for all strategy and execution settings
- VaR 95%, VaR 99%, CVaR 95%, expectancy, and recovery factor in `PerformanceAnalyzer`
- MAE (Maximum Adverse Excursion) and MFE (Maximum Favorable Excursion) per trade
- JSON trade log export (`exportTradeLogToJSON`) in addition to CSV
- NYSE market hours filter for `NewsMomentumStrategy` (configurable UTC window)
- ATR-based volatility scaling stacked on top of Kelly and regime multipliers
- `--config`, `--strategy`, `--json-output` CLI flags in `main.cpp`
- `Makefile` with build, run, optimize, walk-forward, compare, and Docker targets
- `Dockerfile` — multi-stage build for containerized backtesting
- `tests/` — zero-dependency test runner with 20 indicator and performance tests
- `scripts/walk_forward.py` — rolling IS/OOS window validation
- `scripts/optimize_params.py` — grid-search parameter sweep
- `scripts/compare_strategies.py` — cross-scenario performance table
- `scripts/heatmap.py` — 2-D sensitivity heatmap from optimization results
- `scripts/export_metrics.py` — Python metrics-to-JSON exporter
- `.editorconfig` for consistent style across editors and IDEs

### Changed
- `CMakeLists.txt` — version bumped to 1.3.0, added `AlgoCatalystTests` build target
- CI pipeline now runs unit tests and exports metrics JSON as a build artifact
- CSV export includes MAE and MFE columns

## [1.2.0] — 2026-04-19

### Added
- `Stochastic Oscillator` %K and %D indicator
- `CONTRIBUTING.md` with dev setup, style guide, and PR guidelines
- `PerformanceAnalyzer` with Sortino, Calmar, best/worst trade, and streak metrics
- Max drawdown and daily loss circuit breakers
- Fractional Kelly criterion for position sizing
- VOLATILE regime detection
- `visualize.sh` — full build-generate-backtest-plot pipeline

### Changed
- README simplified to plain English
- MACD SMA warm-up fixed for 12/26 EMA components

## [1.1.0] — 2026-01-17

### Added
- Initial event-driven backtesting engine
- `NewsMomentumStrategy` and `MeanReversionStrategy`
- EMA, MACD, RSI, Bollinger Bands, ATR, VWAP indicators
- k-Means regime classifier (CHOPPY / TRENDING / VOLATILE)
- GBM data generator with four market scenarios
- Dark-themed performance chart with equity curve and drawdown panel
- GitHub Actions CI for build, smoke test, and Python lint
