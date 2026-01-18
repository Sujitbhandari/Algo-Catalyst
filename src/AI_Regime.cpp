#include "AI_Regime.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <random>

namespace AlgoCatalyst {

RegimeClassifier::RegimeClassifier(std::size_t lookback, std::size_t num_clusters)
    : lookback_(lookback), num_clusters_(num_clusters), 
      current_regime_(Regime::CHOPPY) {
    // Initialize centroids
    centroids_.resize(num_clusters_);
}

RegimeClassifier::Regime RegimeClassifier::updateAndClassify(const Tick& tick) {
    tick_history_.push_back(tick);
    
    // Keep only last 'lookback' ticks
    if (tick_history_.size() > lookback_) {
        tick_history_.pop_front();
    }
    
    // Need minimum ticks to classify
    if (tick_history_.size() < 20) {
        current_regime_ = Regime::CHOPPY;
        return current_regime_;
    }
    
    // Extract features from tick history
    std::vector<Feature> features = extractFeatures(tick_history_);
    
    // Perform k-Means clustering
    performKMeans(features);
    
    // Classify current tick's feature
    Feature current_feature;
    if (tick_history_.size() >= 2) {
        current_feature.volatility = calculateVolatility(tick_history_);
        current_feature.direction = calculateDirection(tick_history_);
        
        // Normalize volume (use recent average)
        std::int64_t sum_vol = 0;
        std::size_t count = 0;
        for (const auto& t : tick_history_) {
            sum_vol += t.volume;
            count++;
        }
        double avg_vol = count > 0 ? static_cast<double>(sum_vol) / count : 1.0;
        current_feature.volume_norm = static_cast<double>(tick.volume) / avg_vol;
    } else {
        current_feature = features.back();
    }
    
    // Assign to nearest centroid
    double min_dist = std::numeric_limits<double>::max();
    std::size_t nearest_cluster = 0;
    
    for (std::size_t i = 0; i < centroids_.size(); ++i) {
        double dist = calculateDistance(current_feature, centroids_[i]);
        if (dist < min_dist) {
            min_dist = dist;
            nearest_cluster = i;
        }
    }
    
    // Map cluster to regime
    // Assume cluster 0 is CHOPPY, cluster 1 is TRENDING
    // Higher volatility and direction -> TRENDING
    if (nearest_cluster == 0) {
        // Check if it's actually trending based on feature values
        if (current_feature.volatility > 0.02 && current_feature.direction > 0.01) {
            current_regime_ = Regime::TRENDING;
        } else {
            current_regime_ = Regime::CHOPPY;
        }
    } else {
        current_regime_ = Regime::TRENDING;
    }
    
    return current_regime_;
}

double RegimeClassifier::getPositionMultiplier() const {
    switch (current_regime_) {
        case Regime::CHOPPY:
            return 0.0;  // Disable trading
        case Regime::TRENDING:
            return 1.5;  // Increase position size by 50%
        default:
            return 1.0;
    }
}

void RegimeClassifier::performKMeans(const std::vector<Feature>& features) {
    if (features.empty()) return;
    
    // Initialize centroids randomly from feature range
    std::vector<double> volatilities, directions, volumes;
    for (const auto& f : features) {
        volatilities.push_back(f.volatility);
        directions.push_back(f.direction);
        volumes.push_back(f.volume_norm);
    }
    
    std::sort(volatilities.begin(), volatilities.end());
    std::sort(directions.begin(), directions.end());
    std::sort(volumes.begin(), volumes.end());
    
    // Initialize centroids at percentiles
    centroids_[0].volatility = volatilities[volatilities.size() / 4];
    centroids_[0].direction = directions[directions.size() / 4];
    centroids_[0].volume_norm = volumes[volumes.size() / 4];
    
    if (num_clusters_ > 1) {
        centroids_[1].volatility = volatilities[3 * volatilities.size() / 4];
        centroids_[1].direction = directions[3 * directions.size() / 4];
        centroids_[1].volume_norm = volumes[3 * volumes.size() / 4];
    }
    
    // k-Means iteration (simplified - 10 iterations max)
    const int max_iterations = 10;
    for (int iter = 0; iter < max_iterations; ++iter) {
        // Assign features to clusters
        std::vector<std::vector<Feature>> clusters(num_clusters_);
        
        for (const auto& feature : features) {
            double min_dist = std::numeric_limits<double>::max();
            std::size_t nearest = 0;
            
            for (std::size_t i = 0; i < centroids_.size(); ++i) {
                double dist = calculateDistance(feature, centroids_[i]);
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest = i;
                }
            }
            
            clusters[nearest].push_back(feature);
        }
        
        // Update centroids
        bool converged = true;
        for (std::size_t i = 0; i < num_clusters_; ++i) {
            if (clusters[i].empty()) continue;
            
            Feature new_centroid = calculateCentroid(clusters[i]);
            
            // Check convergence
            double dist_change = calculateDistance(new_centroid, centroids_[i]);
            if (dist_change > 0.001) {
                converged = false;
            }
            
            centroids_[i] = new_centroid;
        }
        
        if (converged) break;
    }
}

std::vector<RegimeClassifier::Feature> RegimeClassifier::extractFeatures(const std::deque<Tick>& ticks) {
    std::vector<Feature> features;
    
    if (ticks.size() < 2) return features;
    
    // Calculate features for each tick window
    const std::size_t window_size = 10;
    
    for (std::size_t i = window_size; i < ticks.size(); ++i) {
        Feature f;
        
        // Create window
        std::deque<Tick> window(ticks.begin() + (i - window_size), ticks.begin() + i + 1);
        
        f.volatility = calculateVolatility(window);
        f.direction = calculateDirection(window);
        
        // Normalize volume
        std::int64_t sum_vol = 0;
        for (const auto& t : window) {
            sum_vol += t.volume;
        }
        double avg_vol = sum_vol > 0 ? static_cast<double>(sum_vol) / window.size() : 1.0;
        f.volume_norm = static_cast<double>(window.back().volume) / avg_vol;
        
        features.push_back(f);
    }
    
    // If no features extracted, create one from entire history
    if (features.empty() && ticks.size() >= 2) {
        Feature f;
        f.volatility = calculateVolatility(ticks);
        f.direction = calculateDirection(ticks);
        
        std::int64_t sum_vol = 0;
        for (const auto& t : ticks) {
            sum_vol += t.volume;
        }
        double avg_vol = sum_vol > 0 ? static_cast<double>(sum_vol) / ticks.size() : 1.0;
        f.volume_norm = static_cast<double>(ticks.back().volume) / avg_vol;
        
        features.push_back(f);
    }
    
    return features;
}

double RegimeClassifier::calculateDistance(const Feature& a, const Feature& b) {
    double vol_diff = a.volatility - b.volatility;
    double dir_diff = a.direction - b.direction;
    double vol_norm_diff = a.volume_norm - b.volume_norm;
    
    return std::sqrt(vol_diff * vol_diff + dir_diff * dir_diff + vol_norm_diff * vol_norm_diff);
}

RegimeClassifier::Feature RegimeClassifier::calculateCentroid(const std::vector<Feature>& cluster) {
    Feature centroid;
    
    if (cluster.empty()) return centroid;
    
    double sum_vol = 0.0, sum_dir = 0.0, sum_vol_norm = 0.0;
    for (const auto& f : cluster) {
        sum_vol += f.volatility;
        sum_dir += f.direction;
        sum_vol_norm += f.volume_norm;
    }
    
    std::size_t count = cluster.size();
    centroid.volatility = sum_vol / count;
    centroid.direction = sum_dir / count;
    centroid.volume_norm = sum_vol_norm / count;
    
    return centroid;
}

double RegimeClassifier::calculateVolatility(const std::deque<Tick>& ticks) {
    if (ticks.size() < 2) return 0.0;
    
    // Calculate returns
    std::vector<double> returns;
    for (std::size_t i = 1; i < ticks.size(); ++i) {
        if (ticks[i-1].price > 0.0) {
            double ret = (ticks[i].price - ticks[i-1].price) / ticks[i-1].price;
            returns.push_back(ret);
        }
    }
    
    if (returns.empty()) return 0.0;
    
    // Calculate mean return
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double ret : returns) {
        double diff = ret - mean;
        variance += diff * diff;
    }
    variance /= returns.size();
    
    return std::sqrt(variance);
}

double RegimeClassifier::calculateDirection(const std::deque<Tick>& ticks) {
    if (ticks.size() < 2) return 0.0;
    
    // Calculate net directional movement
    double total_move = 0.0;
    for (std::size_t i = 1; i < ticks.size(); ++i) {
        if (ticks[i-1].price > 0.0) {
            double change = (ticks[i].price - ticks[i-1].price) / ticks[i-1].price;
            total_move += change;
        }
    }
    
    return std::abs(total_move) / ticks.size();  // Normalized directional strength
}

} // namespace AlgoCatalyst

