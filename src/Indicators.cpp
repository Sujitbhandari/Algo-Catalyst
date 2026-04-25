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
    macd_tick_count_++;

    // SMA warm-up for 12-EMA
    if (macd_tick_count_ == 1) {
        ema_12_ = price;
        ema_26_ = price;
    } else if (macd_tick_count_ <= 12) {
        ema_12_ += (price - ema_12_) / macd_tick_count_;
        ema_26_ += (price - ema_26_) / macd_tick_count_;
    } else if (macd_tick_count_ <= 26) {
        double alpha_12 = 2.0 / (12.0 + 1.0);
        ema_12_ = alpha_12 * price + (1.0 - alpha_12) * ema_12_;
        ema_26_ += (price - ema_26_) / macd_tick_count_;
    } else {
        double alpha_12 = 2.0 / (12.0 + 1.0);
        double alpha_26 = 2.0 / (26.0 + 1.0);
        ema_12_ = alpha_12 * price + (1.0 - alpha_12) * ema_12_;
        ema_26_ = alpha_26 * price + (1.0 - alpha_26) * ema_26_;
    }

    // Only compute signal / histogram after 26-tick warm-up
    if (macd_tick_count_ < 26) return;

    double macd_line = ema_12_ - ema_26_;

    if (macd_tick_count_ == 26) {
        macd_signal_ema_9_ = macd_line;
    } else {
        double alpha_9 = 2.0 / (9.0 + 1.0);
        macd_signal_ema_9_ = alpha_9 * macd_line + (1.0 - alpha_9) * macd_signal_ema_9_;
    }

    double histogram = macd_line - macd_signal_ema_9_;
    macd_histogram_history_.push_back(histogram);
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
    // Reset VWAP at start of new session or when crossing midnight boundary
    if (vwap_session_start_us_ == 0) {
        vwap_session_start_us_ = timestamp_us;
        cumulative_price_volume_ = 0.0;
        cumulative_volume_ = 0;
    } else {
        // Detect new trading day: check if the date portion of the timestamp changed
        // Timestamps are microseconds since epoch; one day = 86400 * 1e6 us
        constexpr std::int64_t US_PER_DAY = 86400LL * 1'000'000LL;
        std::int64_t session_day = vwap_session_start_us_ / US_PER_DAY;
        std::int64_t current_day = timestamp_us / US_PER_DAY;
        if (current_day != session_day) {
            resetVWAP();
            vwap_session_start_us_ = timestamp_us;
        }
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
    // Gap is measured from the session open price vs previous close
    // For intraday data where we lack a true prev_close, use open_price_ as the reference
    if (open_price_ == 0.0) return 0.0;
    return ((current_price_ - open_price_) / open_price_) * 100.0;
}

void Indicators::updateATR(double high, double low, double close, std::size_t period) {
    auto it = atr_states_.find(period);

    if (it == atr_states_.end()) {
        ATRState state;
        state.atr = high - low;
        state.prev_close = close;
        state.tick_count = 1;
        atr_states_[period] = state;
        return;
    }

    ATRState& state = it->second;
    state.tick_count++;

    double tr = std::max({high - low,
                          std::abs(high - state.prev_close),
                          std::abs(low - state.prev_close)});

    if (state.tick_count <= period) {
        // SMA warm-up
        state.atr += (tr - state.atr) / state.tick_count;
    } else {
        // Wilder smoothing
        state.atr = (state.atr * (period - 1) + tr) / period;
    }

    state.prev_close = close;
}

double Indicators::getATR(std::size_t period) const {
    auto it = atr_states_.find(period);
    if (it == atr_states_.end() || it->second.tick_count < period) return 0.0;
    return it->second.atr;
}

double Indicators::getATRPercent(double price, std::size_t period) const {
    if (price == 0.0) return 0.0;
    return (getATR(period) / price) * 100.0;
}

void Indicators::updateStochastic(double high, double low, double close,
                                   std::size_t k_period, std::size_t d_period) {
    stoch_highs_.push_back(high);
    stoch_lows_.push_back(low);
    if (stoch_highs_.size() > k_period) stoch_highs_.pop_front();
    if (stoch_lows_.size()  > k_period) stoch_lows_.pop_front();

    if (stoch_highs_.size() < k_period) return;

    double highest = *std::max_element(stoch_highs_.begin(), stoch_highs_.end());
    double lowest  = *std::min_element(stoch_lows_.begin(),  stoch_lows_.end());

    stoch_k_ = (highest == lowest) ? 50.0 :
               100.0 * (close - lowest) / (highest - lowest);

    stoch_k_history_.push_back(stoch_k_);
    if (stoch_k_history_.size() > d_period) stoch_k_history_.pop_front();

    // %D = simple average of last d_period %K values
    double sum_k = 0.0;
    for (double k : stoch_k_history_) sum_k += k;
    stoch_d_ = sum_k / stoch_k_history_.size();
}

double Indicators::getStochasticK() const { return stoch_k_; }
double Indicators::getStochasticD() const { return stoch_d_; }

bool Indicators::isStochasticOversold(double threshold) const {
    return stoch_k_ < threshold && stoch_d_ < threshold;
}

bool Indicators::isStochasticOverbought(double threshold) const {
    return stoch_k_ > threshold && stoch_d_ > threshold;
}

// ── WMA ─────────────────────────────────────────────────────────────────────
void Indicators::updateWMA(double price, std::size_t period) {
    auto& window = wma_windows_[period];
    window.push_back(price);
    if (window.size() > period) window.pop_front();
}

double Indicators::getWMA(std::size_t period) const {
    auto it = wma_windows_.find(period);
    if (it == wma_windows_.end() || it->second.size() < period) return 0.0;

    const auto& w = it->second;
    double weighted_sum = 0.0;
    double weight_total = 0.0;
    std::size_t n = w.size();
    for (std::size_t i = 0; i < n; ++i) {
        double wt = static_cast<double>(i + 1);
        weighted_sum += w[i] * wt;
        weight_total += wt;
    }
    return weight_total > 0.0 ? weighted_sum / weight_total : 0.0;
}

// ── OBV ─────────────────────────────────────────────────────────────────────
void Indicators::updateOBV(double price, std::int64_t volume) {
    if (!obv_initialized_) {
        obv_prev_price_ = price;
        obv_initialized_ = true;
        obv_history_.push_back(obv_value_);
        return;
    }

    if (price > obv_prev_price_)       obv_value_ += volume;
    else if (price < obv_prev_price_)  obv_value_ -= volume;

    obv_prev_price_ = price;
    obv_history_.push_back(obv_value_);
    if (obv_history_.size() > 200) obv_history_.pop_front();
}

double Indicators::getOBV() const { return obv_value_; }

double Indicators::getOBVEMA(std::size_t period) const {
    if (obv_history_.size() < period) return obv_value_;
    double ema = obv_history_.front();
    double alpha = 2.0 / (period + 1.0);
    for (double v : obv_history_) {
        ema = alpha * v + (1.0 - alpha) * ema;
    }
    return ema;
}

// ── Williams %R ──────────────────────────────────────────────────────────────
void Indicators::updateWilliamsR(double high, double low, double close, std::size_t period) {
    wr_highs_.push_back(high);
    wr_lows_.push_back(low);
    if (wr_highs_.size() > period) wr_highs_.pop_front();
    if (wr_lows_.size()  > period) wr_lows_.pop_front();

    if (wr_highs_.size() < period) return;

    double highest = *std::max_element(wr_highs_.begin(), wr_highs_.end());
    double lowest  = *std::min_element(wr_lows_.begin(),  wr_lows_.end());

    williams_r_ = (highest == lowest) ? -50.0 :
                  -100.0 * (highest - close) / (highest - lowest);
}

double Indicators::getWilliamsR() const { return williams_r_; }

bool Indicators::isWilliamsROversold(double threshold) const {
    return williams_r_ <= threshold;
}

bool Indicators::isWilliamsROverbought(double threshold) const {
    return williams_r_ >= threshold;
}

// ── Donchian Channel ─────────────────────────────────────────────────────────
void Indicators::updateDonchian(double high, double low, std::size_t period) {
    dc_highs_.push_back(high);
    dc_lows_.push_back(low);
    if (dc_highs_.size() > period) dc_highs_.pop_front();
    if (dc_lows_.size()  > period) dc_lows_.pop_front();

    if (dc_highs_.size() < period) return;

    dc_upper_ = *std::max_element(dc_highs_.begin(), dc_highs_.end());
    dc_lower_ = *std::min_element(dc_lows_.begin(),  dc_lows_.end());
}

double Indicators::getDonchianUpper() const { return dc_upper_; }
double Indicators::getDonchianLower() const { return dc_lower_; }
double Indicators::getDonchianMid()   const { return (dc_upper_ + dc_lower_) / 2.0; }

bool Indicators::isPriceAboveDonchianUpper(double price) const {
    return dc_upper_ > 0.0 && price > dc_upper_;
}

bool Indicators::isPriceBelowDonchianLower(double price) const {
    return dc_lower_ > 0.0 && price < dc_lower_;
}

// ── DEMA ─────────────────────────────────────────────────────────────────────
void Indicators::updateDEMA(double price, std::size_t period) {
    auto it = dema_states_.find(period);
    if (it == dema_states_.end()) {
        DEMAState s;
        s.ema1 = price;
        s.ema2 = price;
        s.alpha = 2.0 / (period + 1.0);
        s.tick_count = 1;
        dema_states_[period] = s;
        return;
    }

    DEMAState& s = it->second;
    s.tick_count++;

    if (s.tick_count <= period) {
        s.ema1 += (price - s.ema1) / s.tick_count;
        s.ema2 += (s.ema1 - s.ema2) / s.tick_count;
    } else {
        s.ema1 = s.alpha * price + (1.0 - s.alpha) * s.ema1;
        s.ema2 = s.alpha * s.ema1 + (1.0 - s.alpha) * s.ema2;
    }
}

double Indicators::getDEMA(std::size_t period) const {
    auto it = dema_states_.find(period);
    if (it == dema_states_.end() || it->second.tick_count < period) return 0.0;
    const DEMAState& s = it->second;
    return 2.0 * s.ema1 - s.ema2;
}

// ── CCI ──────────────────────────────────────────────────────────────────────
void Indicators::updateCCI(double high, double low, double close, std::size_t period) {
    double typical = (high + low + close) / 3.0;

    cci_highs_.push_back(high);
    cci_lows_.push_back(low);
    cci_closes_.push_back(typical);

    if (cci_closes_.size() > period) {
        cci_highs_.pop_front();
        cci_lows_.pop_front();
        cci_closes_.pop_front();
    }

    if (cci_closes_.size() < period) return;

    double mean_tp = std::accumulate(cci_closes_.begin(), cci_closes_.end(), 0.0) / period;

    double mean_dev = 0.0;
    for (double tp : cci_closes_) mean_dev += std::abs(tp - mean_tp);
    mean_dev /= period;

    cci_value_ = (mean_dev == 0.0) ? 0.0 : (typical - mean_tp) / (0.015 * mean_dev);
}

double Indicators::getCCI() const { return cci_value_; }
bool Indicators::isCCIOverbought(double threshold) const { return cci_value_ > threshold; }
bool Indicators::isCCIOversold(double threshold)   const { return cci_value_ < threshold; }

// ── CMF ──────────────────────────────────────────────────────────────────────
void Indicators::updateCMF(double high, double low, double close,
                            std::int64_t volume, std::size_t period) {
    double range = high - low;
    double mfm = (range == 0.0) ? 0.0 : ((close - low) - (high - close)) / range;
    double mfv = mfm * volume;

    cmf_bars_.push_back({mfv, volume});
    if (cmf_bars_.size() > period) cmf_bars_.pop_front();

    if (cmf_bars_.size() < period) return;

    double sum_mfv = 0.0;
    std::int64_t sum_vol = 0;
    for (const auto& b : cmf_bars_) {
        sum_mfv += b.mfv;
        sum_vol += b.volume;
    }
    cmf_value_ = (sum_vol == 0) ? 0.0 : sum_mfv / sum_vol;
}

double Indicators::getCMF() const { return cmf_value_; }

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
    atr_states_.clear();
    bb_price_history_.clear();
    stoch_highs_.clear();
    stoch_lows_.clear();
    stoch_k_history_.clear();
    stoch_k_ = 50.0;
    stoch_d_ = 50.0;
    bb_upper_ = 0.0;
    bb_middle_ = 0.0;
    bb_lower_ = 0.0;
    wma_windows_.clear();
    obv_value_ = 0.0;
    obv_prev_price_ = 0.0;
    obv_initialized_ = false;
    obv_history_.clear();
    wr_highs_.clear();
    wr_lows_.clear();
    williams_r_ = -50.0;
    dc_highs_.clear();
    dc_lows_.clear();
    dc_upper_ = 0.0;
    dc_lower_ = 0.0;
    dema_states_.clear();
    cci_highs_.clear();
    cci_lows_.clear();
    cci_closes_.clear();
    cci_value_ = 0.0;
    cmf_bars_.clear();
    cmf_value_ = 0.0;
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

