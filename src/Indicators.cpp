#include "Indicators.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace AlgoCatalyst {

Indicators::Indicators()
    : ema_12_(0.0), ema_26_(0.0), macd_signal_ema_9_(0.0),
      cumulative_price_volume_(0.0), cumulative_volume_(0),
      vwap_session_start_us_(0), prev_close_(0.0), current_price_(0.0),
      open_price_(0.0), is_first_tick_(true) {
}

void Indicators::updateEMA(double price, std::size_t period) {
    auto it = emas_.find(period);
    
    if (it == emas_.end()) {
        // Initialize EMA with first price
        double alpha = 2.0 / (period + 1.0);
        emas_[period] = {price, alpha};
    } else {
        // Update EMA: EMA = alpha * price + (1 - alpha) * prev_EMA
        double& ema_value = it->second.first;
        double alpha = it->second.second;
        ema_value = alpha * price + (1.0 - alpha) * ema_value;
    }
}

double Indicators::getEMA(std::size_t period) const {
    auto it = emas_.find(period);
    if (it != emas_.end()) {
        return it->second.first;
    }
    return 0.0;
}

bool Indicators::isPriceAboveEMA(double price, std::size_t period) const {
    double ema = getEMA(period);
    return ema > 0.0 && price > ema;
}

void Indicators::updateMACD(double price) {
    // Update 12-period EMA
    if (ema_12_ == 0.0) {
        ema_12_ = price;
    } else {
        double alpha_12 = 2.0 / (12.0 + 1.0);
        ema_12_ = alpha_12 * price + (1.0 - alpha_12) * ema_12_;
    }
    
    // Update 26-period EMA
    if (ema_26_ == 0.0) {
        ema_26_ = price;
    } else {
        double alpha_26 = 2.0 / (26.0 + 1.0);
        ema_26_ = alpha_26 * price + (1.0 - alpha_26) * ema_26_;
    }
    
    // Calculate MACD line
    double macd_line = ema_12_ - ema_26_;
    
    // Update signal line (9-period EMA of MACD line)
    if (macd_signal_ema_9_ == 0.0) {
        macd_signal_ema_9_ = macd_line;
    } else {
        double alpha_9 = 2.0 / (9.0 + 1.0);
        macd_signal_ema_9_ = alpha_9 * macd_line + (1.0 - alpha_9) * macd_signal_ema_9_;
    }
    
    // Store histogram
    double histogram = macd_line - macd_signal_ema_9_;
    macd_histogram_history_.push_back(histogram);
    
    // Keep only last 10 histograms for expansion check
    if (macd_histogram_history_.size() > 10) {
        macd_histogram_history_.pop_front();
    }
}

double Indicators::getMACD() const {
    return ema_12_ - ema_26_;
}

double Indicators::getMACDSignal() const {
    return macd_signal_ema_9_;
}

double Indicators::getMACDHistogram() const {
    if (macd_histogram_history_.empty()) return 0.0;
    return macd_histogram_history_.back();
}

bool Indicators::isMACDHistogramExpanding() const {
    if (macd_histogram_history_.size() < 2) return false;
    
    // Check if histogram is increasing (last value > previous value)
    auto it = macd_histogram_history_.rbegin();
    double current = *it;
    double previous = *(++it);
    
    return current > previous;
}

void Indicators::updateVWAP(double price, std::int64_t volume, std::int64_t timestamp_us) {
    // Reset VWAP at start of new session (first tick or new day)
    if (vwap_session_start_us_ == 0) {
        vwap_session_start_us_ = timestamp_us;
        cumulative_price_volume_ = 0.0;
        cumulative_volume_ = 0;
    }
    
    // Update cumulative values
    cumulative_price_volume_ += price * volume;
    cumulative_volume_ += volume;
}

double Indicators::getVWAP() const {
    if (cumulative_volume_ == 0) return 0.0;
    return cumulative_price_volume_ / cumulative_volume_;
}

bool Indicators::isPriceAboveVWAP(double price) const {
    double vwap = getVWAP();
    return vwap > 0.0 && price > vwap;
}

void Indicators::resetVWAP() {
    cumulative_price_volume_ = 0.0;
    cumulative_volume_ = 0;
    vwap_session_start_us_ = 0;
}

void Indicators::updateVolume(std::int64_t volume, std::int64_t timestamp_us) {
    volume_history_.push_back({timestamp_us, volume});
    
    // Keep only last 20 ticks for average calculation
    if (volume_history_.size() > 20) {
        volume_history_.pop_front();
    }
}

double Indicators::getAverageVolume(std::size_t lookback) const {
    if (volume_history_.size() < 2) return 0.0;
    
    std::size_t count = std::min(lookback, volume_history_.size());
    std::int64_t sum = 0;
    
    auto start_it = volume_history_.end() - count;
    for (auto it = start_it; it != volume_history_.end(); ++it) {
        sum += it->second;
    }
    
    return static_cast<double>(sum) / count;
}

double Indicators::getRelativeVolume() const {
    double avg_vol = getAverageVolume();
    if (avg_vol == 0.0) return 0.0;
    
    if (volume_history_.empty()) return 0.0;
    std::int64_t current_vol = volume_history_.back().second;
    
    return static_cast<double>(current_vol) / avg_vol;
}

void Indicators::updatePrice(double price) {
    if (is_first_tick_) {
        prev_close_ = price;
        open_price_ = price;
        is_first_tick_ = false;
    } else {
        prev_close_ = current_price_;
    }
    current_price_ = price;
}

double Indicators::getGapUpPercent() const {
    if (prev_close_ == 0.0) return 0.0;
    return ((current_price_ - prev_close_) / prev_close_) * 100.0;
}

void Indicators::reset() {
    emas_.clear();
    ema_12_ = 0.0;
    ema_26_ = 0.0;
    macd_signal_ema_9_ = 0.0;
    macd_histogram_history_.clear();
    cumulative_price_volume_ = 0.0;
    cumulative_volume_ = 0;
    vwap_session_start_us_ = 0;
    volume_history_.clear();
    prev_close_ = 0.0;
    current_price_ = 0.0;
    open_price_ = 0.0;
    is_first_tick_ = true;
}

} // namespace AlgoCatalyst

