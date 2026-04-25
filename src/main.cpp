#include "Engine.h"
#include "Strategy.h"
#include "AI_Regime.h"
#include "PerformanceAnalyzer.h"
#include "ConfigLoader.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <cstring>

using namespace AlgoCatalyst;

static void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --config <path>     JSON config file (overridden by CLI flags)\n"
              << "  --data <path>       Path to tick CSV file (default: data/tick_data.csv)\n"
              << "  --symbol <sym>      Ticker symbol name (default: TICKER)\n"
              << "  --latency <ms>      Simulated fill latency in ms (default: 200)\n"
              << "  --output <path>     Trade log CSV output path (default: trades.csv)\n"
              << "  --stop-loss <pct>   Hard stop-loss percent (default: 2.0)\n"
              << "  --take-profit <pct> Take-profit percent (default: 6.0)\n"
              << "  --trailing <pct>    Trailing stop percent (default: 3.0)\n"
              << "  --slippage <bps>    Slippage in basis points (default: 5)\n"
              << "  --strategy <name>   Strategy: momentum|meanrev|breakout (default: momentum)\n"
              << "  --help              Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string csv_file = "data/tick_data.csv";
    std::string symbol = "TICKER";
    std::string output_file = "trades.csv";
    std::string strategy_name = "momentum";
    std::string config_file;
    double latency_ms = 200.0;
    double stop_loss_pct = 2.0;
    double take_profit_pct = 6.0;
    double trailing_stop_pct = 3.0;
    double slippage_bps = 5.0;

    // Pre-scan for --config so it loads before other flags
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        }
    }
    if (!config_file.empty()) {
        try {
            AlgoCatalyst::ConfigLoader cfg = AlgoCatalyst::ConfigLoader::fromFile(config_file);
            csv_file         = cfg.getString("data",              csv_file);
            symbol           = cfg.getString("symbol",            symbol);
            output_file      = cfg.getString("output",            output_file);
            strategy_name    = cfg.getString("strategy",          strategy_name);
            latency_ms       = cfg.getDouble("latency_ms",        latency_ms);
            stop_loss_pct    = cfg.getDouble("stop_loss_pct",     stop_loss_pct);
            take_profit_pct  = cfg.getDouble("take_profit_pct",   take_profit_pct);
            trailing_stop_pct= cfg.getDouble("trailing_stop_pct", trailing_stop_pct);
            slippage_bps     = cfg.getDouble("slippage_bps",      slippage_bps);
            std::cout << "Loaded config from: " << config_file << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[WARN] Could not load config: " << e.what() << "\n";
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            ++i; // already processed above
        } else if (std::strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            csv_file = argv[++i];
        } else if (std::strcmp(argv[i], "--symbol") == 0 && i + 1 < argc) {
            symbol = argv[++i];
        } else if (std::strcmp(argv[i], "--latency") == 0 && i + 1 < argc) {
            latency_ms = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (std::strcmp(argv[i], "--stop-loss") == 0 && i + 1 < argc) {
            stop_loss_pct = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--take-profit") == 0 && i + 1 < argc) {
            take_profit_pct = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--trailing") == 0 && i + 1 < argc) {
            trailing_stop_pct = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--slippage") == 0 && i + 1 < argc) {
            slippage_bps = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--strategy") == 0 && i + 1 < argc) {
            strategy_name = argv[++i];
        } else {
            // Legacy positional argument support
            if (i == 1) csv_file = argv[i];
            else if (i == 2) symbol = argv[i];
        }
    }

    std::cout << "Algo-Catalyst: High-Performance Trading Engine v"
              << ALGOCATALYST_VERSION_MAJOR << "."
              << ALGOCATALYST_VERSION_MINOR << "."
              << ALGOCATALYST_VERSION_PATCH << "\n"
              << "Event-Driven Backtesting for News Catalyst Strategies\n\n";

    Backtester backtester(latency_ms);
    backtester.setSlippageBps(slippage_bps);

    std::cout << "Loading tick data from: " << csv_file << "\n";
    if (!backtester.loadTickData(csv_file, symbol)) {
        std::cerr << "Error: Failed to load tick data from " << csv_file << "\n"
                  << "Please ensure the CSV file exists with format:\n"
                  << "Timestamp,Price,Volume,Bid_Size,Ask_Size\n";
        return 1;
    }

    RegimeClassifier regime_classifier(100, 2);

    std::unique_ptr<Strategy> strategy;
    if (strategy_name == "breakout") {
        auto s = std::make_unique<BreakoutStrategy>(symbol, &regime_classifier);
        s->setBasePositionSize(100.0);
        s->setStopLossPercent(stop_loss_pct);
        s->setTakeProfitPercent(take_profit_pct);
        s->setTrailingStopPercent(trailing_stop_pct);
        strategy = std::move(s);
    } else if (strategy_name == "meanrev") {
        auto s = std::make_unique<MeanReversionStrategy>(symbol, &regime_classifier);
        s->setBasePositionSize(100.0);
        s->setStopLossPercent(stop_loss_pct);
        s->setTakeProfitPercent(take_profit_pct);
        strategy = std::move(s);
    } else {
        auto s = std::make_unique<NewsMomentumStrategy>(symbol, &regime_classifier);
        s->setMinRelativeVolume(5.0);
        s->setMinGapUpPercent(10.0);
        s->setMinBidAskRatio(1.5);
        s->setBasePositionSize(100.0);
        s->setStopLossPercent(stop_loss_pct);
        s->setTakeProfitPercent(take_profit_pct);
        s->setTrailingStopPercent(trailing_stop_pct);
        strategy = std::move(s);
    }

    backtester.registerStrategy(symbol, std::move(strategy));

    std::cout << "\n";
    backtester.run();

    // Print detailed analytics via PerformanceAnalyzer
    auto metrics = PerformanceAnalyzer::compute(backtester.getTradeLog());
    PerformanceAnalyzer::print(metrics);

    backtester.exportTradeLogToCSV(output_file);

    return 0;
}
