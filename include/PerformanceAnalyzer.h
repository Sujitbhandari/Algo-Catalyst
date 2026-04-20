#pragma once

#include "Engine.h"
#include <vector>
#include <string>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace AlgoCatalyst {

/**
 * PerformanceAnalyzer - standalone metrics engine that works on a TradeRecord vector.
 * Decoupled from Backtester so it can be used with externally loaded trade logs.
 */
class PerformanceAnalyzer {
public:
    struct Metrics {
        int    num_trades       = 0;
        int    num_wins         = 0;
        int    num_losses       = 0;
        double win_rate         = 0.0;
        double total_pnl        = 0.0;
        double gross_profit     = 0.0;
        double gross_loss       = 0.0;
        double profit_factor    = 0.0;
        double avg_win          = 0.0;
        double avg_loss         = 0.0;
        double best_trade       = 0.0;
        double worst_trade      = 0.0;
        double max_drawdown     = 0.0;
        double max_drawdown_pct = 0.0;
        double sharpe_ratio     = 0.0;
        double sortino_ratio    = 0.0;
        double calmar_ratio     = 0.0;
        double avg_hold_time_s  = 0.0;
        int    max_consec_wins  = 0;
        int    max_consec_losses= 0;
    };

    static Metrics compute(const std::vector<TradeRecord>& trades) {
        Metrics m;
        if (trades.empty()) return m;

        m.num_trades = static_cast<int>(trades.size());

        // PnL stats
        std::vector<double> pnls;
        pnls.reserve(trades.size());
        double hold_sum = 0.0;

        for (const auto& t : trades) {
            pnls.push_back(t.pnl);
            m.total_pnl += t.pnl;
            hold_sum += static_cast<double>(t.exit_timestamp_us - t.entry_timestamp_us) / 1e6;

            if (t.pnl > 0.0) {
                m.num_wins++;
                m.gross_profit += t.pnl;
                m.avg_win += t.pnl;
            } else {
                m.num_losses++;
                m.gross_loss += std::abs(t.pnl);
                m.avg_loss += t.pnl;
            }
        }

        m.win_rate  = m.num_trades > 0 ? 100.0 * m.num_wins / m.num_trades : 0.0;
        m.avg_win   = m.num_wins   > 0 ? m.avg_win  / m.num_wins   : 0.0;
        m.avg_loss  = m.num_losses > 0 ? m.avg_loss / m.num_losses : 0.0;
        m.best_trade  = *std::max_element(pnls.begin(), pnls.end());
        m.worst_trade = *std::min_element(pnls.begin(), pnls.end());
        m.profit_factor = m.gross_loss > 0.0 ? m.gross_profit / m.gross_loss : 0.0;
        m.avg_hold_time_s = hold_sum / trades.size();

        // Drawdown
        double peak = 0.0, equity = 0.0, max_dd = 0.0;
        for (double p : pnls) {
            equity += p;
            if (equity > peak) peak = equity;
            max_dd = std::max(max_dd, peak - equity);
        }
        m.max_drawdown = max_dd;
        m.max_drawdown_pct = peak > 0.0 ? 100.0 * max_dd / peak : 0.0;

        // Sharpe (trade-level, risk-free = 0)
        double mean = m.total_pnl / trades.size();
        double var = 0.0;
        for (double p : pnls) { double d = p - mean; var += d * d; }
        var /= std::max<std::size_t>(pnls.size() - 1, 1);
        double std_dev = std::sqrt(var);
        m.sharpe_ratio = std_dev > 0.0 ? mean / std_dev : 0.0;

        // Sortino (downside deviation)
        double downside_var = 0.0;
        int downside_count = 0;
        for (double p : pnls) {
            if (p < 0.0) { downside_var += p * p; downside_count++; }
        }
        double downside_dev = downside_count > 0 ?
            std::sqrt(downside_var / downside_count) : 0.0;
        m.sortino_ratio = downside_dev > 0.0 ? mean / downside_dev : 0.0;

        // Calmar
        m.calmar_ratio = m.max_drawdown > 0.0 ? m.total_pnl / m.max_drawdown : 0.0;

        // Consecutive wins/losses
        int cur_w = 0, cur_l = 0;
        for (double p : pnls) {
            if (p > 0.0) { cur_w++; cur_l = 0; m.max_consec_wins  = std::max(m.max_consec_wins,  cur_w); }
            else         { cur_l++; cur_w = 0; m.max_consec_losses = std::max(m.max_consec_losses, cur_l); }
        }

        return m;
    }

    static void print(const Metrics& m, std::ostream& out = std::cout) {
        out << std::fixed << std::setprecision(2);
        out << "\n╔══════════ PERFORMANCE REPORT ══════════╗\n";
        out << "  Trades:          " << m.num_trades << "  (W:" << m.num_wins << " / L:" << m.num_losses << ")\n";
        out << "  Win Rate:        " << m.win_rate << "%\n";
        out << "  Total PnL:       $" << m.total_pnl << "\n";
        out << "  Gross Profit:    $" << m.gross_profit << "\n";
        out << "  Gross Loss:      $" << m.gross_loss << "\n";
        out << "  Profit Factor:   " << m.profit_factor << "\n";
        out << "  Avg Win:         $" << m.avg_win << "\n";
        out << "  Avg Loss:        $" << m.avg_loss << "\n";
        out << "  Best Trade:      $" << m.best_trade << "\n";
        out << "  Worst Trade:     $" << m.worst_trade << "\n";
        out << "  Max Drawdown:    $" << m.max_drawdown << "  (" << m.max_drawdown_pct << "%)\n";
        out << "  Sharpe Ratio:    " << m.sharpe_ratio << "\n";
        out << "  Sortino Ratio:   " << m.sortino_ratio << "\n";
        out << "  Calmar Ratio:    " << m.calmar_ratio << "\n";
        out << "  Avg Hold (s):    " << m.avg_hold_time_s << "\n";
        out << "  Max Consec Wins: " << m.max_consec_wins << "\n";
        out << "  Max Consec Loss: " << m.max_consec_losses << "\n";
        out << "╚════════════════════════════════════════╝\n";
    }
};

} // namespace AlgoCatalyst
