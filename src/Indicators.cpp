#include "Indicators.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace AlgoCatalyst {

Indicators::Indicators()
    : ema_12_(0.0), ema_26_(0.0), macd_signal_ema_9_(0.0), macd_tick_count_(0),
      bb_upper_(0.0), bb_middle_(0.0), bb_lower_(0.0), bb_period_(20), bb_std_dev_mult_(2.0),
      cumulative_price_volume_(0.0), cumulative_volume_(0),
      vwap_session_start_us_(0), prev_close_(0.0), current_price_(0.0),
      open_price_(0.0), is_first_tick_(true) {
}

void Indicators::updateEMA(double price, std::size_t period) {
    auto it = emas_.find(period);
    
    if (it == emas_.end()) {
        double alpha = 2.0 / (period + 1.0);
        emas_[period] = {price, alpha, 1};
    } else {
        EMAState& state = it->second;
        state.tick_count++;
        // Use SMA for the first 'period' ticks for a proper warm-up
        if (state.tick_count <= period) {
            state.value = state.value + (price - state.value) / state.tick_count;
        } else {
            state.value = state.alpha * price + (1.0 - state.alpha) * state.value;
        }
    }
}

double Indicators::getEMA(std::size_t period) const {
    auto it = emas_.find(period);
    if (it != emas_.end()) {
        // Only return EMA after warm-up period
        if (it->second.tick_count >= period) {
            return it->second.value;
        }
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

void Indicators::updateRSI(double price, std::size_t period) {
    auto it = rsi_states_.find(period);
    
    if (it == rsi_states_.end()) {
        RSIState state;
        state.avg_gain = 0.0;
        state.avg_loss = 0.0;
        state.prev_price = price;
        state.tick_count = 1;
        rsi_states_[period] = state;
        return;
    }
    
    RSIState& state = it->second;
    double change = price - state.prev_price;
    double gain = (change > 0.0) ? change : 0.0;
    double loss = (change < 0.0) ? -change : 0.0;
    
    state.tick_count++;
    
    if (state.tick_count <= period) {
        // Accumulate initial averages via SMA
        state.avg_gain += (gain - state.avg_gain) / state.tick_count;
        state.avg_loss += (loss - state.avg_loss) / state.tick_count;
    } else {
        // Wilder smoothing
        double alpha = 1.0 / period;
        state.avg_gain = alpha * gain + (1.0 - alpha) * state.avg_gain;
        state.avg_loss = alpha * loss + (1.0 - alpha) * state.avg_loss;
    }
    
    state.prev_price = price;
}

double Indicators::getRSI(std::size_t period) const {
    auto it = rsi_states_.find(period);
    if (it == rsi_states_.end() || it->second.tick_count < period) return 50.0;
    
    const RSIState& state = it->second;
    if (state.avg_loss == 0.0) return 100.0;
    
    double rs = state.avg_gain / state.avg_loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

bool Indicators::isOversold(std::size_t period, double threshold) const {
    return getRSI(period) < threshold;
}

bool Indicators::isOverbought(std::size_t period, double threshold) const {
    return getRSI(period) > threshold;
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

void Indicators::updateBollingerBands(double price, std::size_t period, double std_dev_mult) {
    bb_period_ = period;
    bb_std_dev_mult_ = std_dev_mult;
    bb_price_history_.push_back(price);

    if (bb_price_history_.size() > period) {
        bb_price_history_.pop_front();
    }

    if (bb_price_history_.size() < period) return;

    double mean = std::accumulate(bb_price_history_.begin(), bb_price_history_.end(), 0.0) / period;

    double variance = 0.0;
    for (double p : bb_price_history_) {
        double diff = p - mean;
        variance += diff * diff;
    }
    double std_dev = std::sqrt(variance / period);

    bb_middle_ = mean;
    bb_upper_ = mean + std_dev_mult * std_dev;
    bb_lower_ = mean - std_dev_mult * std_dev;
}

double Indicators::getBollingerUpper() const { return bb_upper_; }
double Indicators::getBollingerMiddle() const { return bb_middle_; }
double Indicators::getBollingerLower() const { return bb_lower_; }

double Indicators::getBollingerBandwidth() const {
    if (bb_middle_ == 0.0) return 0.0;
    return (bb_upper_ - bb_lower_) / bb_middle_;
}

bool Indicators::isPriceAboveUpperBand() const {
    return bb_upper_ > 0.0 && current_price_ > bb_upper_;
}

bool Indicators::isPriceBelowLowerBand() const {
    return bb_lower_ > 0.0 && current_price_ < bb_lower_;
}

void Indicators::reset() {
    emas_.clear();
    ema_12_ = 0.0;
    ema_26_ = 0.0;
    macd_signal_ema_9_ = 0.0;
    macd_tick_count_ = 0;
    macd_histogram_history_.clear();
    rsi_states_.clear();
    bb_price_history_.clear();
    bb_upper_ = 0.0;
    bb_middle_ = 0.0;
    bb_lower_ = 0.0;
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

