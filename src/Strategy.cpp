#include "Strategy.h"
#include "AI_Regime.h"
#include <algorithm>

namespace AlgoCatalyst {

// Base Strategy Implementation
Strategy::Strategy(const std::string& symbol)
    : symbol_(symbol), position_(0.0), avg_fill_price_(0.0) {
}

// News Momentum Strategy Implementation
NewsMomentumStrategy::NewsMomentumStrategy(const std::string& symbol, RegimeClassifier* regime_classifier)
    : Strategy(symbol), regime_classifier_(regime_classifier),
      was_long_ema_above_short_(false), entry_timestamp_us_(0) {
}

std::vector<EventPtr> NewsMomentumStrategy::processMarketUpdate(const MarketUpdateEvent& event) {
    std::vector<EventPtr> signals;
    
    const Tick& tick = event.getTick();
    std::int64_t timestamp_us = event.getTimestamp();
    
    // Update regime classifier
    if (regime_classifier_) {
        regime_classifier_->updateAndClassify(tick);
    }
    
    // Update indicators
    indicators_.updatePrice(tick.price);
    indicators_.updateEMA(tick.price, 9);
    indicators_.updateEMA(tick.price, 90);
    indicators_.updateEMA(tick.price, 200);
    indicators_.updateMACD(tick.price);
    indicators_.updateVWAP(tick.price, tick.volume, timestamp_us);
    indicators_.updateVolume(tick.volume, timestamp_us);
    
    // Check exit conditions first if in position
    if (hasPosition()) {
        if (checkExitConditions(tick)) {
            SignalEvent::Direction direction = (position_ > 0) ? 
                SignalEvent::Direction::EXIT : SignalEvent::Direction::EXIT;
            
            signals.push_back(std::make_unique<SignalEvent>(
                timestamp_us, symbol_, SignalEvent::Direction::EXIT,
                std::abs(position_), tick.price));
        }
        return signals;
    }
    
    // Check entry conditions only if no position
    if (!hasPosition() && checkEntryConditions(tick)) {
        double position_size = calculatePositionSize();
        
        if (position_size > 0.0) {
            signals.push_back(std::make_unique<SignalEvent>(
                timestamp_us, symbol_, SignalEvent::Direction::LONG,
                position_size, tick.price));
            
            entry_timestamp_us_ = timestamp_us;
            entry_price_ = tick.price;
            highest_price_since_entry_ = tick.price;
        }
    }
    
    return signals;
}

bool NewsMomentumStrategy::checkEntryConditions(const Tick& tick) {
    // Primary trigger: Volume spike + Gap up
    if (!checkVolumeSpike() || !checkGapUp()) {
        return false;
    }
    
    // Trend filters
    if (!checkEMATrend(tick.price)) {
        return false;
    }
    
    // EMA crossover check
    if (!checkEMACrossover()) {
        return false;
    }
    
    // Intraday controls
    if (!checkVWAP(tick.price)) {
        return false;
    }
    
    if (!checkMACD()) {
        return false;
    }
    
    // Microstructure (Level 2)
    if (!checkOrderBookImbalance(tick)) {
        return false;
    }
    
    // Regime check - only trade in TRENDING regime
    if (regime_classifier_ && 
        regime_classifier_->getCurrentRegime() != RegimeClassifier::Regime::TRENDING) {
        return false;
    }
    
    return true;
}

bool NewsMomentumStrategy::checkExitConditions(const Tick& tick) {
    // Update peak price for trailing stop
    if (tick.price > highest_price_since_entry_) {
        highest_price_since_entry_ = tick.price;
    }

    // Hard stop-loss: exit if loss from entry exceeds stop_loss_pct_
    if (entry_price_ > 0.0 && stop_loss_pct_ > 0.0) {
        double loss_pct = ((entry_price_ - tick.price) / entry_price_) * 100.0;
        if (loss_pct >= stop_loss_pct_) {
            return true;
        }
    }

    // Trailing stop: exit if price pulls back trailing_stop_pct_ from peak
    if (trailing_stop_pct_ > 0.0 && highest_price_since_entry_ > 0.0) {
        double pullback_pct = ((highest_price_since_entry_ - tick.price) /
                               highest_price_since_entry_) * 100.0;
        if (pullback_pct >= trailing_stop_pct_) {
            return true;
        }
    }

    // Hard take-profit: exit if gain exceeds take_profit_pct_
    if (entry_price_ > 0.0 && take_profit_pct_ > 0.0) {
        double gain_pct = ((tick.price - entry_price_) / entry_price_) * 100.0;
        if (gain_pct >= take_profit_pct_) {
            return true;
        }
    }

    // Exit if price drops below VWAP
    if (!indicators_.isPriceAboveVWAP(tick.price)) {
        return true;
    }
    
    // Exit if MACD histogram starts contracting and is negative
    if (!indicators_.isMACDHistogramExpanding() && indicators_.getMACDHistogram() < 0) {
        return true;
    }
    
    // Exit if regime switches to CHOPPY
    if (regime_classifier_ && 
        regime_classifier_->getCurrentRegime() == RegimeClassifier::Regime::CHOPPY) {
        return true;
    }
    
    return false;
}

double NewsMomentumStrategy::calculatePositionSize() {
    double base_size = base_position_size_;
    
    // Adjust based on regime
    if (regime_classifier_) {
        double multiplier = regime_classifier_->getPositionMultiplier();
        if (multiplier > 0.0) {
            return base_size * multiplier;
        }
    }
    
    return base_size;
}

bool NewsMomentumStrategy::checkVolumeSpike() {
    double rel_vol = indicators_.getRelativeVolume();
    return rel_vol >= min_relative_volume_;
}

bool NewsMomentumStrategy::checkGapUp() {
    double gap_percent = indicators_.getGapUpPercent();
    return gap_percent >= min_gap_up_percent_;
}

bool NewsMomentumStrategy::checkEMATrend(double price) {
    if (price == 0.0) return false;
    
    // Price must be above both 90-EMA and 200-EMA
    bool above_90 = indicators_.isPriceAboveEMA(price, 90);
    bool above_200 = indicators_.isPriceAboveEMA(price, 200);
    
    // Also check if 90-EMA > 200-EMA (bullish alignment)
    double ema_90 = indicators_.getEMA(90);
    double ema_200 = indicators_.getEMA(200);
    
    return above_90 && above_200 && (ema_90 > ema_200);
}

bool NewsMomentumStrategy::checkEMACrossover() {
    double ema_9 = indicators_.getEMA(9);
    double ema_90 = indicators_.getEMA(90);
    
    if (ema_9 == 0.0 || ema_90 == 0.0) return false;
    
    // Check if 9-EMA crosses above 90-EMA
    bool current_long_above = ema_9 > ema_90;
    
    // Detect crossover (previous state was below, now above)
    bool crossover = current_long_above && !was_long_ema_above_short_;
    
    // Update state
    was_long_ema_above_short_ = current_long_above;
    
    return crossover || current_long_above;  // Allow entry on crossover or if already above
}

bool NewsMomentumStrategy::checkVWAP(double price) {
    if (price == 0.0) return false;
    
    return indicators_.isPriceAboveVWAP(price);
}

bool NewsMomentumStrategy::checkMACD() {
    return indicators_.isMACDHistogramExpanding();
}

bool NewsMomentumStrategy::checkOrderBookImbalance(const Tick& tick) {
    if (tick.ask_size == 0.0) return false;
    
    double bid_ask_ratio = tick.bid_size / tick.ask_size;
    return bid_ask_ratio >= min_bid_ask_ratio_;
}

// Mean Reversion Strategy Implementation
MeanReversionStrategy::MeanReversionStrategy(const std::string& symbol,
                                             RegimeClassifier* regime_classifier)
    : Strategy(symbol), regime_classifier_(regime_classifier) {}

std::vector<EventPtr> MeanReversionStrategy::processMarketUpdate(const MarketUpdateEvent& event) {
    std::vector<EventPtr> signals;
    const Tick& tick = event.getTick();
    std::int64_t timestamp_us = event.getTimestamp();

    if (regime_classifier_) {
        regime_classifier_->updateAndClassify(tick);
    }

    indicators_.updatePrice(tick.price);
    indicators_.updateRSI(tick.price, rsi_period_);
    indicators_.updateBollingerBands(tick.price, bb_period_);
    indicators_.updateEMA(tick.price, 20);
    indicators_.updateVolume(tick.volume, timestamp_us);

    if (hasPosition()) {
        if (checkExitConditions(tick)) {
            signals.push_back(std::make_unique<SignalEvent>(
                timestamp_us, symbol_, SignalEvent::Direction::EXIT,
                std::abs(position_), tick.price));
        }
        return signals;
    }

    if (checkEntryConditions(tick)) {
        signals.push_back(std::make_unique<SignalEvent>(
            timestamp_us, symbol_, SignalEvent::Direction::LONG,
            base_position_size_, tick.price));
        entry_price_ = tick.price;
    }

    return signals;
}

bool MeanReversionStrategy::checkEntryConditions(const Tick& tick) {
    // Enter long when RSI is oversold AND price is below the lower Bollinger Band
    // Only in CHOPPY regime (mean reversion works better there)
    if (regime_classifier_ &&
        regime_classifier_->getCurrentRegime() != RegimeClassifier::Regime::CHOPPY) {
        return false;
    }

    bool rsi_oversold = indicators_.isOversold(rsi_period_, oversold_threshold_);
    bool below_lower_band = indicators_.isPriceBelowLowerBand();

    return rsi_oversold && below_lower_band;
}

bool MeanReversionStrategy::checkExitConditions(const Tick& tick) {
    // Hard stop-loss
    if (entry_price_ > 0.0 && stop_loss_pct_ > 0.0) {
        double loss_pct = ((entry_price_ - tick.price) / entry_price_) * 100.0;
        if (loss_pct >= stop_loss_pct_) return true;
    }

    // Take-profit
    if (entry_price_ > 0.0 && take_profit_pct_ > 0.0) {
        double gain_pct = ((tick.price - entry_price_) / entry_price_) * 100.0;
        if (gain_pct >= take_profit_pct_) return true;
    }

    // Exit when RSI recovers above middle band (mean reversion achieved)
    bool rsi_neutral = !indicators_.isOversold(rsi_period_, oversold_threshold_);
    bool above_middle = tick.price > indicators_.getBollingerMiddle();

    return rsi_neutral && above_middle;
}

} // namespace AlgoCatalyst

