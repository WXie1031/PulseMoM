#!/usr/bin/env python3
"""
Debug the MoM solving process to understand why currents are zero
"""

import numpy as np
from scipy.sparse.linalg import gmres

def debug_mom_solving():
    """Debug MoM solving process"""
    
    print("Debugging MoM solving process...")
    
    # Simulate the current situation
    N = 100  # Number of RWG functions
    
    # Create a simple test case
    print(f"Testing with {N} RWG functions...")
    
    # Create a simple impedance matrix (diagonal dominant)
    Z_matrix = np.eye(N, dtype=complex) * (1 + 1j)  # Simple diagonal matrix
    
    # Create excitation vector with small but non-zero values
    V_vector = np.ones(N, dtype=complex) * 3.22e-07  # Based on our excitation calculation
    
    print(f"Impedance matrix shape: {Z_matrix.shape}")
    print(f"Excitation vector shape: {V_vector.shape}")
    print(f"Excitation vector magnitude: {np.linalg.norm(V_vector):.2e}")
    print(f"Max excitation: {np.max(np.abs(V_vector)):.2e}")
    print(f"Min excitation: {np.min(np.abs(V_vector)):.2e}")
    
    # Check matrix condition
    try:
        condition_number = np.linalg.cond(Z_matrix)
        print(f"Condition number: {condition_number:.2e}")
    except:
        print("Could not calculate condition number")
    
    # Solve the system
    try:
        # First try direct solve
        if N < 200:
            currents_direct = np.linalg.solve(Z_matrix, V_vector)
            print(f"Direct solve successful")
            print(f"Currents magnitude range: {np.min(np.abs(currents_direct)):.2e} - {np.max(np.abs(currents_direct)):.2e}")
        
        # Try iterative solve
        currents_iterative, info = gmres(Z_matrix, V_vector, tol=1e-6)
        print(f"GMRES solve info: {info}")
        if info == 0:
            print(f"GMRES solve successful")
            print(f"Currents magnitude range: {np.min(np.abs(currents_iterative)):.2e} - {np.max(np.abs(currents_iterative)):.2e}")
        else:
            print(f"GMRES solve failed with info: {info}")
            
    except Exception as e:
        print(f"Solving failed: {e}")
    
    # Test with different excitation magnitudes
    print(f"\nTesting with different excitation magnitudes:")
    excitation_magnitudes = [1e-3, 1e-6, 1e-9, 1e-12]
    
    for mag in excitation_magnitudes:
        V_test = np.ones(N, dtype=complex) * mag
        try:
            currents_test, info = gmres(Z_matrix, V_test, tol=1e-6)
            if info == 0:
                max_current = np.max(np.abs(currents_test))
                print(f"  Excitation {mag:.0e}: max current = {max_current:.2e}")
            else:
                print(f"  Excitation {mag:.0e}: solve failed")
        except:
            print(f"  Excitation {mag:.0e}: exception")

if __name__ == "__main__":
    debug_mom_solving()