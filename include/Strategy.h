#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Events.h"
#include "Indicators.h"

namespace AlgoCatalyst {

// Forward declaration
class RegimeClassifier;

// Base Strategy Interface
class Strategy {
public:
    Strategy(const std::string& symbol);
    virtual ~Strategy() = default;
    
    // Process market update and generate signals
    virtual std::vector<EventPtr> processMarketUpdate(const MarketUpdateEvent& event) = 0;
    
    // Get current position state
    bool hasPosition() const { return position_ != 0.0; }
    double getPosition() const { return position_; }
    double getAvgFillPrice() const { return avg_fill_price_; }
    
protected:
    std::string symbol_;
    double position_;
    double avg_fill_price_;
    Indicators indicators_;
};

// News Momentum Strategy Implementation
class NewsMomentumStrategy : public Strategy {
public:
    NewsMomentumStrategy(const std::string& symbol, RegimeClassifier* regime_classifier);
    
    std::vector<EventPtr> processMarketUpdate(const MarketUpdateEvent& event) override;
    
    // Strategy parameters
    void setMinRelativeVolume(double vol) { min_relative_volume_ = vol; }
    void setMinGapUpPercent(double percent) { min_gap_up_percent_ = percent; }
    void setMinBidAskRatio(double ratio) { min_bid_ask_ratio_ = ratio; }
    void setBasePositionSize(double size) { base_position_size_ = size; }
    
private:
    // Check if all entry conditions are met
    bool checkEntryConditions(const Tick& tick);
    
    // Check exit conditions
    bool checkExitConditions(const Tick& tick);
    
    // Calculate position size based on regime
    double calculatePositionSize();
    
    // Entry condition checks
    bool checkVolumeSpike();      // Relative volume > threshold
    bool checkGapUp();             // Gap up > threshold
    bool checkEMATrend(double price);          // Price above 90/200 EMA
    bool checkEMACrossover();      // 9-EMA crosses above 90-EMA
    bool checkVWAP(double price);              // Price above VWAP
    bool checkMACD();              // MACD histogram expanding
    bool checkOrderBookImbalance(const Tick& tick);  // Bid/Ask ratio
    
    RegimeClassifier* regime_classifier_;
    
    // Strategy parameters
    double min_relative_volume_ = 5.0;
    double min_gap_up_percent_ = 10.0;
    double min_bid_ask_ratio_ = 1.5;
    double base_position_size_ = 100.0;
    
    // State tracking
    bool was_long_ema_above_short_ = false;
    std::int64_t entry_timestamp_us_ = 0;
};

} // namespace AlgoCatalyst

