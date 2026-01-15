#!/usr/bin/env python3
"""
Debug the scattered field calculation to understand why it's zero
"""

import numpy as np
import matplotlib.pyplot as plt

def debug_scattered_field():
    """Debug why scattered field is zero"""
    
    print("Debugging scattered field calculation...")
    
    # Simulate the current situation
    surface_currents = np.zeros(100)  # All zero currents
    
    print(f"Surface currents: min={np.min(surface_currents)}, max={np.max(surface_currents)}")
    print(f"All currents zero: {np.all(surface_currents == 0)}")
    
    # Check if this is the issue
    if np.all(surface_currents == 0):
        print("❌ All surface currents are zero!")
        print("This explains why scattered field is zero")
        
        # Let's check the excitation vector
        print("\nPossible causes:")
        print("1. Plane wave excitation is zero")
        print("2. Impedance matrix is singular")
        print("3. Boundary conditions are incorrect")
        
        return False
    else:
        print("✅ Surface currents are non-zero")
        return True

if __name__ == "__main__":
    debug_scattered_field()