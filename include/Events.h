#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <variant>
#include <cstdint>

namespace AlgoCatalyst {

// Forward declarations
enum class EventType {
    MarketUpdate,
    SignalEvent,
    OrderEvent,
    FillEvent
};

// Market Tick Data
struct Tick {
    std::int64_t timestamp_us;  // Microseconds since epoch
    double price;
    std::int64_t volume;
    double bid_size;
    double ask_size;
};

// Base Event class
class Event {
public:
    Event(EventType type, std::int64_t timestamp_us)
        : type_(type), timestamp_us_(timestamp_us) {}
    
    virtual ~Event() = default;
    
    EventType getType() const { return type_; }
    std::int64_t getTimestamp() const { return timestamp_us_; }
    
private:
    EventType type_;
    std::int64_t timestamp_us_;
};

// MarketUpdate Event
class MarketUpdateEvent : public Event {
public:
    MarketUpdateEvent(std::int64_t timestamp_us, const Tick& tick)
        : Event(EventType::MarketUpdate, timestamp_us), tick_(tick) {}
    
    const Tick& getTick() const { return tick_; }
    
private:
    Tick tick_;
};

// SignalEvent - Strategy decision
class SignalEvent : public Event {
public:
    enum class Direction {
        LONG,
        SHORT,
        EXIT
    };
    
    SignalEvent(std::int64_t timestamp_us, const std::string& symbol, 
                Direction direction, double quantity, double price)
        : Event(EventType::SignalEvent, timestamp_us),
          symbol_(symbol), direction_(direction), 
          quantity_(quantity), price_(price) {}
    
    std::string getSymbol() const { return symbol_; }
    Direction getDirection() const { return direction_; }
    double getQuantity() const { return quantity_; }
    double getPrice() const { return price_; }
    
private:
    std::string symbol_;
    Direction direction_;
    double quantity_;
    double price_;
};

// OrderEvent - Order submission
class OrderEvent : public Event {
public:
    OrderEvent(std::int64_t timestamp_us, const std::string& symbol,
               SignalEvent::Direction direction, double quantity, double price)
        : Event(EventType::OrderEvent, timestamp_us),
          symbol_(symbol), direction_(direction),
          quantity_(quantity), price_(price) {}
    
    std::string getSymbol() const { return symbol_; }
    SignalEvent::Direction getDirection() const { return direction_; }
    double getQuantity() const { return quantity_; }
    double getPrice() const { return price_; }
    
private:
    std::string symbol_;
    SignalEvent::Direction direction_;
    double quantity_;
    double price_;
};

// FillEvent - Order execution confirmation
class FillEvent : public Event {
public:
    FillEvent(std::int64_t timestamp_us, const std::string& symbol,
              SignalEvent::Direction direction, double quantity, 
              double fill_price, double commission = 0.0)
        : Event(EventType::FillEvent, timestamp_us),
          symbol_(symbol), direction_(direction),
          quantity_(quantity), fill_price_(fill_price), commission_(commission) {}
    
    std::string getSymbol() const { return symbol_; }
    SignalEvent::Direction getDirection() const { return direction_; }
    double getQuantity() const { return quantity_; }
    double getFillPrice() const { return fill_price_; }
    double getCommission() const { return commission_; }
    
private:
    std::string symbol_;
    SignalEvent::Direction direction_;
    double quantity_;
    double fill_price_;
    double commission_;
};

using EventPtr = std::unique_ptr<Event>;

} // namespace AlgoCatalyst

