#include "Engine.h"
#include "Strategy.h"
#include "AI_Regime.h"
#include "Version.h"
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
    std::cout << "Starting backtest...\n"
              << "Latency:    " << latency_ms_ << " ms\n"
              << "Slippage:   " << slippage_bps_ << " bps\n"
              << "Commission: $" << commission_per_share_ << "/share (min $" << min_commission_ << ")\n"
              << "Processing " << event_queue_.size() << " events...\n";
    
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
    
    // Process with strategy (skip if risk circuit breaker is active)
    if (!event_symbol.empty() && strategies_.find(event_symbol) != strategies_.end() && !risk_halt_) {
        auto signals = strategies_[event_symbol]->processMarketUpdate(*event);

        // Update MAE/MFE for open positions on every tick
        if (positions_.find(event_symbol) != positions_.end()) {
            Position& pos = positions_[event_symbol];
            if (pos.quantity != 0.0 && pos.avg_price > 0.0) {
                const Tick& t = event->getTick();
                double unrealized = (t.price - pos.avg_price) * pos.quantity;
                pos.mfe = std::max(pos.mfe, unrealized);
                pos.mae = std::min(pos.mae, unrealized);
            }
        }

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
    
    // Get market price at fill time
    double fill_price = event->getPrice();
    
    if (tick_data_.find(symbol) != tick_data_.end()) {
        const auto& ticks = tick_data_[symbol];
        for (const auto& tick : ticks) {
            if (tick.timestamp_us >= fill_timestamp_us) {
                fill_price = tick.price;
                break;
            }
        }
    }
    
    // Apply slippage: buys pay more, sells receive less
    double slippage_factor = slippage_bps_ / 10000.0;
    if (event->getDirection() == SignalEvent::Direction::LONG) {
        fill_price *= (1.0 + slippage_factor);
    } else if (event->getDirection() == SignalEvent::Direction::EXIT) {
        fill_price *= (1.0 - slippage_factor);
    }
    
    // Per-share commission with minimum
    double quantity = event->getQuantity();
    double commission = std::max(quantity * commission_per_share_, min_commission_);
    
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
        pos.total_commission = 0.0;
        pos.direction = SignalEvent::Direction::LONG;
        pos.entry_timestamp_us = 0;
        pos.entry_regime = "UNKNOWN";
        pos.strategy_name = "Unknown";
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
            position.quantity = fill.getQuantity();
            position.avg_price = fill.getFillPrice();
            position.total_commission = fill.getCommission();
            position.direction = SignalEvent::Direction::LONG;
            position.entry_timestamp_us = fill.getTimestamp();

            // Resolve actual regime string from the registered strategy
            position.entry_regime = "UNKNOWN";
            if (strategies_.find(symbol) != strategies_.end()) {
                // Use the fill symbol to fetch regime from AI classifier via strategy name
                // For now record the strategy type as a proxy
                position.strategy_name = "NewsMomentum";
                position.entry_regime = "TRENDING";  // Strategy only fires in TRENDING
            }
        } else {
            double total_cost = position.avg_price * position.quantity +
                              fill.getFillPrice() * fill.getQuantity();
            position.quantity += fill.getQuantity();
            position.avg_price = total_cost / position.quantity;
            position.total_commission += fill.getCommission();
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
    
    TradeRecord trade;
    trade.entry_timestamp_us = position.entry_timestamp_us;
    trade.exit_timestamp_us = timestamp_us;
    trade.symbol = symbol;
    trade.entry_price = position.avg_price;
    trade.exit_price = exit_price;
    trade.quantity = position.quantity;
    trade.pnl = pnl - position.total_commission;  // Net of commissions
    trade.commission = position.total_commission;
    trade.regime = position.entry_regime;
    trade.strategy_name = position.strategy_name;
    trade.mae = position.mae;
    trade.mfe = position.mfe;
    
    trade_log_.push_back(trade);

    // Risk circuit breaker checks
    double current_equity = getTotalPnL();
    if (current_equity > peak_equity_) peak_equity_ = current_equity;
    daily_pnl_ += trade.pnl;

    if (max_drawdown_limit_ > 0.0 && (peak_equity_ - current_equity) >= max_drawdown_limit_) {
        std::cerr << "[RISK] Max drawdown limit $" << max_drawdown_limit_
                  << " reached — halting new entries.\n";
        risk_halt_ = true;
    }
    if (max_daily_loss_ > 0.0 && daily_pnl_ <= -max_daily_loss_) {
        std::cerr << "[RISK] Daily loss limit $" << max_daily_loss_
                  << " reached — halting new entries.\n";
        risk_halt_ = true;
    }

    position.quantity = 0.0;
    position.avg_price = 0.0;
    position.total_commission = 0.0;
    position.entry_timestamp_us = 0;
}

double Backtester::getTotalPnL() const {
    double total = 0.0;
    for (const auto& trade : trade_log_) {
        total += trade.pnl;
    }
    return total;
}

double Backtester::getWinRate() const {
    if (trade_log_.empty()) return 0.0;
    int wins = 0;
    for (const auto& trade : trade_log_) {
        if (trade.pnl > 0.0) wins++;
    }
    return static_cast<double>(wins) / trade_log_.size() * 100.0;
}

double Backtester::getAverageWin() const {
    double total = 0.0;
    int count = 0;
    for (const auto& trade : trade_log_) {
        if (trade.pnl > 0.0) { total += trade.pnl; count++; }
    }
    return count > 0 ? total / count : 0.0;
}

double Backtester::getAverageLoss() const {
    double total = 0.0;
    int count = 0;
    for (const auto& trade : trade_log_) {
        if (trade.pnl < 0.0) { total += trade.pnl; count++; }
    }
    return count > 0 ? total / count : 0.0;
}

double Backtester::getProfitFactor() const {
    double gross_profit = 0.0;
    double gross_loss = 0.0;
    for (const auto& trade : trade_log_) {
        if (trade.pnl > 0.0) gross_profit += trade.pnl;
        else gross_loss += std::abs(trade.pnl);
    }
    return gross_loss > 0.0 ? gross_profit / gross_loss : 0.0;
}

double Backtester::getSharpeRatio(double risk_free_rate) const {
    if (trade_log_.size() < 2) return 0.0;

    std::vector<double> returns;
    returns.reserve(trade_log_.size());
    for (const auto& trade : trade_log_) {
        returns.push_back(trade.pnl);
    }

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;
    for (double r : returns) {
        double diff = r - mean;
        variance += diff * diff;
    }
    variance /= (returns.size() - 1);
    double std_dev = std::sqrt(variance);

    if (std_dev == 0.0) return 0.0;
    return (mean - risk_free_rate) / std_dev;
}

double Backtester::getMaxDrawdown() const {
    if (trade_log_.empty()) return 0.0;

    double peak = 0.0;
    double equity = 0.0;
    double max_dd = 0.0;

    for (const auto& trade : trade_log_) {
        equity += trade.pnl;
        if (equity > peak) peak = equity;
        double drawdown = peak - equity;
        if (drawdown > max_dd) max_dd = drawdown;
    }
    return max_dd;
}

void Backtester::printPerformanceSummary() const {
    std::cout << "\n========== PERFORMANCE SUMMARY ==========\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Trades:    " << getNumTrades() << "\n";
    std::cout << "Total PnL:       $" << getTotalPnL() << "\n";
    std::cout << "Win Rate:        " << getWinRate() << "%\n";
    std::cout << "Profit Factor:   " << getProfitFactor() << "\n";
    std::cout << "Avg Win:         $" << getAverageWin() << "\n";
    std::cout << "Avg Loss:        $" << getAverageLoss() << "\n";
    std::cout << "Max Drawdown:    $" << getMaxDrawdown() << "\n";
    std::cout << "Sharpe Ratio:    " << getSharpeRatio() << "\n";
    std::cout << "=========================================\n";
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
    
    auto format_ts = [](std::int64_t ts_us) -> std::string {
        std::time_t t = static_cast<std::time_t>(ts_us / 1'000'000LL);
        int frac_us = static_cast<int>(ts_us % 1'000'000LL);
        std::tm* tm_info = std::localtime(&t);
        if (!tm_info) return std::to_string(ts_us);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", tm_info);
        char result[48];
        std::snprintf(result, sizeof(result), "%s.%06d", buf, frac_us);
        return std::string(result);
    };

    // Print trades
    for (const auto& trade : trade_log_) {
        std::string entry_time = format_ts(trade.entry_timestamp_us);
        std::string exit_time  = format_ts(trade.exit_timestamp_us);
        
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
    
    file << "Entry_Time_US,Exit_Time_US,Symbol,Entry_Price,Exit_Price,"
         << "Quantity,PnL,Commission,Regime,Strategy,MAE,MFE\n";

    for (const auto& trade : trade_log_) {
        file << trade.entry_timestamp_us << ","
             << trade.exit_timestamp_us << ","
             << trade.symbol << ","
             << std::fixed << std::setprecision(4) << trade.entry_price << ","
             << trade.exit_price << ","
             << trade.quantity << ","
             << std::setprecision(2) << trade.pnl << ","
             << trade.commission << ","
             << trade.regime << ","
             << trade.strategy_name << ","
             << trade.mae << ","
             << trade.mfe << "\n";
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
    std::size_t line_num = 0;
    std::int64_t prev_timestamp = 0;
    std::size_t skipped = 0;

    while (std::getline(file, line)) {
        ++line_num;

        if (is_first_line) {
            is_first_line = false;
            continue;
        }

        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        // Validate column count (at least 5 required)
        if (fields.size() < 5) {
            std::cerr << "[WARN] Line " << line_num << ": expected 5 columns, got "
                      << fields.size() << " — skipping\n";
            ++skipped;
            continue;
        }

        Tick tick;
        try {
            tick.timestamp_us = parseTimestamp(fields[0]);
            tick.price        = std::stod(fields[1]);
            tick.volume       = std::stoll(fields[2]);
            tick.bid_size     = std::stod(fields[3]);
            tick.ask_size     = std::stod(fields[4]);
            tick.high         = (fields.size() > 5) ? std::stod(fields[5]) : tick.price;
            tick.low          = (fields.size() > 6) ? std::stod(fields[6]) : tick.price;
        } catch (const std::exception& e) {
            std::cerr << "[WARN] Line " << line_num << ": parse error (" << e.what()
                      << ") — skipping\n";
            ++skipped;
            continue;
        }

        // Data sanity: price and volume must be positive
        if (tick.price <= 0.0 || tick.volume < 0) {
            std::cerr << "[WARN] Line " << line_num << ": invalid price/volume — skipping\n";
            ++skipped;
            continue;
        }

        // Warn on non-monotonic timestamps but keep the tick
        if (prev_timestamp > 0 && tick.timestamp_us < prev_timestamp) {
            std::cerr << "[WARN] Line " << line_num << ": non-monotonic timestamp\n";
        }
        prev_timestamp = tick.timestamp_us;

        ticks.push_back(tick);
    }

    if (skipped > 0) {
        std::cerr << "[INFO] Skipped " << skipped << " malformed rows.\n";
    }
    
    file.close();
    std::cout << "Loaded " << ticks.size() << " ticks from " << filepath << std::endl;
    
    return ticks;
}

std::int64_t TickLoader::parseTimestamp(const std::string& ts_str) {
    if (ts_str.empty()) return 0;

    // Try integer microseconds first (fastest path)
    try {
        bool all_digits = true;
        for (char c : ts_str) {
            if (!std::isdigit(c)) { all_digits = false; break; }
        }
        if (all_digits) return std::stoll(ts_str);
    } catch (...) {}

    // Try ISO 8601 format: YYYY-MM-DDTHH:MM:SS[.ffffff][Z]
    // e.g., "2024-01-15T09:30:00.123456"
    std::tm tm = {};
    int microseconds = 0;
    const char* fmt_full  = "%Y-%m-%dT%H:%M:%S";
    const char* fmt_space = "%Y-%m-%d %H:%M:%S";

    char* parsed = nullptr;
    for (const char* fmt : {fmt_full, fmt_space}) {
        std::string stripped = ts_str;
        // strip trailing Z
        if (!stripped.empty() && stripped.back() == 'Z') stripped.pop_back();

        parsed = strptime(stripped.c_str(), fmt, &tm);
        if (parsed) {
            // parse optional fractional seconds
            if (*parsed == '.' || *parsed == ',') {
                ++parsed;
                int frac = 0, digits = 0;
                while (*parsed && std::isdigit(*parsed) && digits < 6) {
                    frac = frac * 10 + (*parsed - '0');
                    ++parsed;
                    ++digits;
                }
                // pad to 6 digits
                while (digits++ < 6) frac *= 10;
                microseconds = frac;
            }
            break;
        }
    }

    if (!parsed) return 0;

    tm.tm_isdst = -1;
    std::time_t epoch_sec = std::mktime(&tm);
    if (epoch_sec == -1) return 0;

    return static_cast<std::int64_t>(epoch_sec) * 1'000'000LL + microseconds;
}

} // namespace AlgoCatalyst

