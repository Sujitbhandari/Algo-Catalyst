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
    void setStopLossPercent(double pct) { stop_loss_pct_ = pct; }
    void setTakeProfitPercent(double pct) { take_profit_pct_ = pct; }
    void setTrailingStopPercent(double pct) { trailing_stop_pct_ = pct; }
    
private:
    // Check if all entry conditions are met
    bool checkEntryConditions(const Tick& tick);
    
    // Check exit conditions
    bool checkExitConditions(const Tick& tick);
    
    // Calculate position size (Kelly-informed, regime-adjusted)
    double calculatePositionSize();
    
    // Update win/loss stats for Kelly fraction
    void recordTrade(double pnl);
    
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
    double stop_loss_pct_ = 2.0;    // 2% hard stop loss
    double take_profit_pct_ = 6.0;  // 6% take profit (3:1 R/R)
    double trailing_stop_pct_ = 3.0; // 3% trailing stop from peak

    // State tracking
    bool was_long_ema_above_short_ = false;
    std::int64_t entry_timestamp_us_ = 0;
    double entry_price_ = 0.0;
    double highest_price_since_entry_ = 0.0;  // For trailing stop

    // Kelly criterion tracking
    int trades_won_   = 0;
    int trades_total_ = 0;
    double avg_win_  = 0.0;
    double avg_loss_ = 0.0;
};

// Mean Reversion Strategy - trades Bollinger Band breakouts and RSI extremes
class MeanReversionStrategy : public Strategy {
public:
    MeanReversionStrategy(const std::string& symbol, RegimeClassifier* regime_classifier);

    std::vector<EventPtr> processMarketUpdate(const MarketUpdateEvent& event) override;

    void setRSIPeriod(std::size_t period) { rsi_period_ = period; }
    void setOversoldThreshold(double threshold) { oversold_threshold_ = threshold; }
    void setOverboughtThreshold(double threshold) { overbought_threshold_ = threshold; }
    void setBBPeriod(std::size_t period) { bb_period_ = period; }
    void setBasePositionSize(double size) { base_position_size_ = size; }
    void setStopLossPercent(double pct) { stop_loss_pct_ = pct; }
    void setTakeProfitPercent(double pct) { take_profit_pct_ = pct; }

private:
    bool checkEntryConditions(const Tick& tick);
    bool checkExitConditions(const Tick& tick);

    RegimeClassifier* regime_classifier_;

    std::size_t rsi_period_ = 14;
    double oversold_threshold_ = 30.0;
    double overbought_threshold_ = 70.0;
    std::size_t bb_period_ = 20;
    double base_position_size_ = 100.0;
    double stop_loss_pct_ = 1.5;
    double take_profit_pct_ = 3.0;

    double entry_price_ = 0.0;
};

} // namespace AlgoCatalyst

