#!/usr/bin/env python3
"""
Debug the waveform time scale issue
"""

import numpy as np
import os

# Simulate the waveform loading and interpolation
def debug_waveform_timescale():
    print("🔍 Debugging waveform time scale...")
    
    # Load the waveform file like in the original code
    waveform_file = "hpm_waveform_X(10.0GHz)_20ns.txt"
    
    if os.path.exists(waveform_file):
        data = np.loadtxt(waveform_file)
        time_data_raw = data[:, 0]  # Raw time values
        amplitude_data = data[:, 1]
        
        print(f"Raw waveform data:")
        print(f"  First 10 time values: {time_data_raw[:10]}")
        print(f"  Last 10 time values: {time_data_raw[-10:]}")
        print(f"  Time range: {time_data_raw[0]:.6e} to {time_data_raw[-1]:.6e}")
        print(f"  Amplitude range: {np.min(amplitude_data):.3e} to {np.max(amplitude_data):.3e}")
        
        # Check what happens when we multiply by 1e-9
        time_data_seconds = time_data_raw * 1e-9
        print(f"\nAfter conversion to seconds:")
        print(f"  Time range: {time_data_seconds[0]:.6e} to {time_data_seconds[-1]:.6e}")
        print(f"  This equals: {time_data_seconds[0]*1e9:.3f} to {time_data_seconds[-1]*1e9:.3f} ns")
        
        # Test interpolation at various time points
        test_times_seconds = np.array([0, 1e-9, 5e-9, 10e-9, 15e-9, 20e-9])  # 0 to 20 ns in seconds
        
        print(f"\n🧪 Testing interpolation with correct time scale:")
        for t in test_times_seconds:
            amplitude = np.interp(t, time_data_seconds, amplitude_data)
            print(f"  t={t*1e9:5.1f} ns: amplitude={amplitude:.3e}")
            
    else:
        print(f"❌ Waveform file not found: {waveform_file}")

if __name__ == "__main__":
    debug_waveform_timescale()