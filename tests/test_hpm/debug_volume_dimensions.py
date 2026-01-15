#!/usr/bin/env python3
"""
Debug the volume data dimensions
"""

import h5py
import numpy as np

def debug_volume_dimensions():
    """Check volume data dimensions"""
    h5_file = "output_time_domain/time_domain_results.h5"
    
    try:
        with h5py.File(h5_file, 'r') as f:
            print("🔍 Checking volume data dimensions...")
            
            # Check volume data
            for component in ['Ex', 'Ey', 'Ez']:
                key = f'volume_{component}'
                if key in f:
                    data = f[key][:]
                    print(f"  {key}: shape={data.shape}")
                    print(f"    Time dimension: {data.shape[0]} (should be {len(f['time'][:])})")
                    
            # Check time data
            time_data = f['time'][:]
            print(f"  Time data: shape={time_data.shape}, range=[{time_data[0]*1e9:.3f}, {time_data[-1]*1e9:.3f}] ns")
            
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    debug_volume_dimensions()