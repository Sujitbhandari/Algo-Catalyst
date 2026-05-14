# Changelog

All notable changes to this project will be documented here.

## [1.4.0] — 2026-05-14

### Added
- Five new indicators: TRIX, ADX (+DI/-DI), MFI, KAMA, and Pivot Points (R1–R3, S1–S3)
- Bullish RSI divergence detection in `MeanReversionStrategy`
- Consecutive-loss circuit breaker (`setMaxConsecLosses`)
- Commission-free mode (`setCommissionFree`)
- `--dry-run` CLI flag for config verification without executing
- `scripts/monte_carlo.py` — bootstrap equity curve simulation with ruin probability
- `scripts/sharpe_tearsheet.py` — rolling Sharpe + rolling win-rate three-panel chart
- `scripts/plot_mae_mfe.py` — MAE/MFE scatter plot with MFE capture ratio
- `scripts/batch_backtest.sh` — run multiple config files and collect summary CSV
- `configs/` preset directory: `aggressive.json`, `conservative.json`, `breakout.json`, `meanrev.json`
- `docs/indicators.md` — full formula reference for all 18 indicators
- `docs/strategies.md` — entry/exit logic and position sizing for all three strategies
- GitHub issue templates for bug reports and feature requests
- `tests/test_regime.cpp` — 5 regime classifier unit tests
- `tests/test_engine.cpp` — 10 backtester and tick-loader unit tests
- Dedicated `unit-tests` CI job using ctest on ubuntu-22.04

### Fixed
- `getSharpeRatio` — guarded against NaN/Inf with epsilon check and sample-variance floor

### Changed
- CMakeLists: version bumped to 1.4.0; Engine.cpp and Strategy.cpp added to test build
- Makefile: added `monte-carlo`, `sharpe-tearsheet`, `mae-mfe`, `batch` targets

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
