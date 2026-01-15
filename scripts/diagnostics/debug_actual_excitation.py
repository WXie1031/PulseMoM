#!/usr/bin/env python3
"""
Debug the actual excitation calculation in the simulation
"""

import numpy as np

def debug_actual_excitation():
    """Debug the actual excitation calculation"""
    
    print("Debugging actual excitation calculation...")
    
    # Test the exact same parameters as in the simulation
    frequency = 10e9  # 10 GHz
    c = 299792458.0  # 光速
    wavelength = c / frequency
    k = 2 * np.pi / wavelength
    
    # Test artificial RWG function similar to what's created
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
    
    # Test the exact excitation calculation
    incident_direction = np.array([1, 0, 0])  # +X方向
    polarization = np.array([0, 0, 1])  # Z方向极化
    incident_amplitude = 1e3  # 1 kV/m
    
    # Calculate centers
    center_plus = np.mean(np.array(test_rwg['plus_triangle']['vertices']), axis=0)
    center_minus = np.mean(np.array(test_rwg['minus_triangle']['vertices']), axis=0)
    
    print(f"Plus triangle center: {center_plus}")
    print(f"Minus triangle center: {center_minus}")
    
    # Calculate phases
    phase_plus = -1j * k * np.dot(center_plus, incident_direction)
    phase_minus = -1j * k * np.dot(center_minus, incident_direction)
    
    print(f"Phase plus: {phase_plus}")
    print(f"Phase minus: {phase_minus}")
    
    # Calculate incident fields
    E_inc_plus = np.exp(phase_plus) * polarization
    E_inc_minus = np.exp(phase_minus) * polarization
    
    print(f"E_inc_plus: {E_inc_plus}")
    print(f"E_inc_minus: {E_inc_minus}")
    
    # Calculate dot products
    dot_plus = np.dot(E_inc_plus, [0, 0, 1])
    dot_minus = np.dot(E_inc_minus, [0, 0, 1])
    
    print(f"Dot product plus: {dot_plus}")
    print(f"Dot product minus: {dot_minus}")
    
    # RWG integration
    area_plus = test_rwg['plus_triangle']['area']
    area_minus = test_rwg['minus_triangle']['area']
    edge_length = test_rwg['edge_length']
    
    V_excitation = dot_plus * area_plus - dot_minus * area_minus
    V_excitation_scaled = V_excitation * edge_length / 2.0 * incident_amplitude
    
    print(f"V_excitation (raw): {V_excitation}")
    print(f"V_excitation (scaled): {V_excitation_scaled}")
    print(f"V_excitation magnitude: {abs(V_excitation_scaled):.2e}")
    
    # Check if this is actually zero
    if abs(V_excitation_scaled) < 1e-15:
        print("❌ V_excitation is effectively zero!")
        print("This explains why the excitation vector magnitude is 0.00e+00")
    else:
        print("✅ V_excitation is non-zero")
    
    # Test with different triangle positions
    print(f"\nTesting with triangles at different positions...")
    
    # Move triangles to X = 1.0 (where the satellite is)
    test_rwg_2 = {
        'plus_triangle': {
            'vertices': [[1.0, 0, 0], [1.01, 0, 0], [1.0, 0.01, 0]],
            'area': 5e-5
        },
        'minus_triangle': {
            'vertices': [[1.0, 0, 0], [1.0, 0.01, 0], [0.99, 0, 0]],
            'area': 5e-5
        },
        'edge_length': 0.01
    }
    
    center_plus_2 = np.mean(np.array(test_rwg_2['plus_triangle']['vertices']), axis=0)
    center_minus_2 = np.mean(np.array(test_rwg_2['minus_triangle']['vertices']), axis=0)
    
    phase_plus_2 = -1j * k * np.dot(center_plus_2, incident_direction)
    phase_minus_2 = -1j * k * np.dot(center_minus_2, incident_direction)
    
    E_inc_plus_2 = np.exp(phase_plus_2) * polarization
    E_inc_minus_2 = np.exp(phase_minus_2) * polarization
    
    dot_plus_2 = np.dot(E_inc_plus_2, [0, 0, 1])
    dot_minus_2 = np.dot(E_inc_minus_2, [0, 0, 1])
    
    V_excitation_2 = dot_plus_2 * 5e-5 - dot_minus_2 * 5e-5
    V_excitation_scaled_2 = V_excitation_2 * 0.01 / 2.0 * 1e3
    
    print(f"New V_excitation magnitude: {abs(V_excitation_scaled_2):.2e}")

if __name__ == "__main__":
    debug_actual_excitation()