#!/usr/bin/env python3
"""
Debug the plane wave excitation calculation
"""

import numpy as np

def debug_plane_wave_excitation():
    """Debug plane wave excitation"""
    
    print("Debugging plane wave excitation...")
    
    # Test parameters
    frequency = 10e9  # 10 GHz
    c = 299792458.0  # 光速
    wavelength = c / frequency
    k = 2 * np.pi / wavelength
    
    print(f"Frequency: {frequency/1e9} GHz")
    print(f"Wavelength: {wavelength:.4f} m")
    print(f"Wave number k: {k:.4f}")
    
    # Test excitation calculation
    incident_direction = np.array([1, 0, 0])  # +X方向
    polarization = np.array([0, 0, 1])  # Z方向极化
    
    # Test observation points
    test_points = [
        [0, 0, 0],
        [0.1, 0, 0],
        [1, 0, 0],
        [0, 0.1, 0],
        [0, 0, 0.1]
    ]
    
    print(f"\nTesting excitation at various points:")
    for i, point in enumerate(test_points):
        phase = -1j * k * np.dot(point, incident_direction)
        field = np.exp(phase) * polarization
        field_magnitude = np.linalg.norm(field)
        print(f"  Point {point}: phase={phase:.2f}, field_magnitude={field_magnitude:.2f}")
    
    # Check if the issue is in RWG integration
    print(f"\nChecking RWG integration logic:")
    
    # Simulate a simple RWG function
    test_rwg = {
        'plus_triangle': {
            'vertices': [[0, 0, 0], [0.01, 0, 0], [0, 0.01, 0]],
            'area': 5e-5  # 0.5 * 0.01 * 0.01
        },
        'minus_triangle': {
            'vertices': [[0, 0, 0], [0, 0.01, 0], [-0.01, 0, 0]],
            'area': 5e-5
        },
        'edge_length': 0.01
    }
    
    # Calculate centers
    center_plus = np.mean(np.array(test_rwg['plus_triangle']['vertices']), axis=0)
    center_minus = np.mean(np.array(test_rwg['minus_triangle']['vertices']), axis=0)
    
    print(f"Plus triangle center: {center_plus}")
    print(f"Minus triangle center: {center_minus}")
    
    # Calculate excitation
    phase_plus = -1j * k * np.dot(center_plus, incident_direction)
    phase_minus = -1j * k * np.dot(center_minus, incident_direction)
    
    E_inc_plus = np.exp(phase_plus) * polarization
    E_inc_minus = np.exp(phase_minus) * polarization
    
    print(f"E_inc_plus: {E_inc_plus}")
    print(f"E_inc_minus: {E_inc_minus}")
    
    # RWG integration
    area_plus = test_rwg['plus_triangle']['area']
    area_minus = test_rwg['minus_triangle']['area']
    
    V_excitation = np.dot(E_inc_plus, [0, 0, 1]) * area_plus - np.dot(E_inc_minus, [0, 0, 1]) * area_minus
    V_excitation_scaled = V_excitation * test_rwg['edge_length'] / 2.0
    
    print(f"V_excitation (raw): {V_excitation}")
    print(f"V_excitation (scaled): {V_excitation_scaled}")
    print(f"V_excitation magnitude: {abs(V_excitation_scaled):.2e}")

if __name__ == "__main__":
    debug_plane_wave_excitation()