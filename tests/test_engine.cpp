#include "runner.h"
#include "Engine.h"

using namespace AlgoCatalyst;
using namespace TestRunner;

TEST(tick_loader_returns_empty_on_missing_file) {
    auto ticks = TickLoader::loadFromCSV("/nonexistent/path/that/does/not/exist.csv");
    check(ticks.empty(), "TickLoader returns empty on missing file");
}

TEST(backtester_starts_with_zero_trades) {
    Backtester bt(200.0);
    check(bt.getNumTrades() == 0, "Zero trades before run");
    checkClose(bt.getTotalPnL(), 0.0, 1e-9, "Zero PnL before run");
}

TEST(backtester_win_rate_zero_on_empty) {
    Backtester bt(200.0);
    checkClose(bt.getWinRate(), 0.0, 1e-9, "Win rate 0 on empty log");
}

TEST(backtester_sharpe_zero_on_empty) {
    Backtester bt(200.0);
    checkClose(bt.getSharpeRatio(), 0.0, 1e-9, "Sharpe 0 on empty log");
}

TEST(backtester_max_drawdown_zero_on_empty) {
    Backtester bt(200.0);
    checkClose(bt.getMaxDrawdown(), 0.0, 1e-9, "Max drawdown 0 on empty log");
}

TEST(backtester_not_halted_initially) {
    Backtester bt(200.0);
    check(!bt.isHaltedByRisk(), "Not halted on construction");
}

TEST(backtester_export_csv_fails_on_bad_path) {
    Backtester bt(200.0);
    bool ok = bt.exportTradeLogToCSV("/nonexistent/dir/that/cannot/exist/trades.csv");
    check(!ok, "exportTradeLogToCSV returns false on bad path");
}

TEST(backtester_export_json_fails_on_bad_path) {
    Backtester bt(200.0);
    bool ok = bt.exportTradeLogToJSON("/nonexistent/dir/that/cannot/exist/trades.json");
    check(!ok, "exportTradeLogToJSON returns false on bad path");
}

TEST(backtester_slippage_setter) {
    Backtester bt(200.0);
    bt.setSlippageBps(10.0);
    check(!bt.isHaltedByRisk(), "Setting slippage does not halt engine");
}

TEST(backtester_commission_free_mode) {
    Backtester bt(200.0);
    bt.setCommissionFree(true);
    check(!bt.isHaltedByRisk(), "Commission-free mode does not corrupt state");
}
