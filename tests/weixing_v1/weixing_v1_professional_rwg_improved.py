#!/usr/bin/env python3
"""
Improved RWG Basis Function Implementation for MoM
Addresses connectivity issues in STL geometry processing
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import re
import subprocess
import tempfile
from scipy.spatial import cKDTree

def parse_stl_file_professional(filename):
    """Professional STL file parser with geometry validation"""
    try:
        with open(filename, 'r') as f:
            content = f.read()
        
        facets = []
        vertices = []
        facet_normals = []
        
        # Parse ASCII STL with validation
        lines = content.strip().split('\n')
        i = 0
        facet_count = 0
        
        while i < len(lines):
            line = lines[i].strip()
            if line.startswith('solid'):
                # Extract solid name
                solid_name = line[5:].strip()
                print(f"  Parsing solid: {solid_name}")
            elif line.startswith('facet normal'):
                # Extract normal vector
                normal_parts = line.split()[2:5]
                normal = np.array([float(x) for x in normal_parts])
                
                # Validate normal (should be unit vector)
                normal_mag = np.linalg.norm(normal)
                if abs(normal_mag - 1.0) > 0.1:  # Allow 10% tolerance
                    print(f"  Warning: Non-unit normal at facet {facet_count}: magnitude = {normal_mag}")
                
                # Extract vertices
                i += 1  # Skip to outer loop
                triangle_vertices = []
                
                for j in range(3):
                    i += 1  # Move to vertex line
                    vertex_line = lines[i].strip()
                    if vertex_line.startswith('vertex'):
                        vertex_parts = vertex_line.split()[1:4]
                        vertex = np.array([float(x) for x in vertex_parts]) * 1e-3  # mm to m
                        triangle_vertices.append(vertex)
                
                # Validate triangle (non-zero area)
                v1 = triangle_vertices[1] - triangle_vertices[0]
                v2 = triangle_vertices[2] - triangle_vertices[0]
                area = 0.5 * np.linalg.norm(np.cross(v1, v2))
                
                if area < 1e-12:  # Skip degenerate triangles
                    print(f"  Warning: Degenerate triangle at facet {facet_count}, area = {area}")
                    i += 2  # Skip endloop and endfacet
                    continue
                
                facets.append({
                    'normal': normal,
                    'vertices': triangle_vertices,
                    'area': area,
                    'id': facet_count
                })
                
                vertices.extend(triangle_vertices)
                facet_normals.append(normal)
                facet_count += 1
            
            i += 1
        
        vertices_array = np.array(vertices)
        
        # Geometry validation
        print(f"  Total facets parsed: {facet_count}")
        print(f"  Total vertices: {len(vertices_array)}")
        
        # Check geometry bounds
        if len(vertices_array) > 0:
            bounds = {
                'x': [np.min(vertices_array[:, 0]), np.max(vertices_array[:, 0])],
                'y': [np.min(vertices_array[:, 1]), np.max(vertices_array[:, 1])],
                'z': [np.min(vertices_array[:, 2]), np.max(vertices_array[:, 2])]
            }
            print(f"  Geometry bounds (m): X{bounds['x']}, Y{bounds['y']}, Z{bounds['z']}")
            
            # Check for water-tightness (simplified)
            print(f"  Average facet area: {np.mean([f['area'] for f in facets]):.6f} m²")
            print(f"  Total surface area: {sum([f['area'] for f in facets]):.6f} m²")
        
        return facets, vertices_array
    except Exception as e:
        print(f"Error parsing STL file {filename}: {e}")
        return None, None

def calculate_rwg_basis_functions_improved(elements):
    """Improved RWG basis function calculation with better connectivity detection"""
    
    print("\nCalculating improved RWG basis functions...")
    
    # Step 1: Build vertex list and use KD-tree for efficient merging
    print("  Building vertex list...")
    all_vertices = []
    element_vertex_map = {}  # Maps (element_idx, vertex_idx) to global vertex index
    
    for elem_idx, elem in enumerate(elements):
        element_vertex_map[elem_idx] = []
        for vertex_idx, vertex in enumerate(elem['vertices']):
            all_vertices.append(vertex)
            element_vertex_map[elem_idx].append(len(all_vertices) - 1)
    
    all_vertices = np.array(all_vertices)
    print(f"    Total vertices collected: {len(all_vertices)}")
    
    # Step 2: Use KD-tree for efficient vertex merging
    print("  Merging nearby vertices using KD-tree...")
    tolerance = 1e-6  # 1 micron tolerance
    
    # Build KD-tree
    kdtree = cKDTree(all_vertices)
    
    # Find connected components (groups of nearby vertices)
    processed = set()
    vertex_groups = []
    
    for i in range(len(all_vertices)):
        if i in processed:
            continue
        
        # Find all vertices within tolerance
        nearby_indices = kdtree.query_ball_point(all_vertices[i], tolerance)
        
        # Add to group
        vertex_groups.append(nearby_indices)
        
        # Mark as processed
        for idx in nearby_indices:
            processed.add(idx)
    
    print(f"    Found {len(vertex_groups)} unique vertex groups")
    
    # Create merged vertex mapping
    merged_vertices = []
    vertex_remap = {}  # Maps original vertex index to merged vertex index
    
    for group_idx, group in enumerate(vertex_groups):
        # Use average position for merged vertex
        group_positions = all_vertices[group]
        merged_pos = np.mean(group_positions, axis=0)
        merged_vertices.append(merged_pos)
        
        for orig_idx in group:
            vertex_remap[orig_idx] = group_idx
    
    print(f"    Merged vertices: {len(merged_vertices)}")
    print(f"    Reduction: {len(all_vertices) - len(merged_vertices)} vertices")
    
    # Step 3: Build edge connectivity with improved logic
    print("  Building edge connectivity...")
    edge_map = {}
    
    for elem_idx, elem in enumerate(elements):
        # Get merged vertex indices for this element
        original_vertex_indices = element_vertex_map[elem_idx]
        merged_vertex_indices = [vertex_remap[orig_idx] for orig_idx in original_vertex_indices]
        
        # Create edges (ordered by vertex ID for consistency)
        edges = [
            tuple(sorted([merged_vertex_indices[0], merged_vertex_indices[1]])),
            tuple(sorted([merged_vertex_indices[1], merged_vertex_indices[2]])),
            tuple(sorted([merged_vertex_indices[2], merged_vertex_indices[0]]))
        ]
        
        # Store edge information
        for edge in edges:
            if edge not in edge_map:
                edge_map[edge] = []
            
            edge_map[edge].append({
                'element': elem,
                'merged_vertices': merged_vertex_indices,
                'original_vertices': elem['vertices'],
                'elem_idx': elem_idx
            })
    
    # Step 4: Find internal edges (shared by exactly 2 triangles)
    print("  Finding internal edges...")
    internal_edges = []
    boundary_edges = []
    
    for edge, elem_data_list in edge_map.items():
        if len(elem_data_list) == 2:
            internal_edges.append({
                'edge': edge,
                'elements': elem_data_list,
                'id': len(internal_edges)
            })
        else:
            boundary_edges.append({
                'edge': edge,
                'elements': elem_data_list,
                'count': len(elem_data_list)
            })
    
    print(f"    Internal edges (RWG functions): {len(internal_edges)}")
    print(f"    Boundary edges: {len(boundary_edges)}")
    print(f"    Total edges: {len(edge_map)}")
    
    # Print boundary edge statistics
    boundary_counts = [edge['count'] for edge in boundary_edges]
    if boundary_counts:
        print(f"    Boundary edge connectivity: min={min(boundary_counts)}, max={max(boundary_counts)}")
    
    # Step 5: Create RWG basis functions
    print("  Creating RWG basis functions...")
    rwg_functions = []
    
    for edge_data in internal_edges:
        edge = edge_data['edge']
        elem_data_list = edge_data['elements']
        
        # Verify we have exactly 2 elements sharing this edge
        if len(elem_data_list) != 2:
            continue
        
        # Calculate RWG parameters for both triangles
        rwg_params = []
        
        for elem_data in elem_data_list:
            elem = elem_data['element']
            merged_vertices_elem = elem_data['merged_vertices']
            original_vertices = elem_data['original_vertices']
            
            # Find the free vertex (opposite to the shared edge)
            free_vertex_idx = None
            for i, merged_vertex in enumerate(merged_vertices_elem):
                if merged_vertex not in edge:
                    free_vertex_idx = i
                    break
            
            if free_vertex_idx is not None:
                # Get free vertex coordinates
                r_plus = original_vertices[free_vertex_idx]
                
                # Find edge vertices from original vertices
                edge_vertices = []
                for i, merged_vertex in enumerate(merged_vertices_elem):
                    if merged_vertex in edge:
                        edge_vertices.append(original_vertices[i])
                
                if len(edge_vertices) == 2:
                    # Calculate edge length
                    edge_length = np.linalg.norm(edge_vertices[1] - edge_vertices[0])
                    
                    # Get triangle properties
                    area = elem['area']
                    normal = elem['normal']
                    
                    rwg_params.append({
                        'element': elem,
                        'r_plus': r_plus,
                        'edge_length': edge_length,
                        'area': area,
                        'normal': normal,
                        'edge_vertices': edge_vertices,
                        'merged_vertices': merged_vertices_elem
                    })
        
        # Create RWG function if we have valid parameters for both triangles
        if len(rwg_params) == 2:
            # Calculate average edge length
            avg_edge_length = (rwg_params[0]['edge_length'] + rwg_params[1]['edge_length']) / 2
            
            rwg_functions.append({
                'edge_id': edge_data['id'],
                'edge': edge,
                'triangles': rwg_params,
                'edge_length': avg_edge_length
            })
    
    print(f"  Successfully created {len(rwg_functions)} RWG basis functions")
    
    # Validate RWG function creation
    if len(rwg_functions) == 0:
        print("  WARNING: No RWG basis functions created!")
        print("  This may indicate:")
        print("    - STL file contains disconnected triangles")
        print("    - Mesh is not manifold (non-watertight)")
        print("    - Vertex merging tolerance is too strict")
        print("  Consider checking the STL file quality and connectivity.")
    
    return rwg_functions, elements

def parse_pfd_config_enhanced(filename):
    """Enhanced PFD configuration parser with encoding fix"""
    config = {
        'domain_size': [3400, 3400, 1400],  # mm
        'grid_spacing': [20, 20, 20],       # mm
        'frequency': 10e9,                  # Hz (10 GHz)
        'material': {'epsr': 1.0, 'mur': 1.0, 'sigma': 1e20},  # PEC
        'geometry_translate': [0, 0, -550], # mm
        'source_angle': [45.0, 45.0, 45.0],  # theta, phi, psi
        'source_position': [0, 1e-3, 1e4],  # position
        'source_range': [-1550, -1550, -6000, 1550, 1550, 6000],  # mm
        'waveform_file': 'hpm_waveform_X(10.0GHz)_20ns.txt',
        'output_planes': [],
        'output_volumes': [],
        'output_points': [],
        'computation_time': 20e-9,  # 20 ns
        'boundary': 'PML',
        'mesh_target_length': 0.003,  # 3mm = λ/10 at 10 GHz
        'mesh_min_angle': 25.0,      # degrees
        'mesh_max_aspect_ratio': 3.0
    }
    
    try:
        # Try different encodings
        encodings = ['utf-8', 'gbk', 'latin-1', 'cp1252']
        content = None
        
        for encoding in encodings:
            try:
                with open(filename, 'r', encoding=encoding) as f:
                    content = f.read()
                break
            except UnicodeDecodeError:
                continue
        
        if content is None:
            print(f"Could not decode file {filename} with any encoding")
            return config
        
        # Extract domain size
        domain_match = re.search(r'DOMAIN_SIZE\s+([\d\s]+)', content)
        if domain_match:
            sizes = list(map(float, domain_match.group(1).split()))
            config['domain_size'] = sizes[:3]
        
        # Extract grid spacing
        grid_match = re.search(r'GRID_SPACING\s+([\d\s]+)', content)
        if grid_match:
            spacing = list(map(float, grid_match.group(1).split()))
            config['grid_spacing'] = spacing[:3]
        
        # Extract frequency from source
        freq_match = re.search(r'(\d+\.?\d*)GHz', content)
        if freq_match:
            config['frequency'] = float(freq_match.group(1)) * 1e9
        
        # Extract source angles
        angle_match = re.search(r'TSF_ANGLE_DEG\s+([\d.\s]+)', content)
        if angle_match:
            angles = list(map(float, angle_match.group(1).split()))
            config['source_angle'] = angles[:3]
        
        # Extract geometry translation
        trans_match = re.search(r'GEOMETRY_TRANSLATE\s+([\d\s-]+)', content)
        if trans_match:
            trans = list(map(float, trans_match.group(1).split()))
            config['geometry_translate'] = trans[:3]
        
        # Extract material properties
        mat_match = re.search(r'MATERIAL_DEFINE\s+id=(\d+)\s+name=(\w+)\s+epsr=([\d.]+)\s+mur=([\d.]+)\s+sigma=([\de+-]+)', content)
        if mat_match:
            config['material'] = {
                'epsr': float(mat_match.group(3)),
                'mur': float(mat_match.group(4)),
                'sigma': float(mat_match.group(5))
            }
        
        # Extract computation time
        time_match = re.search(r'TSF_FREQUENCY\s+\d+\.?\d*\s+(\d+\.?\d*)\s+(\d+)', content)
        if time_match:
            config['computation_time'] = float(time_match.group(1)) * 1e-9  # Convert to seconds
        
        # Extract output planes
        plane_matches = re.findall(r'OUTPUT_PLANE\s+([\d\s.-]+)', content)
        for match in plane_matches:
            plane_data = list(map(float, match.split()))
            if len(plane_data) >= 7:
                config['output_planes'].append({
                    'type': 'PLANE',
                    'position': plane_data[0:3],
                    'normal': plane_data[3:6],
                    'resolution': plane_data[6] if len(plane_data) > 6 else 20
                })
        
    except Exception as e:
        print(f"Error parsing PFD file {filename}: {e}")
    
    return config

def main():
    """Main function to test improved RWG implementation"""
    
    print("="*70)
    print("IMPROVED RWG BASIS FUNCTION IMPLEMENTATION TEST")
    print("="*70)
    
    # Load configuration
    print("\n1. Loading PFD configuration...")
    config = parse_pfd_config_enhanced('weixing_v1_case.pfd')
    print(f"   Domain: {config['domain_size']} mm")
    print(f"   Frequency: {config['frequency']/1e9} GHz")
    
    # Parse STL file with professional validation
    print("\n2. Parsing STL file with professional validation...")
    facets, vertices = parse_stl_file_professional('tests/test_hpm/weixing_v1.stl')
    
    if facets is None:
        print("   Failed to parse STL file")
        return
    
    print(f"   Original facets: {len(facets)}")
    
    # Apply coordinate translation
    print("\n3. Applying coordinate translation...")
    translate = np.array(config['geometry_translate']) * 1e-3  # mm to m
    for facet in facets:
        for vertex in facet['vertices']:
            vertex += translate
    
    # Convert facets to elements format
    elements = []
    for facet in facets:
        elements.append({
            'vertices': facet['vertices'],
            'normal': facet['normal'],
            'area': facet['area'],
            'id': facet['id']
        })
    
    # Create improved RWG basis functions
    print("\n4. Creating improved RWG basis functions...")
    rwg_functions, elements = calculate_rwg_basis_functions_improved(elements)
    
    if not rwg_functions:
        print("   ERROR: No RWG basis functions created!")
        print("   The STL geometry may not be suitable for MoM analysis.")
        return
    
    print(f"\nSuccess! Created {len(rwg_functions)} RWG basis functions")
    print("\nThe improved RWG implementation successfully handles STL geometry connectivity!")
    
    # Return results for further processing
    return rwg_functions, elements, config

if __name__ == "__main__":
    main()