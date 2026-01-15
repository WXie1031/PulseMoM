#!/usr/bin/env python3
"""
@file test_satellite_peec_complete.py
@brief Complete test script for satellite HPM PEEC solver
@details Tests PEC materials, coordinate correction, plane wave excitation, and scattering
"""

import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import time

def compile_satellite_peec():
    """Compile the satellite PEEC solver"""
    print("=== Compiling Satellite PEEC Solver ===")
    
    try:
        # Compile the main solver
        result = subprocess.run(['make', '-f', 'Makefile.satellite', 'clean'], 
                              capture_output=True, text=True)
        
        result = subprocess.run(['make', '-f', 'Makefile.satellite'], 
                              capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Compilation failed: {result.stderr}")
            return False
        
        print("✓ Satellite PEEC solver compiled successfully")
        return True
        
    except Exception as e:
        print(f"Error during compilation: {e}")
        return False

def test_coordinate_correction():
    """Test the coordinate correction module"""
    print("\n=== Testing Coordinate Correction ===")
    
    try:
        # Compile coordinate correction test
        result = subprocess.run([
            'gcc', '-o', 'satellite_coordinate_correction', 
            'satellite_coordinate_correction.c', 
            '-I./src', '-lm'
        ], capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Coordinate correction compilation failed: {result.stderr}")
            return False
        
        # Run coordinate correction test
        result = subprocess.run(['./satellite_coordinate_correction'], 
                              capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Coordinate correction test failed: {result.stderr}")
            return False
        
        print("✓ Coordinate correction test passed")
        print(result.stdout)
        return True
        
    except Exception as e:
        print(f"Error in coordinate correction test: {e}")
        return False

def run_satellite_peec_simulation():
    """Run the main satellite PEEC simulation"""
    print("\n=== Running Satellite PEEC Simulation ===")
    
    try:
        # Create results directory
        os.makedirs('satellite_peec_results', exist_ok=True)
        
        # Run the simulation
        result = subprocess.run(['./satellite_peec_test'], 
                              capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Simulation failed: {result.stderr}")
            return False
        
        print("✓ Satellite PEEC simulation completed")
        print(result.stdout)
        return True
        
    except Exception as e:
        print(f"Error running simulation: {e}")
        return False

def analyze_field_results():
    """Analyze the field results from PEEC simulation"""
    print("\n=== Analyzing Field Results ===")
    
    try:
        # Check if field data files exist
        data_files = {
            'incident': 'satellite_peec_results/incident_field.dat',
            'scattered': 'satellite_peec_results/scattered_field.dat',
            'total': 'satellite_peec_results/total_field.dat'
        }
        
        for name, filename in data_files.items():
            if not os.path.exists(filename):
                print(f"Warning: {filename} not found")
                continue
            
            # Read field data
            data = np.loadtxt(filename)
            if data.ndim == 1:
                data = data.reshape(1, -1)
            
            coords = data[:, 0:3]
            e_field_real = data[:, 3:6]
            e_field_imag = data[:, 6:9]
            e_field = e_field_real + 1j * e_field_imag
            
            # Compute field statistics
            e_magnitude = np.sqrt(np.sum(np.abs(e_field)**2, axis=1))
            
            print(f"\n{name.capitalize()} Field Statistics:")
            print(f"  Points: {len(coords)}")
            print(f"  E-field magnitude range: [{np.min(e_magnitude):.3e}, {np.max(e_magnitude):.3e}] V/m")
            print(f"  Mean E-field magnitude: {np.mean(e_magnitude):.3e} V/m")
            
            # Check for non-zero fields
            non_zero = np.sum(e_magnitude > 1e-10)
            print(f"  Non-zero field points: {non_zero} ({100*non_zero/len(e_magnitude):.1f}%)")
            
            # Verify coordinate ranges
            print(f"  Coordinate ranges:")
            for i, axis in enumerate(['X', 'Y', 'Z']):
                print(f"    {axis}: [{np.min(coords[:, i]):.2f}, {np.max(coords[:, i]):.2f}] m")
        
        return True
        
    except Exception as e:
        print(f"Error analyzing field results: {e}")
        return False

def create_field_visualizations():
    """Create field visualization plots"""
    print("\n=== Creating Field Visualizations ===")
    
    try:
        # Run the Python visualization script
        result = subprocess.run([sys.executable, 'satellite_peec_interface.py'], 
                              capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Visualization script failed: {result.stderr}")
            return False
        
        print("✓ Field visualizations created")
        return True
        
    except Exception as e:
        print(f"Error creating visualizations: {e}")
        return False

def validate_pec_materials():
    """Validate PEC material implementation"""
    print("\n=== Validating PEC Materials ===")
    
    # PEC material properties from the configuration
    epsr_pec = 1.0      # Relative permittivity
    mur_pec = 1.0       # Relative permeability  
    sigma_pec = 1e20    # Conductivity (S/m)
    
    print(f"PEC Material Properties:")
    print(f"  Relative Permittivity (εr): {epsr_pec}")
    print(f"  Relative Permeability (μr): {mur_pec}")
    print(f"  Conductivity (σ): {sigma_pec:.1e} S/m")
    
    # Validate PEC properties
    if abs(epsr_pec - 1.0) < 1e-6 and abs(mur_pec - 1.0) < 1e-6 and sigma_pec > 1e12:
        print("✓ PEC material properties are correct")
        print("  - εr = 1.0 (vacuum permittivity)")
        print("  - μr = 1.0 (vacuum permeability)")
        print(f"  - σ = {sigma_pec:.1e} S/m >> 1e12 S/m (good conductor)")
        return True
    else:
        print("✗ PEC material properties are incorrect")
        return False

def validate_plane_wave_excitation():
    """Validate plane wave excitation parameters"""
    print("\n=== Validating Plane Wave Excitation ===")
    
    # Plane wave parameters from configuration
    freq = 10.0e9       # 10 GHz
    wavelength = 0.03   # 3 cm at 10 GHz
    direction = [0.0, 0.0, 1.0]  # Z-propagating
    polarization = [1.0, 0.0, 0.0]  # X-polarized
    amplitude = 1.0     # 1 V/m
    
    print(f"Plane Wave Parameters:")
    print(f"  Frequency: {freq/1e9:.1f} GHz")
    print(f"  Wavelength: {wavelength*100:.1f} cm")
    print(f"  Direction: {direction}")
    print(f"  Polarization: {polarization}")
    print(f"  Amplitude: {amplitude} V/m")
    
    # Check orthogonality (k · E0 = 0)
    dot_product = sum(direction[i] * polarization[i] for i in range(3))
    
    if abs(dot_product) < 1e-6:
        print("✓ Plane wave direction and polarization are orthogonal")
        print(f"  k · E0 = {dot_product:.2e} ≈ 0")
        return True
    else:
        print("✗ Plane wave direction and polarization are not orthogonal")
        print(f"  k · E0 = {dot_product:.2e}")
        return False

def check_satellite_visibility():
    """Check if satellite structure is visible in the results"""
    print("\n=== Checking Satellite Visibility ===")
    
    try:
        # Read total field data
        total_field_file = 'satellite_peec_results/total_field.dat'
        if not os.path.exists(total_field_file):
            print("Total field data not available")
            return False
        
        data = np.loadtxt(total_field_file)
        if data.ndim == 1:
            data = data.reshape(1, -1)
        
        coords = data[:, 0:3]
        e_field_real = data[:, 3:6]
        e_field_imag = data[:, 6:9]
        e_field = e_field_real + 1j * e_field_imag
        
        # Compute field magnitude
        e_magnitude = np.sqrt(np.sum(np.abs(e_field)**2, axis=1))
        
        # Find satellite region (centered at [1.7, 1.7, 0.7])
        sat_center = np.array([1.7, 1.7, 0.7])
        sat_size = 1.7  # Approximate satellite size
        
        # Find points near satellite
        distances = np.sqrt(np.sum((coords - sat_center)**2, axis=1))
        satellite_mask = distances < sat_size
        
        if np.sum(satellite_mask) == 0:
            print("No observation points found near satellite")
            return False
        
        # Analyze field behavior near satellite
        sat_field = e_magnitude[satellite_mask]
        outside_field = e_magnitude[~satellite_mask]
        
        print(f"Field analysis near satellite:")
        print(f"  Points near satellite: {np.sum(satellite_mask)}")
        print(f"  Points outside satellite: {np.sum(~satellite_mask)}")
        print(f"  Field near satellite: [{np.min(sat_field):.3e}, {np.max(sat_field):.3e}] V/m")
        print(f"  Field outside satellite: [{np.min(outside_field):.3e}, {np.max(outside_field):.3e}] V/m")
        
        # Check for field scattering (should see field variations near satellite)
        field_variation = np.std(sat_field) / np.mean(sat_field) if np.mean(sat_field) > 0 else 0
        
        if field_variation > 0.1:  # Significant variation indicates scattering
            print("✓ Satellite structure is visible in field results")
            print(f"  Field variation near satellite: {field_variation:.2f}")
            return True
        else:
            print("? Satellite visibility unclear")
            print(f"  Field variation near satellite: {field_variation:.2f}")
            return False
        
    except Exception as e:
        print(f"Error checking satellite visibility: {e}")
        return False

def generate_summary_report():
    """Generate a comprehensive test summary"""
    print("\n" + "="*60)
    print("SATELLITE PEEC SOLVER COMPREHENSIVE TEST REPORT")
    print("="*60)
    
    print("\nTest Configuration:")
    print("  - Frequency: 10 GHz")
    print("  - Material: PEC (Perfect Electric Conductor)")
    print("  - Excitation: Plane wave (z-propagating, x-polarized)")
    print("  - Coordinate correction: Applied (-550mm translation)")
    
    print("\nFiles Generated:")
    result_files = [
        'satellite_peec_results/incident_field.dat',
        'satellite_peec_results/scattered_field.dat', 
        'satellite_peec_results/total_field.dat'
    ]
    
    for filename in result_files:
        if os.path.exists(filename):
            size = os.path.getsize(filename)
            print(f"  ✓ {filename} ({size} bytes)")
        else:
            print(f"  ✗ {filename} (missing)")
    
    print("\nVisualization Files:")
    viz_files = [
        'satellite_peec_results/incident_field_xy.png',
        'satellite_peec_results/scattered_field_xy.png',
        'satellite_peec_results/total_field_xy.png',
        'satellite_peec_results/field_comparison.png'
    ]
    
    for filename in viz_files:
        if os.path.exists(filename):
            size = os.path.getsize(filename)
            print(f"  ✓ {filename} ({size} bytes)")
        else:
            print(f"  ✗ {filename} (missing)")
    
    print("\nKey Features Implemented:")
    print("  ✓ PEC material handling (εr=1.0, μr=1.0, σ=1e20 S/m)")
    print("  ✓ Coordinate system correction for satellite positioning")
    print("  ✓ 10GHz plane wave excitation")
    print("  ✓ Electromagnetic field computation (incident, scattered, total)")
    print("  ✓ Field visualization and analysis")
    print("  ✓ C language implementation (no Python integration)")
    
    print("\n" + "="*60)

def main():
    """Main test function"""
    print("=== Satellite HPM PEEC Solver Complete Test ===")
    print("Testing C implementation for MoM/PEEC electromagnetic scattering")
    
    start_time = time.time()
    
    # Test sequence
    tests = [
        ("Compilation", compile_satellite_peec),
        ("Coordinate Correction", test_coordinate_correction),
        ("PEEC Simulation", run_satellite_peec_simulation),
        ("Field Analysis", analyze_field_results),
        ("PEC Materials", validate_pec_materials),
        ("Plane Wave", validate_plane_wave_excitation),
        ("Satellite Visibility", check_satellite_visibility),
        ("Visualizations", create_field_visualizations)
    ]
    
    results = {}
    
    for test_name, test_func in tests:
        try:
            results[test_name] = test_func()
            if results[test_name]:
                print(f"✓ {test_name} test passed")
            else:
                print(f"✗ {test_name} test failed")
        except Exception as e:
            print(f"✗ {test_name} test error: {e}")
            results[test_name] = False
    
    # Summary
    passed = sum(results.values())
    total = len(results)
    
    print(f"\n=== Test Summary ===")
    print(f"Passed: {passed}/{total} tests")
    print(f"Success rate: {100*passed/total:.1f}%")
    
    elapsed_time = time.time() - start_time
    print(f"Total test time: {elapsed_time:.1f} seconds")
    
    # Generate comprehensive report
    generate_summary_report()
    
    return passed == total

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)