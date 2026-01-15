#!/usr/bin/env python3
"""
Simple test to debug the satellite MoM/PEEC implementation
"""

import numpy as np
import matplotlib.pyplot as plt
import sys
import os

# Add current directory to path
sys.path.append('.')

def main():
    print("Starting simple satellite MoM/PEEC test...")
    
    try:
        # Import the main tester
        from satellite_mom_peec_final import SatelliteMoMPEECTester
        
        # Create a simple instance
        tester = SatelliteMoMPEECTester(
            stl_file='tests/test_hpm/weixing_v1.stl',
            pfd_file='tests/test_hpm/weixing_v1_case.pfd',
            max_facets=100,  # Use fewer facets for testing
            frequency=10e9,
            mesh_accuracy='standard'
        )
        
        print("✅ Tester created successfully")
        
        # Run just the basic simulation without visualization
        print("Running basic simulation...")
        
        # Step 1: Parse STL
        print("Step 1: Parsing STL...")
        success = tester._parse_stl_geometry()
        if success:
            print(f"   Vertices: {len(tester.vertices)}")
            print(f"   Facets: {len(tester.facets)}")
        
        # Step 2: Generate RWG functions
        print("Step 2: Generating RWG functions...")
        success = tester._generate_rwg_basis()
        if success:
            print(f"   RWG functions: {len(tester.rwg_functions)}")
        
        # Step 3: Calculate MoM matrix
        print("Step 3: Calculating MoM impedance matrix...")
        success = tester._calculate_mom_matrix()
        if success:
            print(f"   Matrix shape: {tester.impedance_matrix.shape}")
        
        # Step 4: Solve currents
        print("Step 4: Solving surface currents...")
        success = tester._setup_excitation_and_solve()
        if success:
            print(f"   Current range: {np.min(np.abs(tester.surface_currents)):.2e} - {np.max(np.abs(tester.surface_currents)):.2e}")
        
        # Step 5: Calculate fields
        print("Step 5: Calculating electromagnetic fields...")
        success = tester._calculate_electromagnetic_fields()
        if success:
            print(f"   Incident field range: {np.min(np.abs(tester.results['incident_fields'])):.2e} - {np.max(np.abs(tester.results['incident_fields'])):.2e}")
            print(f"   Scattered field range: {np.min(np.abs(tester.results['scattered_fields'])):.2e} - {np.max(np.abs(tester.results['scattered_fields'])):.2e}")
        
        print("\n✅ Basic simulation completed successfully!")
        
        # Now test just one visualization component
        print("\nTesting field enhancement visualization...")
        fig, ax = plt.subplots(1, 1, figsize=(8, 6))
        
        incident_fields = np.abs(tester.results['incident_fields'])
        total_fields = np.abs(tester.results['total_fields'])
        
        print(f"Incident fields shape: {incident_fields.shape}")
        print(f"Total fields shape: {total_fields.shape}")
        
        # Calculate enhancement factor
        enhancement_factor = total_fields / (incident_fields + 1e-10)
        print(f"Enhancement factor shape: {enhancement_factor.shape}")
        print(f"Enhancement factor range: {np.min(enhancement_factor):.2e} - {np.max(enhancement_factor):.2e}")
        
        # Plot histogram
        ax.hist(enhancement_factor, bins=50, alpha=0.7, color='blue', edgecolor='black')
        ax.set_xlabel('Field Enhancement Factor')
        ax.set_ylabel('Count')
        ax.set_title('Field Enhancement Distribution')
        ax.set_yscale('log')
        ax.grid(True, alpha=0.3)
        
        plt.savefig('simple_field_enhancement_test.png', dpi=150, bbox_inches='tight')
        plt.close()
        
        print("✅ Field enhancement visualization completed")
        
        # Test simulation summary
        print("\nTesting simulation summary...")
        fig, ax = plt.subplots(1, 1, figsize=(8, 6))
        
        summary_text = f"""
        SIMULATION PARAMETERS
        ─────────────────────
        
        Frequency: {tester.frequency/1e9:.1f} GHz
        Wavelength: {tester.wavelength:.4f} m
        STL File: {os.path.basename(tester.stl_file)}
        
        MESH STATISTICS
        ───────────────
        Vertices: {len(tester.vertices)}
        Facets: {len(tester.facets)}
        RWG Functions: {len(tester.rwg_functions)}
        
        ELECTROMAGNETIC RESULTS
        ───────────────────────
        Surface Currents: {np.min(np.abs(tester.surface_currents)):.2e} - {np.max(np.abs(tester.surface_currents)):.2e} A/m
        Incident Field: {np.min(np.abs(tester.results['incident_fields'])):.2e} - {np.max(np.abs(tester.results['incident_fields'])):.2e} V/m
        Scattered Field: {np.min(np.abs(tester.results['scattered_fields'])):.2e} - {np.max(np.abs(tester.results['scattered_fields'])):.2e} V/m
        """
        
        ax.text(0.05, 0.95, summary_text, transform=ax.transAxes, fontsize=10,
                verticalalignment='top', fontfamily='monospace',
                bbox=dict(boxstyle='round', facecolor='lightgray', alpha=0.8))
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.axis('off')
        
        plt.savefig('simple_simulation_summary.png', dpi=150, bbox_inches='tight')
        plt.close()
        
        print("✅ Simulation summary visualization completed")
        
        # Calculate and display final results
        incident_power = np.mean(np.abs(tester.results['incident_fields'])**2)
        scattered_power = np.mean(np.abs(tester.results['scattered_fields'])**2)
        scattering_ratio = scattered_power / incident_power * 100
        rcs = 4 * np.pi * scattered_power / incident_power
        
        print(f"\n[RESULTS] Final Simulation Results:")
        print(f"   入射功率密度: {incident_power:.2e} W/m²")
        print(f"   散射功率密度: {scattered_power:.2e} W/m²")
        print(f"   散射比例: {scattering_ratio:.2f}%")
        print(f"   雷达散射截面: {rcs:.2f} m²")
        
        return True
        
    except Exception as e:
        print(f"❌ Error in simple test: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)