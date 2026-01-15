#!/usr/bin/env python3
"""
Debug test to find the exact comparison issue
"""

import numpy as np
import matplotlib.pyplot as plt
import sys
import os

# Add current directory to path
sys.path.append('.')

def main():
    print("Starting debug test...")
    
    try:
        # Import the main tester
        from satellite_mom_peec_final import SatelliteMoMPEECTester
        
        # Create a simple instance
        tester = SatelliteMoMPEECTester(
            stl_file='tests/test_hpm/weixing_v1.stl',
            pfd_file='tests/test_hpm/weixing_v1_case.pfd',
            max_facets=50,  # Use fewer facets for testing
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
        
        # Debug the surface currents
        print(f"Surface currents type: {type(tester.surface_currents)}")
        print(f"Surface currents shape: {np.array(tester.surface_currents).shape}")
        print(f"Surface currents length: {len(tester.surface_currents)}")
        
        # Test the problematic comparison
        try:
            max_current = np.max(np.abs(tester.surface_currents))
            print(f"Max current: {max_current}")
            print(f"Max current > 0: {max_current > 0}")
        except Exception as e:
            print(f"❌ Error in max current calculation: {e}")
        
        # Test the current density heatmap
        try:
            fig, ax = plt.subplots(1, 1, figsize=(8, 6))
            
            # 投影到XY平面
            current_magnitudes = np.abs(tester.surface_currents)
            print(f"Current magnitudes type: {type(current_magnitudes)}")
            print(f"Current magnitudes shape: {current_magnitudes.shape}")
            
            # 创建二维直方图
            x_coords = []
            y_coords = []
            currents = []
            
            for i, rwg in enumerate(tester.rwg_functions):
                center_plus = np.mean(np.array(rwg['plus_triangle']['vertices']), axis=0)
                x_coords.append(center_plus[0])
                y_coords.append(center_plus[1])
                currents.append(current_magnitudes[i])
            
            print(f"Currents list length: {len(currents)}")
            print(f"Currents list type: {type(currents)}")
            print(f"Max of currents list: {max(currents) if currents else 'N/A'}")
            
            # Test the comparison
            if currents and len(currents) > 0 and np.max(currents) > 0:
                print("✅ Current density heatmap condition passed")
                hb = ax.hexbin(x_coords, y_coords, C=currents, gridsize=10, cmap='plasma')
                plt.colorbar(hb, ax=ax, label='Current Magnitude (A/m)')
            else:
                print("⚠️  Current density heatmap condition failed")
                ax.text(0.5, 0.5, 'No Current Data', ha='center', va='center', transform=ax.transAxes)
            
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_title('Current Density Distribution')
            
            plt.savefig('debug_current_density.png', dpi=150, bbox_inches='tight')
            plt.close()
            print("✅ Current density heatmap completed")
            
        except Exception as e:
            print(f"❌ Error in current density heatmap: {e}")
            import traceback
            traceback.print_exc()
        
        # Test the surface currents plotting
        try:
            fig = plt.figure(figsize=(8, 8))
            ax = fig.add_subplot(111, projection='3d')
            
            # 绘制电流矢量
            max_current = np.max(np.abs(tester.surface_currents))
            print(f"Max current for surface plotting: {max_current}")
            
            for i, rwg in enumerate(tester.rwg_functions[:min(20, len(tester.rwg_functions))]):
                current = tester.surface_currents[i]
                
                # 计算RWG中心位置
                center_plus = np.mean(np.array(rwg['plus_triangle']['vertices']), axis=0)
                center_minus = np.mean(np.array(rwg['minus_triangle']['vertices']), axis=0)
                center = (center_plus + center_minus) / 2
                
                # 电流方向
                direction = center_plus - center_minus
                if np.linalg.norm(direction) > 0:
                    direction = direction / np.linalg.norm(direction)
                
                # 绘制矢量
                vector_length = min(0.1, abs(current) / 1e4)
                if max_current > 0:
                    color_intensity = abs(current) / max_current
                else:
                    color_intensity = 0.5  # 默认颜色
                
                ax.quiver(center[0], center[1], center[2],
                         direction[0] * vector_length,
                         direction[1] * vector_length,
                         direction[2] * vector_length,
                         color=plt.cm.plasma(color_intensity))
            
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_zlabel('Z (m)')
            ax.set_title('Surface Currents (Limited to 20 vectors)')
            
            plt.savefig('debug_surface_currents.png', dpi=150, bbox_inches='tight')
            plt.close()
            print("✅ Surface currents plotting completed")
            
        except Exception as e:
            print(f"❌ Error in surface currents plotting: {e}")
            import traceback
            traceback.print_exc()
        
        return True
        
    except Exception as e:
        print(f"❌ Error in debug test: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)