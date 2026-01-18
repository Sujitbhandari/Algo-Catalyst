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
    
    // Price metrics
    void updatePrice(double price);
    double getGapUpPercent() const;  // Percent change from previous close
    
    // Reset indicators for new symbol
    void reset();
    
private:
    // EMA storage: period -> (current_value, alpha)
    std::map<std::size_t, std::pair<double, double>> emas_;
    
    // MACD components
    double ema_12_;
    double ema_26_;
    double macd_signal_ema_9_;
    std::deque<double> macd_histogram_history_;
    
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

