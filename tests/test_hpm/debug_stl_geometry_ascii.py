#!/usr/bin/env python3
"""
Debug STL geometry import - try ASCII format
"""

import numpy as np
import re

def read_stl_ascii(filename):
    """Read ASCII STL file"""
    try:
        with open(filename, 'r') as f:
            content = f.read()
            
        print(f"STL File: {filename}")
        print(f"  File size: {len(content):,} characters")
        
        # Parse vertices using regex
        vertex_pattern = r'vertex\s+([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)\s+([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)\s+([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)'
        vertices = []
        
        for match in re.finditer(vertex_pattern, content, re.IGNORECASE):
            x, y, z = float(match.group(1)), float(match.group(2)), float(match.group(3))
            vertices.append([x, y, z])
        
        if vertices:
            vertices = np.array(vertices)
            
            # Calculate bounds
            min_bounds = np.min(vertices, axis=0)
            max_bounds = np.max(vertices, axis=0)
            center = (min_bounds + max_bounds) / 2
            size = max_bounds - min_bounds
            
            print(f"\n  Geometry Analysis ({len(vertices):,} vertices):")
            print(f"    Bounds: X=[{min_bounds[0]:.3f}, {max_bounds[0]:.3f}] m")
            print(f"            Y=[{min_bounds[1]:.3f}, {max_bounds[1]:.3f}] m")
            print(f"            Z=[{min_bounds[2]:.3f}, {max_bounds[2]:.3f}] m")
            print(f"    Center: [{center[0]:.3f}, {center[1]:.3f}, {center[2]:.3f}] m")
            print(f"    Size: [{size[0]:.3f}, {size[1]:.3f}, {size[2]:.3f}] m")
            
            return vertices, center, size
        else:
            print("No vertices found in STL file")
            return None, None, None
            
    except Exception as e:
        print(f"Error reading STL file: {e}")
        return None, None, None

def debug_stl_geometry_ascii():
    """Debug STL geometry import using ASCII parsing"""
    print("🔍 Debugging STL Geometry Import (ASCII)")
    print("="*50)
    
    # FDTD Configuration from weixing_v1_case.pfd
    domain_size = np.array([3.4, 3.4, 1.4])  # 3400×3400×1400 mm = 3.4×3.4×1.4 m
    satellite_size_expected = np.array([2.8, 2.8, 1.0])  # 2800×2800×1000 mm = 2.8×2.8×1.0 m
    grid_spacing = 0.02  # 20mm = 0.02m
    
    print(f"FDTD Configuration from weixing_v1_case.pfd:")
    print(f"  Domain size: {domain_size[0]:.1f}×{domain_size[1]:.1f}×{domain_size[2]:.1f} m")
    print(f"  Expected satellite size: {satellite_size_expected[0]:.1f}×{satellite_size_expected[1]:.1f}×{satellite_size_expected[2]:.1f} m")
    print(f"  Grid spacing: {grid_spacing*1000:.0f} mm = {grid_spacing:.3f} m")
    
    stl_file = "weixing_v1.stl"
    
    # Read the STL file as ASCII
    vertices, center, size = read_stl_ascii(stl_file)
    
    if vertices is not None:
        print(f"\nSTL Geometry Summary:")
        print(f"  Total vertices: {len(vertices):,}")
        print(f"  Geometry center: [{center[0]:.3f}, {center[1]:.3f}, {center[2]:.3f}] m")
        print(f"  Geometry size: [{size[0]:.3f}, {size[1]:.3f}, {size[2]:.3f}] m")
        
        # Check if units are correct
        print(f"\nUnit Analysis:")
        if np.max(size) < 10:  # If max dimension < 10m, likely in meters
            print(f"  ✅ STL appears to be in meters (max dim: {np.max(size):.1f} m)")
            stl_in_meters = True
        elif np.max(size) < 10000:  # If max dimension < 10km, likely in mm
            print(f"  ⚠️  STL appears to be in millimeters (max dim: {np.max(size):.1f} mm)")
            print(f"  Suggested: divide by 1000 to convert to meters")
            stl_in_meters = False
        else:
            print(f"  ❌ STL units unclear (max dim: {np.max(size):.1f})")
            stl_in_meters = False
        
        # Convert to meters if needed
        if not stl_in_meters:
            converted_vertices = vertices / 1000
            converted_center = center / 1000
            converted_size = size / 1000
            
            print(f"\n  Converted to meters:")
            print(f"    Center: [{converted_center[0]:.3f}, {converted_center[1]:.3f}, {converted_center[2]:.3f}] m")
            print(f"    Size:   [{converted_size[0]:.3f}, {converted_size[1]:.3f}, {converted_size[2]:.3f}] m")
            
            # Use converted values for further analysis
            center = converted_center
            size = converted_size
        
        # Compare with expected satellite size
        print(f"\nComparison with expected size:")
        print(f"  Expected satellite size: [{satellite_size_expected[0]:.1f}, {satellite_size_expected[1]:.1f}, {satellite_size_expected[2]:.1f}] m")
        print(f"  Actual STL size:         [{size[0]:.3f}, {size[1]:.3f}, {size[2]:.3f}] m")
        print(f"  Size ratio: [{size[0]/satellite_size_expected[0]:.3f}, {size[1]/satellite_size_expected[1]:.3f}, {size[2]/satellite_size_expected[2]:.3f}]")
        
        # Expected position in FDTD domain (centered)
        expected_center = domain_size / 2
        print(f"\nExpected position in FDTD domain:")
        print(f"  Expected center: [{expected_center[0]:.1f}, {expected_center[1]:.1f}, {expected_center[2]:.1f}] m")
        print(f"  Actual center:   [{center[0]:.3f}, {center[1]:.3f}, {center[2]:.3f}] m")
        print(f"  Center offset:   [{center[0]-expected_center[0]:.3f}, {center[1]-expected_center[1]:.3f}, {center[2]-expected_center[2]:.3f}] m")
        
        # Check if satellite is properly positioned
        tolerance = 0.5  # 0.5m tolerance
        position_ok = np.all(np.abs(center - expected_center) < tolerance)
        size_ok = np.all(np.abs(size - satellite_size_expected) < tolerance)
        
        print(f"\nValidation Results:")
        print(f"  Position OK (within {tolerance}m): {position_ok}")
        print(f"  Size OK (within {tolerance}m): {size_ok}")
        
        if not position_ok or not size_ok:
            print(f"\n⚠️  Issues detected:")
            if not position_ok:
                print(f"    - Satellite not centered in domain")
                print(f"    - Required translation: {expected_center - center}")
            if not size_ok:
                print(f"    - Satellite size doesn't match expected dimensions")
                
        return center, size, stl_in_meters
        
    else:
        print("Failed to read STL file")
        return None, None, None

if __name__ == "__main__":
    debug_stl_geometry_ascii()