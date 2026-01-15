#!/usr/bin/env python3
"""
Debug the plane wave excitation function specifically
"""

import numpy as np

def debug_plane_wave_excitation_func():
    """Debug the plane wave excitation function"""
    
    print("Debugging plane wave excitation function...")
    
    # Test parameters from the simulation
    frequency = 10e9  # 10 GHz
    c = 299792458.0  # 光速
    wavelength = c / frequency
    k = 2 * np.pi / wavelength
    omega = 2 * np.pi * frequency
    
    # Test RWG function similar to what's created in the simulation
    test_rwg = {
        'plus_triangle': {
            'vertices': [[0, 0, 0], [0.01, 0, 0], [0, 0.01, 0]],
            'area': 5e-5,
            'free_vertex': 2
        },
        'minus_triangle': {
            'vertices': [[0, 0, 0], [0, 0.01, 0], [-0.01, 0, 0]],
            'area': 5e-5,
            'free_vertex': 2
        },
        'edge_length': 0.01
    }
    
    # Test the exact excitation calculation
    incident_direction = np.array([1, 0, 0])  # +X方向
    polarization = np.array([0, 0, 1])  # Z方向极化
    
    print(f"Frequency: {frequency/1e9} GHz")
    print(f"Wavelength: {wavelength:.4f} m")
    print(f"Wave number k: {k:.4f}")
    print(f"Omega: {omega:.2e}")
    
    # Step-by-step calculation
    print(f"\nStep-by-step calculation:")
    
    # Calculate triangle centers
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
    print(f"|E_inc_plus|: {np.linalg.norm(E_inc_plus):.4f}")
    print(f"|E_inc_minus|: {np.linalg.norm(E_inc_minus):.4f}")
    
    # Calculate dot products
    dot_plus = np.dot(E_inc_plus, [0, 0, 1])
    dot_minus = np.dot(E_inc_minus, [0, 0, 1])
    
    print(f"Dot product plus: {dot_plus}")
    print(f"Dot product minus: {dot_minus}")
    print(f"|dot_plus|: {abs(dot_plus):.4f}")
    print(f"|dot_minus|: {abs(dot_minus):.4f}")
    
    # RWG integration
    area_plus = test_rwg['plus_triangle']['area']
    area_minus = test_rwg['minus_triangle']['area']
    edge_length = test_rwg['edge_length']
    
    print(f"Area plus: {area_plus}")
    print(f"Area minus: {area_minus}")
    print(f"Edge length: {edge_length}")
    
    V_excitation = dot_plus * area_plus - dot_minus * area_minus
    V_excitation_scaled = V_excitation * edge_length / 2.0
    
    print(f"V_excitation (raw): {V_excitation}")
    print(f"V_excitation (scaled): {V_excitation_scaled}")
    print(f"V_excitation magnitude: {abs(V_excitation_scaled):.2e}")
    
    # Check if this is actually zero
    if abs(V_excitation_scaled) < 1e-15:
        print("❌ V_excitation is effectively zero!")
    else:
        print("✅ V_excitation is non-zero")
    
    # Test with artificial RWG function (what's actually created)
    print(f"\nTesting with artificial RWG function:")
    
    artificial_rwg = {
        'plus_triangle': {
            'vertices': [[1.0, 0, 0], [1.01, 0, 0], [1.0, 0.01, 0]],
            'area': 5e-5,
            'free_vertex': 2
        },
        'minus_triangle': {
            'vertices': [[1.0, 0, 0], [1.0, 0.01, 0], [0.99, 0, 0]],
            'area': 5e-5,
            'free_vertex': 2
        },
        'edge_length': 0.01
    }
    
    # Recalculate with artificial RWG
    center_plus_art = np.mean(np.array(artificial_rwg['plus_triangle']['vertices']), axis=0)
    center_minus_art = np.mean(np.array(artificial_rwg['minus_triangle']['vertices']), axis=0)
    
    phase_plus_art = -1j * k * np.dot(center_plus_art, incident_direction)
    phase_minus_art = -1j * k * np.dot(center_minus_art, incident_direction)
    
    E_inc_plus_art = np.exp(phase_plus_art) * polarization
    E_inc_minus_art = np.exp(phase_minus_art) * polarization
    
    dot_plus_art = np.dot(E_inc_plus_art, [0, 0, 1])
    dot_minus_art = np.dot(E_inc_minus_art, [0, 0, 1])
    
    V_excitation_art = dot_plus_art * 5e-5 - dot_minus_art * 5e-5
    V_excitation_scaled_art = V_excitation_art * 0.01 / 2.0
    
    print(f"Artificial RWG V_excitation magnitude: {abs(V_excitation_scaled_art):.2e}")

if __name__ == "__main__":
    debug_plane_wave_excitation_func()