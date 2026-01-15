#!/usr/bin/env python3
"""
Debug coordinate mapping and STL import
"""

import numpy as np

def debug_coordinate_mapping():
    """Debug the coordinate system and mapping"""
    print("🔍 Debugging Coordinate Mapping and STL Import")
    print("="*50)
    
    # From weixing_v1_case.pfd
    domain_size = np.array([3.4, 3.4, 1.4])  # 3400×3400×1400 mm = 3.4×3.4×1.4 m
    satellite_size = np.array([2.8, 2.8, 1.0])  # 2800×2800×1000 mm = 2.8×2.8×1.0 m
    grid_spacing = 0.02  # 20mm = 0.02m
    
    print(f"FDTD Configuration from weixing_v1_case.pfd:")
    print(f"  Domain size: {domain_size[0]:.1f}×{domain_size[1]:.1f}×{domain_size[2]:.1f} m")
    print(f"  Satellite size: {satellite_size[0]:.1f}×{satellite_size[1]:.1f}×{satellite_size[2]:.1f} m")
    print(f"  Grid spacing: {grid_spacing*1000:.0f} mm = {grid_spacing:.3f} m")
    
    # Calculate grid dimensions
    grid_dims = (domain_size / grid_spacing).astype(int)
    print(f"  Grid dimensions: {grid_dims[0]}×{grid_dims[1]}×{grid_dims[2]} = {np.prod(grid_dims):,} cells")
    
    # Expected satellite position (should be centered in domain)
    expected_satellite_center = domain_size / 2
    expected_satellite_bounds = np.array([
        expected_satellite_center[0] - satellite_size[0]/2,
        expected_satellite_center[1] - satellite_size[1]/2,
        expected_satellite_center[2] - satellite_size[2]/2,
        expected_satellite_center[0] + satellite_size[0]/2,
        expected_satellite_center[1] + satellite_size[1]/2,
        expected_satellite_center[2] + satellite_size[2]/2
    ])
    
    print(f"\nExpected satellite position (centered in domain):")
    print(f"  Center: [{expected_satellite_center[0]:.1f}, {expected_satellite_center[1]:.1f}, {expected_satellite_center[2]:.1f}] m")
    print(f"  Bounds: X=[{expected_satellite_bounds[0]:.1f}, {expected_satellite_bounds[3]:.1f}] m")
    print(f"          Y=[{expected_satellite_bounds[1]:.1f}, {expected_satellite_bounds[4]:.1f}] m")
    print(f"          Z=[{expected_satellite_bounds[2]:.1f}, {expected_satellite_bounds[5]:.1f}] m")
    
    # Check observation point coordinates from our simulation
    print(f"\nObservation points from simulation:")
    
    # From the simulation output, let's check some key points
    # These should match the coordinates we see in the HDF5 file
    key_points = {
        'center': [0.0, 0.0, 0.0],
        'satellite_surface_1': [0.7, 0.0, 0.56],
        'satellite_surface_2': [-0.7, 0.0, 0.56],
        'satellite_surface_3': [0.0, 0.7, 0.56],
        'satellite_surface_4': [0.0, -0.7, 0.56]
    }
    
    for name, point in key_points.items():
        print(f"  {name}: [{point[0]:.1f}, {point[1]:.1f}, {point[2]:.1f}] m")
        
        # Check if point is within expected satellite bounds
        in_satellite = (
            expected_satellite_bounds[0] <= point[0] <= expected_satellite_bounds[3] and
            expected_satellite_bounds[1] <= point[1] <= expected_satellite_bounds[4] and
            expected_satellite_bounds[2] <= point[2] <= expected_satellite_bounds[5]
        )
        print(f"    In satellite bounds: {in_satellite}")
    
    # Check plane monitoring coordinates
    print(f"\nPlane monitoring coordinates:")
    
    # X=0 plane: Y=[-1.5, 1.5], Z=[-0.55, 0.55] with 100×37 points
    y_range = np.linspace(-1.5, 1.5, 100)
    z_range = np.linspace(-0.55, 0.55, 37)
    print(f"  X=0 plane: Y=[{y_range[0]:.1f}, {y_range[-1]:.1f}] m, Z=[{z_range[0]:.1f}, {z_range[-1]:.1f}] m")
    print(f"    Resolution: Δy={(y_range[1]-y_range[0])*1000:.0f} mm, Δz={(z_range[1]-z_range[0])*1000:.0f} mm")
    
    # Check if this covers the satellite properly
    satellite_covered = (
        y_range[0] <= expected_satellite_bounds[1] and y_range[-1] >= expected_satellite_bounds[4] and
        z_range[0] <= expected_satellite_bounds[2] and z_range[-1] >= expected_satellite_bounds[5]
    )
    print(f"    Covers satellite: {satellite_covered}")
    
    # Volume monitoring coordinates
    print(f"\nVolume monitoring coordinates:")
    x_vol = np.linspace(-1.0, 1.0, 34)
    y_vol = np.linspace(-1.0, 1.0, 34)
    z_vol = np.linspace(-0.3, 0.8, 19)
    print(f"  Volume: X=[{x_vol[0]:.1f}, {x_vol[-1]:.1f}] m, Y=[{y_vol[0]:.1f}, {y_vol[-1]:.1f}] m, Z=[{z_vol[0]:.1f}, {z_vol[-1]:.1f}] m")
    print(f"    Resolution: Δx={(x_vol[1]-x_vol[0])*1000:.0f} mm, Δy={(y_vol[1]-y_vol[0])*1000:.0f} mm, Δz={(z_vol[1]-z_vol[0])*1000:.0f} mm")
    
    # Check material properties
    print(f"\nMaterial Properties Analysis:")
    print(f"  From weixing_v1_case.pfd: MATERIAL_DEFINE id=1 name=PEC epsr=1.0 mur=1.0 sigma=1e20")
    print(f"  This represents PEC (Perfect Electric Conductor):")
    print(f"    - epsr=1.0: Relative permittivity = 1 (same as vacuum)")
    print(f"    - mur=1.0: Relative permeability = 1 (same as vacuum)")
    print(f"    - sigma=1e20 S/m: Extremely high conductivity (>1e12 S/m indicates metal)")
    print(f"  For PEC material in FDTD:")
    print(f"    - Electric field inside conductor should be ~0")
    print(f"    - Fields should be reflected/scattered at surface")
    print(f"    - No field penetration into conductor")
    
    # Check if our simulation properly handles PEC
    print(f"\nSimulation Issues Identified:")
    print(f"  1. Field amplitudes too low (~0.01 V/m instead of ~1 V/m)")
    print(f"  2. Plane wave polarization not orthogonal to k-vector")
    print(f"  3. No apparent field scattering/reflection from satellite")
    print(f"  4. Coordinate system may need verification against STL geometry")

if __name__ == "__main__":
    debug_coordinate_mapping()