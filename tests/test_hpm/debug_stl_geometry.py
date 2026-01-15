#!/usr/bin/env python3
"""
Debug STL geometry import and coordinate system
"""

import numpy as np
import struct

def read_stl_binary(filename):
    """Read binary STL file"""
    try:
        with open(filename, 'rb') as f:
            # Read header (80 bytes)
            header = f.read(80)
            
            # Read number of triangles (4 bytes)
            num_triangles_bytes = f.read(4)
            num_triangles = struct.unpack('<I', num_triangles_bytes)[0]
            
            print(f"STL File: {filename}")
            print(f"  Header: {header[:50]}...")
            print(f"  Number of triangles: {num_triangles:,}")
            
            # Read triangles
            vertices = []
            normals = []
            
            for i in range(min(num_triangles, 10)):  # Read first 10 triangles
                # Each triangle: normal(3×4) + vertices(3×3×4) + attribute(2)
                triangle_data = f.read(50)
                
                if len(triangle_data) < 50:
                    break
                    
                # Unpack data
                normal = struct.unpack('<3f', triangle_data[0:12])
                v1 = struct.unpack('<3f', triangle_data[12:24])
                v2 = struct.unpack('<3f', triangle_data[24:36])
                v3 = struct.unpack('<3f', triangle_data[36:48])
                
                normals.append(normal)
                vertices.extend([v1, v2, v3])
                
                if i < 3:  # Show first 3 triangles
                    print(f"  Triangle {i+1}:")
                    print(f"    Normal: [{normal[0]:.3f}, {normal[1]:.3f}, {normal[2]:.3f}]")
                    print(f"    V1: [{v1[0]:.3f}, {v1[1]:.3f}, {v1[2]:.3f}]")
                    print(f"    V2: [{v2[0]:.3f}, {v2[1]:.3f}, {v2[2]:.3f}]")
                    print(f"    V3: [{v3[0]:.3f}, {v3[1]:.3f}, {v3[2]:.3f}]")
            
            # Convert to numpy array
            vertices = np.array(vertices)
            
            # Calculate bounds
            if len(vertices) > 0:
                min_bounds = np.min(vertices, axis=0)
                max_bounds = np.max(vertices, axis=0)
                center = (min_bounds + max_bounds) / 2
                size = max_bounds - min_bounds
                
                print(f"\n  Geometry Analysis (first {len(vertices)} vertices):")
                print(f"    Bounds: X=[{min_bounds[0]:.3f}, {max_bounds[0]:.3f}] m")
                print(f"            Y=[{min_bounds[1]:.3f}, {max_bounds[1]:.3f}] m")
                print(f"            Z=[{min_bounds[2]:.3f}, {max_bounds[2]:.3f}] m")
                print(f"    Center: [{center[0]:.3f}, {center[1]:.3f}, {center[2]:.3f}] m")
                print(f"    Size: [{size[0]:.3f}, {size[1]:.3f}, {size[2]:.3f}] m")
                
                return vertices, normals, center, size
            
    except Exception as e:
        print(f"Error reading STL file: {e}")
        return None, None, None, None

def debug_stl_geometry():
    """Debug STL geometry import"""
    print("🔍 Debugging STL Geometry Import")
    print("="*50)
    
    stl_file = "weixing_v1.stl"
    
    # Read the STL file
    vertices, normals, center, size = read_stl_binary(stl_file)
    
    if vertices is not None:
        print(f"\nSTL Geometry Summary:")
        print(f"  Total vertices: {len(vertices):,}")
        print(f"  Geometry center: [{center[0]:.3f}, {center[1]:.3f}, {center[2]:.3f}] m")
        print(f"  Geometry size: [{size[0]:.3f}, {size[1]:.3f}, {size[2]:.3f}] m")
        
        # Compare with expected satellite size from .pfd file
        expected_size = np.array([2.8, 2.8, 1.0])  # meters
        expected_center = np.array([1.7, 1.7, 0.7])  # centered in domain
        
        print(f"\nComparison with weixing_v1_case.pfd:")
        print(f"  Expected satellite size: [{expected_size[0]:.1f}, {expected_size[1]:.1f}, {expected_size[2]:.1f}] m")
        print(f"  Actual STL size:       [{size[0]:.3f}, {size[1]:.3f}, {size[2]:.3f}] m")
        print(f"  Size ratio: [{size[0]/expected_size[0]:.3f}, {size[1]/expected_size[1]:.3f}, {size[2]/expected_size[2]:.3f}]")
        
        print(f"\n  Expected satellite center: [{expected_center[0]:.1f}, {expected_center[1]:.1f}, {expected_center[2]:.1f}] m")
        print(f"  Actual STL center:       [{center[0]:.3f}, {center[1]:.3f}, {center[2]:.3f}] m")
        print(f"  Center offset: [{center[0]-expected_center[0]:.3f}, {center[1]-expected_center[1]:.3f}, {center[2]-expected_center[2]:.3f}] m")
        
        # Check if STL is in correct units (mm vs m)
        if np.max(size) > 100:  # If any dimension > 100m, likely in mm
            print(f"\n⚠️  WARNING: STL appears to be in millimeters!")
            print(f"  Max dimension: {np.max(size):.1f} m (should be ~2.8 m)")
            print(f"  Suggested conversion: divide by 1000")
            
            # Show converted values
            converted_vertices = vertices / 1000
            converted_center = center / 1000
            converted_size = size / 1000
            
            print(f"\n  Converted (mm→m):")
            print(f"    Center: [{converted_center[0]:.3f}, {converted_center[1]:.3f}, {converted_center[2]:.3f}] m")
            print(f"    Size:   [{converted_size[0]:.3f}, {converted_size[1]:.3f}, {converted_size[2]:.3f}] m")
            
        else:
            print(f"\n✅ STL appears to be in correct units (meters)")
            
        # Check coordinate system alignment
        print(f"\nCoordinate System Analysis:")
        print(f"  FDTD domain: [0,0,0] to [{domain_size[0]:.1f}, {domain_size[1]:.1f}, {domain_size[2]:.1f}] m")
        print(f"  Satellite should be centered at: [{expected_center[0]:.1f}, {expected_center[1]:.1f}, {expected_center[2]:.1f}] m")
        
        # Calculate required translation
        if np.max(size) > 100:  # If in mm
            required_translation = expected_center - converted_center
            print(f"  Required translation (mm→m): [{required_translation[0]:.3f}, {required_translation[1]:.3f}, {required_translation[2]:.3f}] m")
        else:
            required_translation = expected_center - center
            print(f"  Required translation: [{required_translation[0]:.3f}, {required_translation[1]:.3f}, {required_translation[2]:.3f}] m")
            
    else:
        print("Failed to read STL file")

if __name__ == "__main__":
    debug_stl_geometry()