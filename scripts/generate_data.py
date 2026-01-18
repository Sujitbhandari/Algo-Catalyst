#!/usr/bin/env python3

import csv
import random
from pathlib import Path

def generate_synthetic_catalyst_data(output_file='data/synthetic_catalyst.csv', num_ticks=10000):
    """
    Generate synthetic tick data that simulates a news catalyst breakout.
    
    Scenario:
    - Ticks 0-1000: Choppy/Flat market (Price ~$10.00)
    - Ticks 1001-5000: The Catalyst - Gap up, volume spike, trending up
    - Ticks 5001-10000: Continue trend with some volatility
    """
    Path('data').mkdir(exist_ok=True)
    
    base_timestamp = 1609459200000000
    timestamps = []
    prices = []
    volumes = []
    bid_sizes = []
    ask_sizes = []
    
    base_price = 10.00
    base_volume = 1000
    normal_bid_size = 5000
    normal_ask_size = 5000
    
    # Phase 1: Choppy/Flat market (Ticks 0-1000)
    print("Generating Phase 1: Choppy market (0-1000 ticks)...")
    current_price = base_price
    for i in range(1000):
        timestamp = base_timestamp + (i * 1000000)
        
        # Slight random walk around $10.00
        price_change = random.uniform(-0.05, 0.05)
        current_price = max(9.90, min(10.10, current_price + price_change))
        
        volume = random.randint(800, 1200)
        bid_size = random.randint(4500, 5500)
        ask_size = random.randint(4500, 5500)
        
        timestamps.append(timestamp)
        prices.append(round(current_price, 2))
        volumes.append(volume)
        bid_sizes.append(bid_size)
        ask_sizes.append(ask_size)
    
    # Phase 2: The Catalyst - Gap up and breakout (Ticks 1001-5000)
    print("Generating Phase 2: News catalyst breakout (1001-5000 ticks)...")
    
    # Calculate average volume for spike calculation
    avg_volume_phase1 = sum(volumes[-20:]) / 20
    
    # Gap up to $11.10+ (ensures >10% gap from previous close ~$10.00)
    previous_close = prices[-1]
    gap_target = previous_close * 1.11  # 11% gap to ensure >10% requirement
    current_price = round(gap_target, 2)
    
    for i in range(1000, 5000):
        timestamp = base_timestamp + (i * 1000000)
        
        if i == 1000:
            # Initial gap up - massive volume spike (10x)
            volume = int(avg_volume_phase1 * 10)
            current_price = 11.00
        elif i < 1500:
            # Early catalyst phase - high volume, trending up
            volume = random.randint(int(avg_volume_phase1 * 8), int(avg_volume_phase1 * 12))
            price_change = random.uniform(0.01, 0.08)
            current_price = min(12.00, current_price + price_change)
        elif i < 3000:
            # Mid-trend - continued momentum
            volume = random.randint(int(avg_volume_phase1 * 5), int(avg_volume_phase1 * 8))
            price_change = random.uniform(0.005, 0.05)
            current_price = min(14.00, current_price + price_change)
        else:
            # Late trend - strong momentum toward $15
            volume = random.randint(int(avg_volume_phase1 * 3), int(avg_volume_phase1 * 6))
            price_change = random.uniform(0.002, 0.03)
            current_price = min(15.00, current_price + price_change)
        
        # Bid/Ask ratio 2.0 (buying pressure)
        bid_size = int(volume * 5)
        ask_size = int(bid_size / 2.0)
        
        timestamps.append(timestamp)
        prices.append(round(current_price, 2))
        volumes.append(volume)
        bid_sizes.append(bid_size)
        ask_sizes.append(ask_size)
    
    # Phase 3: Continue with trend and volatility (Ticks 5001-10000)
    print("Generating Phase 3: Extended trend (5001-10000 ticks)...")
    target_price = 15.50
    
    for i in range(5000, num_ticks):
        timestamp = base_timestamp + (i * 1000000)
        
        # Gradually move toward target with some volatility
        price_diff = target_price - current_price
        price_change = random.uniform(0, price_diff * 0.02) + random.uniform(-0.10, 0.10)
        current_price = max(14.00, min(target_price, current_price + price_change))
        
        # Volume decreases but stays elevated
        volume = random.randint(int(avg_volume_phase1 * 2), int(avg_volume_phase1 * 4))
        
        # Maintain buying pressure but reduce ratio
        bid_size = int(volume * 5)
        ask_size = int(bid_size / 1.8)
        
        timestamps.append(timestamp)
        prices.append(round(current_price, 2))
        volumes.append(volume)
        bid_sizes.append(bid_size)
        ask_sizes.append(ask_size)
    
    # Write to CSV
    print(f"Writing {len(timestamps)} ticks to {output_file}...")
    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Timestamp', 'Price', 'Volume', 'Bid_Size', 'Ask_Size'])
        
        for i in range(len(timestamps)):
            writer.writerow([
                timestamps[i],
                prices[i],
                volumes[i],
                bid_sizes[i],
                ask_sizes[i]
            ])
    
    print(f"Successfully generated {len(timestamps)} ticks!")
    print(f"Price range: ${min(prices):.2f} - ${max(prices):.2f}")
    print(f"Volume range: {min(volumes)} - {max(volumes)}")
    print(f"Gap up at tick 1000: ${prices[999]:.2f} -> ${prices[1000]:.2f} ({(prices[1000]/prices[999]-1)*100:.2f}%)")
    print(f"Volume spike at tick 1000: {volumes[999]} -> {volumes[1000]} ({volumes[1000]/avg_volume_phase1:.2f}x)")

if __name__ == '__main__':
    generate_synthetic_catalyst_data()

