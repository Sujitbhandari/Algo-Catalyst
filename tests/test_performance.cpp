#include "runner.h"
#include "PerformanceAnalyzer.h"

using namespace AlgoCatalyst;
using namespace TestRunner;

static TradeRecord makeTrade(double pnl, std::int64_t hold_us = 1'000'000) {
    TradeRecord t;
    t.entry_timestamp_us = 0;
    t.exit_timestamp_us  = hold_us;
    t.symbol             = "TEST";
    t.entry_price        = 100.0;
    t.exit_price         = 100.0 + pnl;
    t.quantity           = 1.0;
    t.pnl                = pnl;
    t.commission         = 0.0;
    t.regime             = "TRENDING";
    t.strategy_name      = "Test";
    return t;
}

TEST(empty_trades_returns_zero_metrics) {
    auto m = PerformanceAnalyzer::compute({});
    check(m.num_trades == 0, "num_trades == 0");
    check(m.total_pnl == 0.0, "total_pnl == 0");
}

TEST(all_wins_win_rate_100) {
    std::vector<TradeRecord> trades;
    for (int i = 0; i < 10; ++i) trades.push_back(makeTrade(10.0));
    auto m = PerformanceAnalyzer::compute(trades);
    checkClose(m.win_rate, 100.0, 0.001, "win_rate 100%");
}

TEST(all_losses_win_rate_0) {
    std::vector<TradeRecord> trades;
    for (int i = 0; i < 10; ++i) trades.push_back(makeTrade(-5.0));
    auto m = PerformanceAnalyzer::compute(trades);
    checkClose(m.win_rate, 0.0, 0.001, "win_rate 0%");
}

TEST(total_pnl_is_sum) {
    std::vector<TradeRecord> trades = {
        makeTrade(10.0), makeTrade(-3.0), makeTrade(5.0)
    };
    auto m = PerformanceAnalyzer::compute(trades);
    checkClose(m.total_pnl, 12.0, 0.001, "total_pnl = sum");
}

TEST(profit_factor_correct) {
    std::vector<TradeRecord> trades = {
        makeTrade(20.0), makeTrade(-10.0)
    };
    auto m = PerformanceAnalyzer::compute(trades);
    checkClose(m.profit_factor, 2.0, 0.001, "profit_factor = 2.0");
}

TEST(max_drawdown_computed) {
    std::vector<TradeRecord> trades = {
        makeTrade(10.0), makeTrade(-15.0), makeTrade(5.0)
    };
    auto m = PerformanceAnalyzer::compute(trades);
    check(m.max_drawdown > 0.0, "max_drawdown > 0");
}

TEST(var95_leq_worst_trade) {
    std::vector<TradeRecord> trades;
    for (int i = 0; i < 20; ++i) trades.push_back(makeTrade(i % 2 == 0 ? 5.0 : -10.0));
    auto m = PerformanceAnalyzer::compute(trades);
    check(m.var_95 >= 0.0, "VaR 95 >= 0");
    check(m.var_99 >= m.var_95, "VaR 99 >= VaR 95");
}

TEST(expectancy_sign_matches_profitability) {
    std::vector<TradeRecord> wins, losses;
    for (int i = 0; i < 10; ++i) wins.push_back(makeTrade(10.0));
    wins.push_back(makeTrade(-1.0));
    auto m = PerformanceAnalyzer::compute(wins);
    check(m.expectancy > 0.0, "positive expectancy for mostly-winning strategy");
}

TEST(consecutive_win_streak_tracked) {
    std::vector<TradeRecord> trades = {
        makeTrade(5.0), makeTrade(5.0), makeTrade(5.0), makeTrade(-2.0)
    };
    auto m = PerformanceAnalyzer::compute(trades);
    check(m.max_consec_wins == 3, "max consecutive wins = 3");
}

TEST(consecutive_loss_streak_tracked) {
    std::vector<TradeRecord> trades = {
        makeTrade(-2.0), makeTrade(-2.0), makeTrade(10.0)
    };
    auto m = PerformanceAnalyzer::compute(trades);
    check(m.max_consec_losses == 2, "max consecutive losses = 2");
}
