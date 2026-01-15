#!/usr/bin/env python3
"""
Debug the correct waveform time scale
"""

import numpy as np
import os

# Simulate the waveform loading and interpolation
def debug_correct_timescale():
    print("🔍 Finding correct waveform time scale...")
    
    # Load the waveform file like in the original code
    waveform_file = "hpm_waveform_X(10.0GHz)_20ns.txt"
    
    if os.path.exists(waveform_file):
        data = np.loadtxt(waveform_file)
        time_data_raw = data[:, 0]  # Raw time values
        amplitude_data = data[:, 1]
        
        print(f"Raw waveform data analysis:")
        print(f"  Time range: {time_data_raw[0]:.6e} to {time_data_raw[-1]:.6e}")
        print(f"  This appears to be: {time_data_raw[-1]*1e9:.1f} nanoseconds")
        print(f"  So the unit is likely: SECONDS (not nanoseconds)")
        
        # The values are already in seconds, so no conversion needed
        time_data_correct = time_data_raw
        
        print(f"\nCorrect time data (seconds):")
        print(f"  Range: {time_data_correct[0]:.6e} to {time_data_correct[-1]:.6e}")
        print(f"  This equals: {time_data_correct[0]*1e9:.3f} to {time_data_correct[-1]*1e9:.3f} ns")
        
        # Test interpolation at various time points
        test_times_seconds = np.array([0, 1e-9, 5e-9, 10e-9, 15e-9, 20e-9])  # 0 to 20 ns in seconds
        
        print(f"\n🧪 Testing interpolation with CORRECT time scale:")
        for t in test_times_seconds:
            amplitude = np.interp(t, time_data_correct, amplitude_data)
            print(f"  t={t*1e9:5.1f} ns: amplitude={amplitude:.6f}")
            
        # Show some statistics
        print(f"\n📊 Waveform statistics:")
        print(f"  Peak positive amplitude: {np.max(amplitude_data):.6f}")
        print(f"  Peak negative amplitude: {np.min(amplitude_data):.6f}")
        print(f"  RMS amplitude: {np.sqrt(np.mean(amplitude_data**2)):.6f}")
        
    else:
        print(f"❌ Waveform file not found: {waveform_file}")

if __name__ == "__main__":
    debug_correct_timescale()