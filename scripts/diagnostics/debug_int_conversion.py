#!/usr/bin/env python3
"""
Debug test to find the exact int() conversion issue
"""

import numpy as np
import matplotlib.pyplot as plt
import sys
import os

# Add current directory to path
sys.path.append('.')

def main():
    print("Starting debug test for int() conversion issue...")
    
    try:
        # Import the main tester
        from satellite_mom_peec_final import SatelliteMoMPEECTester
        
        # Create a simple instance
        tester = SatelliteMoMPEECTester(
            stl_file='tests/test_hpm/weixing_v1.stl',
            pfd_file='tests/test_hpm/weixing_v1_case.pfd',
            max_facets=10,  # Use very few facets for testing
            frequency=10e9,
            mesh_accuracy='standard'
        )
        
        print("✅ Tester created successfully")
        
        # Run the complete simulation
        print("Running complete simulation...")
        
        # Parse STL
        success = tester._parse_stl_geometry()
        if not success:
            print("❌ STL parsing failed")
            return False
        
        # Generate RWG functions
        success = tester._generate_rwg_basis()
        if not success:
            print("❌ RWG generation failed")
            return False
        
        # Calculate MoM matrix
        success = tester._calculate_mom_matrix()
        if not success:
            print("❌ MoM matrix calculation failed")
            return False
        
        # Solve currents
        success = tester._setup_excitation_and_solve()
        if not success:
            print("❌ Current solving failed")
            return False
        
        # Calculate fields
        success = tester._calculate_electromagnetic_fields()
        if not success:
            print("❌ Field calculation failed")
            return False
        
        print("✅ Simulation completed successfully")
        
        # Debug the RWG functions structure
        print(f"Number of RWG functions: {len(tester.rwg_functions)}")
        
        if tester.rwg_functions:
            for i, rwg in enumerate(tester.rwg_functions[:3]):  # Check first 3
                print(f"\nRWG {i}:")
                print(f"  Keys: {list(rwg.keys())}")
                if 'edge_vertices' in rwg:
                    edge_vertices = rwg['edge_vertices']
                    print(f"  edge_vertices: {edge_vertices}")
                    print(f"  edge_vertices type: {type(edge_vertices)}")
                    print(f"  edge_vertices length: {len(edge_vertices)}")
                    if edge_vertices:
                        print(f"  max(edge_vertices): {max(edge_vertices)}")
                        print(f"  max(edge_vertices) type: {type(max(edge_vertices))}")
                        try:
                            int_max = int(max(edge_vertices))
                            print(f"  int(max(edge_vertices)): {int_max}")
                        except Exception as e:
                            print(f"  ❌ Error converting max to int: {e}")
        
        # Test the specific visualization method that's failing
        print("\nTesting RWG distribution plotting...")
        fig = plt.figure(figsize=(8, 8))
        ax = fig.add_subplot(111, projection='3d')
        
        # 绘制RWG边中心
        for i, rwg in enumerate(tester.rwg_functions[:min(5, len(tester.rwg_functions))]):
            try:
                print(f"\nProcessing RWG {i}...")
                # 计算边中心
                edge_vertices = rwg['edge_vertices']
                print(f"  edge_vertices: {edge_vertices}")
                
                # Debug the comparison
                vertices_len = len(tester.vertices)
                max_edge = max(edge_vertices)
                print(f"  len(vertices): {vertices_len}")
                print(f"  max(edge_vertices): {max_edge}")
                print(f"  max(edge_vertices) type: {type(max_edge)}")
                
                # Test the int conversion
                int_max_edge = int(max_edge)
                print(f"  int(max(edge_vertices)): {int_max_edge}")
                
                # Test the comparison
                comparison_result = vertices_len > int_max_edge
                print(f"  len(vertices) > int(max(edge_vertices)): {comparison_result}")
                
                if comparison_result:
                    v1 = np.array(tester.vertices[edge_vertices[0]])
                    v2 = np.array(tester.vertices[edge_vertices[1]])
                    center = (v1 + v2) / 2
                    
                    ax.scatter(center[0], center[1], center[2], c='red', s=10)
                    print(f"  ✅ Successfully plotted RWG {i}")
                else:
                    print(f"  ⚠️  Skipping RWG {i} due to vertex index out of range")
                    
            except Exception as e:
                print(f"  ❌ Error processing RWG {i}: {e}")
                import traceback
                traceback.print_exc()
                break  # Stop at first error
        
        ax.set_title('RWG Basis Functions (Debug)')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        ax.set_zlabel('Z (m)')
        
        plt.savefig('debug_rwg_distribution.png', dpi=150, bbox_inches='tight')
        plt.close()
        print("✅ RWG distribution plotting completed")
        
        return True
        
    except Exception as e:
        print(f"❌ Error in debug test: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)