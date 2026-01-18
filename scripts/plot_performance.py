#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
from pathlib import Path

def load_trades(csv_path='trades.csv'):
    try:
        trades = pd.read_csv(csv_path)
        trades['Entry_Time'] = pd.to_numeric(trades['Entry_Time'])
        trades['Exit_Time'] = pd.to_numeric(trades['Exit_Time'])
        return trades
    except FileNotFoundError:
        print(f"Error: {csv_path} not found. Run the backtest first.")
        sys.exit(1)

def load_tick_data(csv_path='data/tick_data.csv'):
    try:
        ticks = pd.read_csv(csv_path)
        ticks['Timestamp'] = pd.to_numeric(ticks['Timestamp'])
        ticks = ticks.sort_values('Timestamp')
        return ticks
    except FileNotFoundError:
        print(f"Warning: {csv_path} not found. Price plot will be limited.")
        return None

def plot_performance(trades_df, ticks_df=None):
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10), sharex=True)
    
    min_time = float('inf')
    max_time = 0
    
    if ticks_df is not None and len(ticks_df) > 0:
        min_time = ticks_df['Timestamp'].min()
        max_time = ticks_df['Timestamp'].max()
        
        ax1.plot(ticks_df['Timestamp'], ticks_df['Price'], 
                'b-', linewidth=0.8, alpha=0.7, label='Price')
    
    if len(trades_df) > 0:
        trade_min = trades_df['Entry_Time'].min()
        trade_max = trades_df['Exit_Time'].max()
        
        if min_time == float('inf'):
            min_time = trade_min
        else:
            min_time = min(min_time, trade_min)
            
        if max_time == 0:
            max_time = trade_max
        else:
            max_time = max(max_time, trade_max)
    
    for idx, trade in trades_df.iterrows():
        entry_time = trade['Entry_Time']
        exit_time = trade['Exit_Time']
        entry_price = trade['Entry_Price']
        exit_price = trade['Exit_Price']
        regime = trade['Regime']
        
        color = 'green' if regime == 'TRENDING' else 'red'
        alpha = 0.15
        
        ax1.axvspan(entry_time, exit_time, alpha=alpha, color=color, 
                   label='TRENDING' if regime == 'TRENDING' and idx == 0 else '')
        ax1.axvspan(entry_time, exit_time, alpha=alpha, color='red' if regime != 'TRENDING' else None,
                   label='CHOPPY' if regime != 'TRENDING' and idx == 0 else '')
        
        ax1.scatter(entry_time, entry_price, color='green', marker='^', 
                   s=100, zorder=5, label='Buy' if idx == 0 else '')
        ax1.scatter(exit_time, exit_price, color='red', marker='v', 
                   s=100, zorder=5, label='Sell' if idx == 0 else '')
        
        ax1.plot([entry_time, exit_time], [entry_price, exit_price], 
                'k--', linewidth=1, alpha=0.3)
    
    ax1.set_ylabel('Price', fontsize=12)
    ax1.set_title('Price Action with Buy/Sell Markers and Regime Detection', fontsize=14, fontweight='bold')
    ax1.legend(loc='upper left', fontsize=9)
    ax1.grid(True, alpha=0.3)
    
    if len(trades_df) > 0:
        trades_sorted = trades_df.sort_values('Entry_Time')
        
        cumulative_pnl = [0]
        timestamps = [min_time]
        
        for idx, trade in trades_sorted.iterrows():
            cumulative_pnl.append(cumulative_pnl[-1] + trade['PnL'])
            timestamps.append(trade['Exit_Time'])
        
        ax2.plot(timestamps, cumulative_pnl, 'g-', linewidth=2, marker='o', markersize=4, label='Cumulative PnL')
        ax2.fill_between(timestamps, 0, cumulative_pnl, alpha=0.3, color='green', where=(np.array(cumulative_pnl) >= 0))
        ax2.fill_between(timestamps, 0, cumulative_pnl, alpha=0.3, color='red', where=(np.array(cumulative_pnl) < 0))
        ax2.axhline(y=0, color='k', linestyle='--', linewidth=1, alpha=0.5)
        
        total_pnl = cumulative_pnl[-1]
        ax2.text(timestamps[-1], total_pnl, f'Total PnL: ${total_pnl:.2f}', 
                verticalalignment='bottom' if total_pnl >= 0 else 'top',
                fontsize=10, fontweight='bold')
    
    ax2.set_xlabel('Timestamp (microseconds)', fontsize=12)
    ax2.set_ylabel('Cumulative PnL ($)', fontsize=12)
    ax2.set_title('Equity Curve', fontsize=14, fontweight='bold')
    ax2.legend(loc='upper left', fontsize=9)
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig

def main():
    if len(sys.argv) > 1:
        trades_csv = sys.argv[1]
    else:
        # Check current directory first, then build directory
        if Path('trades.csv').exists():
            trades_csv = 'trades.csv'
        elif Path('build/trades.csv').exists():
            trades_csv = 'build/trades.csv'
        else:
            trades_csv = 'trades.csv'
    
    if len(sys.argv) > 2:
        ticks_csv = sys.argv[2]
    else:
        # Try synthetic_catalyst.csv first, then default tick_data.csv
        if Path('data/synthetic_catalyst.csv').exists():
            ticks_csv = 'data/synthetic_catalyst.csv'
        else:
            ticks_csv = 'data/tick_data.csv'
    
    print(f"Loading trades from {trades_csv}...")
    trades = load_trades(trades_csv)
    
    print(f"Loading tick data from {ticks_csv}...")
    ticks = load_tick_data(ticks_csv)
    
    if len(trades) == 0:
        print("No trades found. Cannot generate plot.")
        return
    
    print(f"Plotting {len(trades)} trades...")
    fig = plot_performance(trades, ticks)
    
    output_file = 'performance_plot.png'
    output_file_docs = 'docs/images/performance_plot.png'
    
    # Save in current directory
    fig.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Plot saved to {output_file}")
    
    # Also save in docs folder for README
    Path('docs/images').mkdir(parents=True, exist_ok=True)
    fig.savefig(output_file_docs, dpi=300, bbox_inches='tight')
    print(f"Plot also saved to {output_file_docs} for README")
    
    plt.show()

if __name__ == '__main__':
    main()

