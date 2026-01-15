#!/usr/bin/env python3
"""
Debug script to check actual field values in the HDF5 file
"""

import h5py
import numpy as np

def check_field_values():
    """Check actual field values to debug zero results"""
    h5_file = "output_time_domain/time_domain_results.h5"
    
    try:
        with h5py.File(h5_file, 'r') as f:
            print("🔍 Checking field data values...")
            
            # Check point data
            print("\n📍 Point monitoring data:")
            for i in range(5):
                for component in ['Ex', 'Ey', 'Ez']:
                    key = f'point_{i}_{component}'
                    if key in f:
                        data = f[key][:]
                        print(f"  {key}: shape={data.shape}, range=[{np.min(data):.3e}, {np.max(data):.3e}], mean={np.mean(data):.3e}")
                        # Show first few values
                        print(f"    First 5 values: {data[:5].flatten()}")
            
            # Check plane data
            print("\n🌐 Plane monitoring data:")
            for component in ['Ex', 'Ey', 'Ez']:
                key = f'plane_x0_{component}'
                if key in f:
                    data = f[key][:]
                    print(f"  {key}: shape={data.shape}, range=[{np.min(data):.3e}, {np.max(data):.3e}], mean={np.mean(data):.3e}")
                    # Show center values at different time steps
                    center_x, center_y = data.shape[1]//2, data.shape[2]//2
                    print(f"    Center values at t=0, 25, 50: {data[0, center_x, center_y]:.3e}, {data[25, center_x, center_y]:.3e}, {data[50, center_x, center_y]:.3e}")
            
            # Check volume data
            print("\n📦 Volume monitoring data:")
            for component in ['Ex', 'Ey', 'Ez']:
                key = f'volume_{component}'
                if key in f:
                    data = f[key][:]
                    print(f"  {key}: shape={data.shape}, range=[{np.min(data):.3e}, {np.max(data):.3e}], mean={np.mean(data):.3e}")
                    # Show center values
                    center_x, center_y, center_z = data.shape[1]//2, data.shape[2]//2, data.shape[3]//2
                    print(f"    Center values at t=0, 50: {data[0, center_x, center_y, center_z]:.3e}, {data[50, center_x, center_y, center_z]:.3e}")
            
            # Check time data
            print(f"\n⏰ Time data: shape={f['time'].shape}, range=[{np.min(f['time'][:]):.3e}, {np.max(f['time'][:]):.3e}]")
            print(f"   First 5 time values: {f['time'][:5]}")
            
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    check_field_values()