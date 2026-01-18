#include "Engine.h"
#include "Strategy.h"
#include "AI_Regime.h"
#include <iostream>
#include <iomanip>
#include <memory>

using namespace AlgoCatalyst;

int main(int argc, char* argv[]) {
    std::cout << "Algo-Catalyst: High-Performance Trading Engine v1.0" << std::endl;
    std::cout << "Event-Driven Backtesting for News Catalyst Strategies" << std::endl;
    std::cout << std::endl;
    
    std::string csv_file = "data/tick_data.csv";
    if (argc > 1) {
        csv_file = argv[1];
    }
    
    std::string symbol = "TICKER";
    if (argc > 2) {
        symbol = argv[2];
    }
    
    Backtester backtester(200.0);
    
    std::cout << "Loading tick data from: " << csv_file << std::endl;
    if (!backtester.loadTickData(csv_file, symbol)) {
        std::cerr << "Error: Failed to load tick data from " << csv_file << std::endl;
        std::cerr << "Please ensure the CSV file exists with format:" << std::endl;
        std::cerr << "Timestamp,Price,Volume,Bid_Size,Ask_Size" << std::endl;
        return 1;
    }
    
    RegimeClassifier regime_classifier(100, 2);
    
    auto strategy = std::make_unique<NewsMomentumStrategy>(symbol, &regime_classifier);
    
    strategy->setMinRelativeVolume(5.0);
    strategy->setMinGapUpPercent(10.0);
    strategy->setMinBidAskRatio(1.5);
    strategy->setBasePositionSize(100.0);
    
    backtester.registerStrategy(symbol, std::move(strategy));
    
    std::cout << std::endl;
    backtester.run();
    
    std::cout << std::endl;
    std::cout << "PERFORMANCE SUMMARY" << std::endl;
    std::cout << "Total Trades: " << backtester.getNumTrades() << std::endl;
    std::cout << "Total PnL: $" << std::fixed << std::setprecision(2) 
              << backtester.getTotalPnL() << std::endl;
    
    backtester.exportTradeLogToCSV("trades.csv");
    
    return 0;
}

