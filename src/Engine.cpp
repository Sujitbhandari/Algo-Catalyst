#include "Engine.h"
#include "Strategy.h"
#include "AI_Regime.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <ctime>

namespace AlgoCatalyst {

// Backtester Implementation
Backtester::Backtester(double latency_ms)
    : latency_ms_(latency_ms), current_time_us_(0) {
}

bool Backtester::loadTickData(const std::string& csv_path, const std::string& symbol) {
    std::vector<Tick> ticks = TickLoader::loadFromCSV(csv_path);
    
    if (ticks.empty()) {
        return false;
    }
    
    tick_data_[symbol] = std::move(ticks);
    
    // Load all ticks into event queue as MarketUpdateEvents
    for (const auto& tick : tick_data_[symbol]) {
        event_queue_.push(std::make_unique<MarketUpdateEvent>(tick.timestamp_us, tick));
    }
    
    return true;
}

void Backtester::registerStrategy(const std::string& symbol, std::unique_ptr<Strategy> strategy) {
    strategies_[symbol] = std::move(strategy);
}

void Backtester::run() {
    std::cout << "Starting backtest..." << std::endl;
    std::cout << "Latency: " << latency_ms_ << " ms" << std::endl;
    std::cout << "Processing " << event_queue_.size() << " events..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::size_t events_processed = 0;
    
    while (!event_queue_.empty()) {
        EventPtr event = std::move(const_cast<EventPtr&>(event_queue_.top()));
        event_queue_.pop();
        
        current_time_us_ = event->getTimestamp();
        processEvent(std::move(event));
        
        events_processed++;
        
        // Progress indicator every 100k events
        if (events_processed % 100000 == 0) {
            std::cout << "Processed " << events_processed << " events..." << std::endl;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\nBacktest completed!" << std::endl;
    std::cout << "Events processed: " << events_processed << std::endl;
    std::cout << "Processing time: " << duration.count() << " ms" << std::endl;
    
    // Close any remaining positions
    for (auto& [symbol, position] : positions_) {
        if (position.quantity != 0.0 && !tick_data_[symbol].empty()) {
            const Tick& last_tick = tick_data_[symbol].back();
            closePosition(symbol, last_tick.price, last_tick.timestamp_us);
        }
    }
    
    printTradeLog();
}

void Backtester::processEvent(EventPtr event) {
    switch (event->getType()) {
        case EventType::MarketUpdate: {
            auto market_event = std::unique_ptr<MarketUpdateEvent>(
                static_cast<MarketUpdateEvent*>(event.release()));
            processMarketUpdate(std::move(market_event));
            break;
        }
        case EventType::SignalEvent: {
            auto signal_event = std::unique_ptr<SignalEvent>(
                static_cast<SignalEvent*>(event.release()));
            processSignalEvent(std::move(signal_event));
            break;
        }
        case EventType::OrderEvent: {
            auto order_event = std::unique_ptr<OrderEvent>(
                static_cast<OrderEvent*>(event.release()));
            processOrderEvent(std::move(order_event));
            break;
        }
        case EventType::FillEvent: {
            auto fill_event = std::unique_ptr<FillEvent>(
                static_cast<FillEvent*>(event.release()));
            processFillEvent(std::move(fill_event));
            break;
        }
    }
}

void Backtester::processMarketUpdate(std::unique_ptr<MarketUpdateEvent> event) {
    // Find symbol from tick data by matching timestamp range
    std::string event_symbol;
    
    for (const auto& [sym, ticks] : tick_data_) {
        if (ticks.empty()) continue;
        
        // Check if this tick belongs to this symbol's data
        // Match by timestamp (within reasonable range of first tick)
        const Tick& event_tick = event->getTick();
        const Tick& first_tick = ticks[0];
        
        // Simple heuristic: if timestamp matches or is within range, assume same symbol
        // In practice, we'd track symbol with each tick or use a better method
        bool likely_match = (event_tick.timestamp_us >= first_tick.timestamp_us) &&
                           (event_tick.timestamp_us <= (first_tick.timestamp_us + 1000000000LL)); // 1000 seconds
        
        if (likely_match) {
            event_symbol = sym;
            break;
        }
    }
    
    // Default to first symbol if only one exists (common case)
    if (event_symbol.empty() && !tick_data_.empty()) {
        if (tick_data_.size() == 1) {
            event_symbol = tick_data_.begin()->first;
        } else if (!strategies_.empty()) {
            // Use first strategy's symbol
            event_symbol = strategies_.begin()->first;
        }
    }
    
    // Process with strategy
    if (!event_symbol.empty() && strategies_.find(event_symbol) != strategies_.end()) {
        auto signals = strategies_[event_symbol]->processMarketUpdate(*event);
        
        // Add signal events to queue
        for (auto& signal : signals) {
            event_queue_.push(std::move(signal));
        }
    }
}

void Backtester::processSignalEvent(std::unique_ptr<SignalEvent> event) {
    const std::string& symbol = event->getSymbol();
    
    // Create order event
    auto order_event = std::make_unique<OrderEvent>(
        event->getTimestamp(),
        symbol,
        event->getDirection(),
        event->getQuantity(),
        event->getPrice()
    );
    
    // Add order to queue
    event_queue_.push(std::move(order_event));
}

void Backtester::processOrderEvent(std::unique_ptr<OrderEvent> event) {
    const std::string& symbol = event->getSymbol();
    
    // Simulate latency: Fill happens after latency_ms_ milliseconds
    std::int64_t fill_timestamp_us = applyLatency(event->getTimestamp());
    
    // Get current market price (simplified: use order price for now)
    // In a real system, we'd look up the tick at fill_timestamp_us
    double fill_price = event->getPrice();
    
    // Try to get actual price at fill time from tick data
    if (tick_data_.find(symbol) != tick_data_.end()) {
        const auto& ticks = tick_data_[symbol];
        for (const auto& tick : ticks) {
            if (tick.timestamp_us >= fill_timestamp_us) {
                fill_price = tick.price;
                break;
            }
        }
    }
    
    // Calculate commission (simplified: 0.01% per trade)
    double quantity = event->getQuantity();
    double commission = fill_price * quantity * 0.0001;
    
    // Create fill event
    auto fill_event = std::make_unique<FillEvent>(
        fill_timestamp_us,
        symbol,
        event->getDirection(),
        quantity,
        fill_price,
        commission
    );
    
    // Add fill to queue
    event_queue_.push(std::move(fill_event));
}

void Backtester::processFillEvent(std::unique_ptr<FillEvent> event) {
    updatePosition(*event);
}

std::int64_t Backtester::applyLatency(std::int64_t timestamp_us) const {
    // Convert latency_ms_ to microseconds
    std::int64_t latency_us = static_cast<std::int64_t>(latency_ms_ * 1000.0);
    return timestamp_us + latency_us;
}

void Backtester::updatePosition(const FillEvent& fill) {
    const std::string& symbol = fill.getSymbol();
    
    // Get or create position
    if (positions_.find(symbol) == positions_.end()) {
        Position pos;
        pos.symbol = symbol;
        pos.quantity = 0.0;
        pos.avg_price = 0.0;
        pos.direction = SignalEvent::Direction::LONG;
        pos.entry_timestamp_us = 0;
        pos.entry_regime = "UNKNOWN";
        positions_[symbol] = pos;
    }
    
    Position& position = positions_[symbol];
    
    // Handle exit signals
    if (fill.getDirection() == SignalEvent::Direction::EXIT) {
        if (position.quantity != 0.0) {
            closePosition(symbol, fill.getFillPrice(), fill.getTimestamp());
        }
        return;
    }
    
    // Handle entry/position building
    if (fill.getDirection() == SignalEvent::Direction::LONG) {
        if (position.quantity == 0.0) {
            // New position
            position.quantity = fill.getQuantity();
            position.avg_price = fill.getFillPrice();
            position.direction = SignalEvent::Direction::LONG;
            position.entry_timestamp_us = fill.getTimestamp();
            
            // Get current regime from strategy if available
            if (strategies_.find(symbol) != strategies_.end()) {
                // Get regime from strategy's regime classifier
                // This is a simplification - in reality we'd track it better
                position.entry_regime = "TRENDING";  // Simplified
            }
        } else {
            // Add to existing position (average price calculation)
            double total_cost = position.avg_price * position.quantity + 
                              fill.getFillPrice() * fill.getQuantity();
            position.quantity += fill.getQuantity();
            position.avg_price = total_cost / position.quantity;
        }
    }
}

void Backtester::closePosition(const std::string& symbol, double exit_price, std::int64_t timestamp_us) {
    if (positions_.find(symbol) == positions_.end() || 
        positions_[symbol].quantity == 0.0) {
        return;
    }
    
    Position& position = positions_[symbol];
    
    // Calculate PnL
    double pnl = 0.0;
    if (position.direction == SignalEvent::Direction::LONG) {
        pnl = (exit_price - position.avg_price) * position.quantity;
    } else {
        pnl = (position.avg_price - exit_price) * position.quantity;
    }
    
    // Create trade record
    TradeRecord trade;
    trade.entry_timestamp_us = position.entry_timestamp_us;
    trade.exit_timestamp_us = timestamp_us;
    trade.symbol = symbol;
    trade.entry_price = position.avg_price;
    trade.exit_price = exit_price;
    trade.quantity = position.quantity;
    trade.pnl = pnl;
    trade.regime = position.entry_regime;
    
    trade_log_.push_back(trade);
    
    // Reset position
    position.quantity = 0.0;
    position.avg_price = 0.0;
    position.entry_timestamp_us = 0;
}

double Backtester::getTotalPnL() const {
    double total = 0.0;
    for (const auto& trade : trade_log_) {
        total += trade.pnl;
    }
    return total;
}

void Backtester::printTradeLog() const {
    std::cout << "\nTRADE LOG" << std::endl;
    
    if (trade_log_.empty()) {
        std::cout << "No trades executed." << std::endl;
        return;
    }
    
    // Print header
    std::cout << std::left << std::setw(15) << "Symbol"
              << std::setw(20) << "Entry Time"
              << std::setw(20) << "Exit Time"
              << std::setw(12) << "Entry Price"
              << std::setw(12) << "Exit Price"
              << std::setw(10) << "Quantity"
              << std::setw(12) << "PnL"
              << std::setw(10) << "Regime"
              << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------------" << std::endl;
    
    // Print trades
    for (const auto& trade : trade_log_) {
        // Format timestamps (simplified)
        std::string entry_time = std::to_string(trade.entry_timestamp_us);
        std::string exit_time = std::to_string(trade.exit_timestamp_us);
        
        std::cout << std::left << std::setw(15) << trade.symbol
                  << std::setw(20) << entry_time
                  << std::setw(20) << exit_time
                  << std::setw(12) << std::fixed << std::setprecision(2) << trade.entry_price
                  << std::setw(12) << trade.exit_price
                  << std::setw(10) << trade.quantity
                  << std::setw(12) << trade.pnl
                  << std::setw(10) << trade.regime
                  << std::endl;
    }
    
    std::cout << "----------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Total Trades: " << trade_log_.size() << std::endl;
    std::cout << "Total PnL: " << std::fixed << std::setprecision(2) << getTotalPnL() << std::endl;
}

bool Backtester::exportTradeLogToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create file " << filepath << std::endl;
        return false;
    }
    
    file << "Entry_Time,Exit_Time,Symbol,Entry_Price,Exit_Price,Quantity,PnL,Regime\n";
    
    for (const auto& trade : trade_log_) {
        file << trade.entry_timestamp_us << ","
             << trade.exit_timestamp_us << ","
             << trade.symbol << ","
             << std::fixed << std::setprecision(2) << trade.entry_price << ","
             << trade.exit_price << ","
             << trade.quantity << ","
             << trade.pnl << ","
             << trade.regime << "\n";
    }
    
    file.close();
    std::cout << "Exported " << trade_log_.size() << " trades to " << filepath << std::endl;
    return true;
}

// TickLoader Implementation
std::vector<Tick> TickLoader::loadFromCSV(const std::string& filepath) {
    std::vector<Tick> ticks;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filepath << std::endl;
        return ticks;
    }
    
    std::string line;
    bool is_first_line = true;
    
    while (std::getline(file, line)) {
        // Skip header line
        if (is_first_line) {
            is_first_line = false;
            continue;
        }
        
        // Skip empty lines
        if (line.empty()) continue;
        
        std::istringstream ss(line);
        std::string field;
        Tick tick;
        
        // Parse: Timestamp, Price, Volume, Bid_Size, Ask_Size
        std::getline(ss, field, ',');
        tick.timestamp_us = parseTimestamp(field);
        
        std::getline(ss, field, ',');
        tick.price = std::stod(field);
        
        std::getline(ss, field, ',');
        tick.volume = std::stoll(field);
        
        std::getline(ss, field, ',');
        tick.bid_size = std::stod(field);
        
        std::getline(ss, field, ',');
        tick.ask_size = std::stod(field);
        
        ticks.push_back(tick);
    }
    
    file.close();
    std::cout << "Loaded " << ticks.size() << " ticks from " << filepath << std::endl;
    
    return ticks;
}

std::int64_t TickLoader::parseTimestamp(const std::string& ts_str) {
    // Try parsing as microseconds timestamp (integer)
    try {
        return std::stoll(ts_str);
    } catch (...) {
        // If not integer, try parsing as ISO 8601 or other formats
        // Simplified: assume it's microseconds if it's a number
        return 0;
    }
}

} // namespace AlgoCatalyst

