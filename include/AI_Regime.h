#pragma once

#include <vector>
#include <deque>
#include <cstdint>
#include "Events.h"

namespace AlgoCatalyst {

// Regime Classifier using k-Means Clustering
class RegimeClassifier {
public:
    enum class Regime {
        CHOPPY = 0,   // Low volatility, mean-reverting
        TRENDING = 1  // High directed volatility
    };
    
    RegimeClassifier(std::size_t lookback = 100, std::size_t num_clusters = 2);
    
    // Update with new tick and classify regime
    Regime updateAndClassify(const Tick& tick);
    
    // Get current regime
    Regime getCurrentRegime() const { return current_regime_; }
    
    // Get position multiplier based on regime
    double getPositionMultiplier() const;
    
private:
    // Feature vector for clustering (volatility, direction, volume)
    struct Feature {
        double volatility;
        double direction;
        double volume_norm;
    };
    
    // k-Means clustering implementation
    void performKMeans(const std::vector<Feature>& features);
    std::vector<Feature> extractFeatures(const std::deque<Tick>& ticks);
    double calculateDistance(const Feature& a, const Feature& b);
    Feature calculateCentroid(const std::vector<Feature>& cluster);
    
    std::deque<Tick> tick_history_;
    std::size_t lookback_;
    std::size_t num_clusters_;
    Regime current_regime_;
    
    // k-Means centroids
    std::vector<Feature> centroids_;
    
    // Calculate volatility (standard deviation of returns)
    double calculateVolatility(const std::deque<Tick>& ticks);
    
    // Calculate directional movement
    double calculateDirection(const std::deque<Tick>& ticks);
};

} // namespace AlgoCatalyst

