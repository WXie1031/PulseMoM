#!/usr/bin/env python3
"""
Debug the waveform interpolation issue
"""

import numpy as np
import os

# Simulate the waveform loading and interpolation
def debug_waveform():
    print("🔍 Debugging waveform interpolation...")
    
    # Load the waveform file like in the original code
    waveform_file = "hpm_waveform_X(10.0GHz)_20ns.txt"
    
    if os.path.exists(waveform_file):
        data = np.loadtxt(waveform_file)
        time_data = data[:, 0] * 1e-9  # Convert to seconds
        amplitude_data = data[:, 1]
        
        print(f"Waveform file loaded successfully:")
        print(f"  Time range: {time_data[0]*1e9:.3f} to {time_data[-1]*1e9:.3f} ns")
        print(f"  Amplitude range: {np.min(amplitude_data):.3e} to {np.max(amplitude_data):.3e}")
        print(f"  Total data points: {len(time_data)}")
        
        # Test interpolation at various time points
        test_times = np.array([0, 1e-9, 5e-9, 10e-9, 15e-9, 20e-9])  # 0 to 20 ns
        
        print(f"\n🧪 Testing interpolation:")
        for t in test_times:
            # This is the same interpolation as in the original code
            amplitude = np.interp(t, time_data, amplitude_data)
            print(f"  t={t*1e9:5.1f} ns: amplitude={amplitude:.3e}")
            
        # Check if there are any issues with the data
        print(f"\n📊 Data quality check:")
        print(f"  Time data is monotonic: {np.all(np.diff(time_data) > 0)}")
        print(f"  Any NaN in time data: {np.any(np.isnan(time_data))}")
        print(f"  Any NaN in amplitude data: {np.any(np.isnan(amplitude_data))}")
        print(f"  Any infinite values: {np.any(np.isinf(amplitude_data))}")
        
        # Check the range of interpolation
        print(f"\n📈 Interpolation bounds:")
        print(f"  Requested time range: 0 to 20 ns")
        print(f"  Available time range: {time_data[0]*1e9:.3f} to {time_data[-1]*1e9:.3f} ns")
        print(f"  Interpolation will extrapolate: {time_data[0]*1e9 > 0 or time_data[-1]*1e9 < 20}")
        
    else:
        print(f"❌ Waveform file not found: {waveform_file}")

if __name__ == "__main__":
    debug_waveform()