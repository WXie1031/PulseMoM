#!/usr/bin/env python3
"""
Debug the data keys in results
"""

import h5py
import numpy as np

def debug_data_keys():
    """Check what data keys are available"""
    h5_file = "output_time_domain/time_domain_results.h5"
    
    try:
        with h5py.File(h5_file, 'r') as f:
            print("🔍 Checking data keys in results...")
            
            # List all top-level datasets
            print("Top-level datasets:")
            for key in f.keys():
                obj = f[key]
                if isinstance(obj, h5py.Dataset):
                    print(f"  {key}: shape={obj.shape}")
                else:
                    print(f"  {key}: group")
                    
            # Check what's in the results dictionary
            print(f"\nCoordinate group contents:")
            if 'coordinates' in f:
                coords = f['coordinates']
                for key in coords.keys():
                    obj = coords[key]
                    print(f"  coordinates/{key}: shape={obj.shape}")
                    
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    debug_data_keys()