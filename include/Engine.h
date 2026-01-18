#pragma once

#include <queue>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "Events.h"

namespace AlgoCatalyst {

// Forward declarations
class Strategy;
class TickLoader;

// Trade record for logging
struct TradeRecord {
    std::int64_t entry_timestamp_us;
    std::int64_t exit_timestamp_us;
    std::string symbol;
    double entry_price;
    double exit_price;
    double quantity;
    double pnl;
    std::string regime;
};

// Backtester Engine - Event-Driven Architecture
class Backtester {
public:
    Backtester(double latency_ms = 200.0);
    
    // Load tick data from CSV
    bool loadTickData(const std::string& csv_path, const std::string& symbol);
    
    // Register strategy for a symbol
    void registerStrategy(const std::string& symbol, std::unique_ptr<Strategy> strategy);
    
    // Run backtest
    void run();
    
    // Get trade log
    const std::vector<TradeRecord>& getTradeLog() const { return trade_log_; }
    
    // Performance metrics
    double getTotalPnL() const;
    int getNumTrades() const { return static_cast<int>(trade_log_.size()); }
    
    // Print trade log
    void printTradeLog() const;
    
    // Export trade log to CSV
    bool exportTradeLogToCSV(const std::string& filepath) const;
    
private:
    // Event queue with custom comparator (priority queue)
    struct EventComparator {
        bool operator()(const EventPtr& a, const EventPtr& b) const {
            return a->getTimestamp() > b->getTimestamp();  // Min-heap
        }
    };
    
    using EventQueue = std::priority_queue<EventPtr, std::vector<EventPtr>, EventComparator>;
    
    // Process events
    void processEvent(EventPtr event);
    void processMarketUpdate(std::unique_ptr<MarketUpdateEvent> event);
    void processSignalEvent(std::unique_ptr<SignalEvent> event);
    void processOrderEvent(std::unique_ptr<OrderEvent> event);
    void processFillEvent(std::unique_ptr<FillEvent> event);
    
    // Simulate latency between signal and fill
    std::int64_t applyLatency(std::int64_t timestamp_us) const;
    
    // Track positions and PnL
    void updatePosition(const FillEvent& fill);
    void closePosition(const std::string& symbol, double exit_price, std::int64_t timestamp_us);
    
    EventQueue event_queue_;
    std::map<std::string, std::unique_ptr<Strategy>> strategies_;
    std::map<std::string, std::vector<Tick>> tick_data_;
    
    // Position tracking
    struct Position {
        std::string symbol;
        double quantity;
        double avg_price;
        SignalEvent::Direction direction;
        std::int64_t entry_timestamp_us;
        std::string entry_regime;
    };
    
    std::map<std::string, Position> positions_;
    std::vector<TradeRecord> trade_log_;
    
    double latency_ms_;
    std::int64_t current_time_us_;
};

// CSV Tick Loader
class TickLoader {
public:
    static std::vector<Tick> loadFromCSV(const std::string& filepath);
    
private:
    static std::int64_t parseTimestamp(const std::string& ts_str);
};

} // namespace AlgoCatalyst

