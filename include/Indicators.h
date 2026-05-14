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

    // Stochastic Oscillator %K and %D
    void updateStochastic(double high, double low, double close,
                          std::size_t k_period = 14, std::size_t d_period = 3);
    double getStochasticK() const;
    double getStochasticD() const;
    bool isStochasticOversold(double threshold = 20.0) const;
    bool isStochasticOverbought(double threshold = 80.0) const;

    // WMA (Weighted Moving Average)
    void updateWMA(double price, std::size_t period);
    double getWMA(std::size_t period) const;

    // OBV (On-Balance Volume)
    void updateOBV(double price, std::int64_t volume);
    double getOBV() const;
    double getOBVEMA(std::size_t period = 20) const;

    // Williams %R
    void updateWilliamsR(double high, double low, double close, std::size_t period = 14);
    double getWilliamsR() const;
    bool isWilliamsROversold(double threshold = -80.0) const;
    bool isWilliamsROverbought(double threshold = -20.0) const;

    // Donchian Channel (highest high / lowest low over N periods)
    void updateDonchian(double high, double low, std::size_t period = 20);
    double getDonchianUpper() const;
    double getDonchianLower() const;
    double getDonchianMid() const;
    bool isPriceAboveDonchianUpper(double price) const;
    bool isPriceBelowDonchianLower(double price) const;

    // DEMA (Double Exponential Moving Average)
    void updateDEMA(double price, std::size_t period);
    double getDEMA(std::size_t period) const;

    // CCI (Commodity Channel Index)
    void updateCCI(double high, double low, double close, std::size_t period = 20);
    double getCCI() const;
    bool isCCIOverbought(double threshold = 100.0) const;
    bool isCCIOversold(double threshold = -100.0) const;

    // CMF (Chaikin Money Flow)
    void updateCMF(double high, double low, double close,
                   std::int64_t volume, std::size_t period = 20);
    double getCMF() const;

    // TRIX (Triple Exponential Average – rate of change of triple-smoothed EMA)
    void updateTRIX(double price, std::size_t period = 15);
    double getTRIX() const;
    bool isTRIXBullish() const;  // TRIX > 0 and rising

    // ADX (Average Directional Index) – trend strength 0–100
    void updateADX(double high, double low, double close, std::size_t period = 14);
    double getADX() const;
    double getPlusDI() const;
    double getMinusDI() const;
    bool isTrending(double threshold = 25.0) const;  // ADX > threshold

    // MFI (Money Flow Index) – volume-weighted RSI
    void updateMFI(double high, double low, double close,
                   std::int64_t volume, std::size_t period = 14);
    double getMFI() const;
    bool isMFIOversold(double threshold = 20.0) const;
    bool isMFIOverbought(double threshold = 80.0) const;

    // KAMA (Kaufman Adaptive Moving Average)
    void updateKAMA(double price, std::size_t er_period = 10,
                    double fast_sc = 2.0 / 3.0, double slow_sc = 2.0 / 31.0);
    double getKAMA() const;

    // Pivot Points (classic floor trader: based on prev H/L/C)
    void updatePivotPoints(double prev_high, double prev_low, double prev_close);
    double getPivotPoint() const;
    double getR1() const;
    double getR2() const;
    double getR3() const;
    double getS1() const;
    double getS2() const;
    double getS3() const;

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

    // Stochastic components
    std::deque<double> stoch_highs_;
    std::deque<double> stoch_lows_;
    std::deque<double> stoch_k_history_;
    double stoch_k_ = 50.0;
    double stoch_d_ = 50.0;

    // WMA components: period -> (price window)
    std::map<std::size_t, std::deque<double>> wma_windows_;

    // OBV components
    double obv_value_ = 0.0;
    double obv_prev_price_ = 0.0;
    bool obv_initialized_ = false;
    std::deque<double> obv_history_;

    // Williams %R components
    std::deque<double> wr_highs_;
    std::deque<double> wr_lows_;
    double williams_r_ = -50.0;

    // Donchian Channel components
    std::deque<double> dc_highs_;
    std::deque<double> dc_lows_;
    double dc_upper_ = 0.0;
    double dc_lower_ = 0.0;

    // DEMA components: period -> (ema1, ema2, tick_count)
    struct DEMAState {
        double ema1;
        double ema2;
        double alpha;
        std::size_t tick_count;
    };
    std::map<std::size_t, DEMAState> dema_states_;

    // CCI components
    std::deque<double> cci_highs_;
    std::deque<double> cci_lows_;
    std::deque<double> cci_closes_;
    double cci_value_ = 0.0;

    // CMF components
    struct CMFBar {
        double mfv;
        std::int64_t volume;
    };
    std::deque<CMFBar> cmf_bars_;
    double cmf_value_ = 0.0;

    // TRIX components: period -> triple EMA states
    struct TRIXState {
        double ema1, ema2, ema3;
        double alpha;
        double prev_ema3;
        std::size_t tick_count;
    };
    std::map<std::size_t, TRIXState> trix_states_;
    double trix_value_ = 0.0;

    // ADX components
    struct ADXState {
        double prev_high = 0.0, prev_low = 0.0, prev_close = 0.0;
        double atr_smooth = 0.0;
        double plus_dm_smooth = 0.0;
        double minus_dm_smooth = 0.0;
        double adx_smooth = 0.0;
        double prev_dx = 0.0;
        std::size_t tick_count = 0;
    };
    ADXState adx_state_;
    double adx_value_    = 0.0;
    double plus_di_      = 0.0;
    double minus_di_     = 0.0;

    // MFI components
    struct MFIBar {
        double typical_price;
        double raw_mf;   // positive = up-day, negative = down-day
    };
    std::deque<MFIBar> mfi_bars_;
    double mfi_prev_typical_ = 0.0;
    double mfi_value_        = 50.0;

    // KAMA components
    double kama_value_          = 0.0;
    bool   kama_initialized_    = false;
    std::deque<double> kama_prices_;

    // Pivot Points
    double pivot_  = 0.0;
    double pivot_r1_ = 0.0, pivot_r2_ = 0.0, pivot_r3_ = 0.0;
    double pivot_s1_ = 0.0, pivot_s2_ = 0.0, pivot_s3_ = 0.0;

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

