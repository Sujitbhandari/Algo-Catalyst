#pragma once

#include <vector>
#include <deque>
#include <map>
#include <cstdint>
#include "Events.h"

namespace AlgoCatalyst {

// Indicator Calculation Engine
class Indicators {
public:
    Indicators();
    
    // EMA (Exponential Moving Average) - manually implemented
    void updateEMA(double price, std::size_t period);
    double getEMA(std::size_t period) const;
    bool isPriceAboveEMA(double price, std::size_t period) const;
    
    // MACD (Moving Average Convergence Divergence) - manually implemented
    void updateMACD(double price);
    double getMACD() const;
    double getMACDSignal() const;
    double getMACDHistogram() const;
    bool isMACDHistogramExpanding() const;
    
    // VWAP (Volume Weighted Average Price) - manually implemented
    void updateVWAP(double price, std::int64_t volume, std::int64_t timestamp_us);
    double getVWAP() const;
    bool isPriceAboveVWAP(double price) const;
    void resetVWAP();  // Reset at start of new trading day
    
    // Volume metrics
    void updateVolume(std::int64_t volume, std::int64_t timestamp_us);
    double getAverageVolume(std::size_t lookback = 20) const;
    double getRelativeVolume() const;  // Current volume / Average volume
    
    // RSI (Relative Strength Index) - manually implemented
    void updateRSI(double price, std::size_t period = 14);
    double getRSI(std::size_t period = 14) const;
    bool isOversold(std::size_t period = 14, double threshold = 30.0) const;
    bool isOverbought(std::size_t period = 14, double threshold = 70.0) const;

    // Bollinger Bands - manually implemented
    void updateBollingerBands(double price, std::size_t period = 20, double std_dev_mult = 2.0);
    double getBollingerUpper() const;
    double getBollingerMiddle() const;
    double getBollingerLower() const;
    double getBollingerBandwidth() const;  // (upper - lower) / middle
    bool isPriceAboveUpperBand() const;
    bool isPriceBelowLowerBand() const;

    // ATR (Average True Range) - measures volatility
    void updateATR(double high, double low, double close, std::size_t period = 14);
    double getATR(std::size_t period = 14) const;
    double getATRPercent(double price, std::size_t period = 14) const;  // ATR / price * 100

    // Price metrics
    void updatePrice(double price);
    double getGapUpPercent() const;  // Percent change from open price
    
    // Reset indicators for new symbol
    void reset();
    
private:
    // EMA storage: period -> (current_value, alpha, tick_count)
    struct EMAState {
        double value;
        double alpha;
        std::size_t tick_count;
    };
    std::map<std::size_t, EMAState> emas_;
    
    // MACD components
    double ema_12_;
    double ema_26_;
    double macd_signal_ema_9_;
    std::size_t macd_tick_count_;
    std::deque<double> macd_histogram_history_;
    
    // RSI components: period -> (avg_gain, avg_loss, prev_price, tick_count)
    struct RSIState {
        double avg_gain;
        double avg_loss;
        double prev_price;
        std::size_t tick_count;
    };
    std::map<std::size_t, RSIState> rsi_states_;

    // Bollinger Bands components
    std::deque<double> bb_price_history_;
    double bb_upper_;
    double bb_middle_;
    double bb_lower_;
    std::size_t bb_period_;
    double bb_std_dev_mult_;

    // ATR components: period -> (atr_value, prev_close, tick_count)
    struct ATRState {
        double atr;
        double prev_close;
        std::size_t tick_count;
    };
    std::map<std::size_t, ATRState> atr_states_;

    // VWAP components
    double cumulative_price_volume_;
    std::int64_t cumulative_volume_;
    std::int64_t vwap_session_start_us_;
    
    // Volume tracking
    std::deque<std::pair<std::int64_t, std::int64_t>> volume_history_;  // (timestamp, volume)
    
    // Price tracking
    double prev_close_;
    double current_price_;
    double open_price_;
    bool is_first_tick_;
};

} // namespace AlgoCatalyst

