#!/usr/bin/env python3
"""
Debug the plane wave implementation and waveform injection
"""

import numpy as np
import matplotlib.pyplot as plt
import os

# Import the classes from the main script
import sys
sys.path.append('.')

# Recreate the key parameters from the simulation
PI = np.pi
C0 = 299792458.0

# Configuration parameters from weixing_v1_case.pfd
frequency = 10.0e9  # 10GHz
wavelength = C0 / frequency  # 0.03 m (30mm)
theta = 45.0  # degrees
phi = 45.0    # degrees  
psi = 45.0    # degrees

print("🔍 Debugging Plane Wave Implementation")
print("="*50)

# Convert to radians
theta_rad = np.radians(theta)
phi_rad = np.radians(phi)
psi_rad = np.radians(psi)

print(f"Simulation parameters:")
print(f"  Frequency: {frequency/1e9} GHz")
print(f"  Wavelength: {wavelength*1000:.1f} mm")
print(f"  Incident angles: θ={theta}°, φ={phi}°, ψ={psi}°")

# Calculate wave vector direction
k_direction = np.array([
    np.sin(theta_rad) * np.cos(phi_rad),
    np.sin(theta_rad) * np.sin(phi_rad),
    np.cos(theta_rad)
])

print(f"\nWave vector k (normalized):")
print(f"  k = [{k_direction[0]:.3f}, {k_direction[1]:.3f}, {k_direction[2]:.3f}]")
print(f"  |k| = {np.linalg.norm(k_direction):.6f}")

# Calculate polarization vector
polarization = np.array([
    np.cos(psi_rad) * np.cos(phi_rad) - np.sin(psi_rad) * np.cos(theta_rad) * np.sin(phi_rad),
    np.cos(psi_rad) * np.sin(phi_rad) + np.sin(psi_rad) * np.cos(theta_rad) * np.cos(phi_rad),
    -np.sin(psi_rad) * np.sin(theta_rad)
])

# Normalize polarization
polarization = polarization / np.linalg.norm(polarization)

print(f"\nPolarization vector E0 (normalized):")
print(f"  E0 = [{polarization[0]:.3f}, {polarization[1]:.3f}, {polarization[2]:.3f}]")
print(f"  |E0| = {np.linalg.norm(polarization):.6f}")

# Check orthogonality (k and E should be orthogonal for plane wave)
kdote = np.dot(k_direction, polarization)
print(f"\nOrthogonality check:")
print(f"  k·E0 = {kdote:.6f} (should be ~0 for plane wave)")

# Load and analyze the waveform
waveform_file = "hpm_waveform_X(10.0GHz)_20ns.txt"
if __name__ == "__main__":
    if os.path.exists(waveform_file):
        data = np.loadtxt(waveform_file)
        time_data = data[:, 0]  # Already in seconds
        amplitude_data = data[:, 1]
        
        print(f"\nWaveform analysis:")
        print(f"  Time range: {time_data[0]*1e9:.1f} to {time_data[-1]*1e9:.1f} ns")
        print(f"  Amplitude range: {np.min(amplitude_data):.3f} to {np.max(amplitude_data):.3f}")
        print(f"  Peak amplitude: {np.max(np.abs(amplitude_data)):.6f}")
        
        # Test the waveform at key time points
        test_times = np.array([0, 5e-9, 10e-9, 15e-9, 20e-9])
        print(f"\nWaveform amplitude at key times:")
        for t in test_times:
            amp = np.interp(t, time_data, amplitude_data)
            print(f"  t={t*1e9:4.0f} ns: amplitude={amp:8.4f}")
            
        # Plot the waveform
        plt.figure(figsize=(12, 6))
        plt.subplot(1, 2, 1)
        plt.plot(time_data * 1e9, amplitude_data, 'b-', linewidth=1)
        plt.xlabel('Time (ns)')
        plt.ylabel('Amplitude')
        plt.title('HPM Waveform (10GHz, 20ns)')
        plt.grid(True, alpha=0.3)
        
        # Test plane wave field calculation at a simple point
        test_point = np.array([0, 0, 0])  # Origin
        k0 = 2 * PI / wavelength
        
        print(f"\nPlane wave field test at origin [0,0,0]:")
        print(f"  k0 = {k0:.3f} rad/m")
        print(f"  Phase at origin: -k·r = -{k0:.3f} * 0 = 0 rad")
        
        # Calculate field at different times
        field_magnitudes = []
        for t in [0, 5e-9, 10e-9, 15e-9, 20e-9]:
            waveform_amp = np.interp(t, time_data, amplitude_data)
            phase = -k0 * np.dot(k_direction, test_point)
            E_field = waveform_amp * polarization * np.cos(phase)
            field_mag = np.linalg.norm(E_field)
            field_magnitudes.append(field_mag)
            print(f"  t={t*1e9:4.0f} ns: |E|={field_mag:8.4f} V/m, waveform_amp={waveform_amp:8.4f}")
        
        plt.subplot(1, 2, 2)
        times_ns = np.array([0, 5, 10, 15, 20])
        plt.plot(times_ns, field_magnitudes, 'ro-', linewidth=2, markersize=8)
        plt.xlabel('Time (ns)')
        plt.ylabel('|E| (V/m)')
        plt.title('Plane Wave Field Magnitude at Origin')
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig("plane_wave_debug_analysis.png", dpi=300, bbox_inches='tight')
        plt.show(block=False)
        
        print(f"\n✅ Analysis complete! Check plane_wave_debug_analysis.png for plots.")
        
    else:
        print(f"❌ Waveform file not found: {waveform_file}")

import os  # Add this import