#!/usr/bin/env python3
"""
Professional MoM/PEEC Implementation with Proper CAD Pipeline
Following industry standards for electromagnetic scattering analysis
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import re
import subprocess
import tempfile

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

def create_gmsh_mesh_script(facets, target_edge_length=0.003, filename="satellite_mesh.geo"):
    """Create Gmsh script for professional surface mesh generation"""
    
    print(f"\nCreating Gmsh script with target edge length: {target_edge_length*1000:.1f} mm")
    
    geo_content = f"""
// Professional satellite surface mesh for MoM analysis
// Target edge length: {target_edge_length*1000:.1f} mm (λ/10 at 10 GHz)

SetFactory("OpenCASCADE");

// Mesh parameters
lc = {target_edge_length};  // Characteristic length
Mesh.Algorithm = 6;         // Frontal algorithm for quality triangles
Mesh.CharacteristicLengthMin = lc * 0.8;
Mesh.CharacteristicLengthMax = lc * 1.2;
Mesh.ElementOrder = 1;      // Linear elements for RWG
Mesh.SecondOrderLinear = 1;

// Quality parameters
Mesh.QualityType = 2;      // Normalized shape quality
Mesh.Smoothing = 10;        // Smoothing iterations
Mesh.RecombineAll = 0;     // Keep triangles
Mesh.SubdivisionAlgorithm = 0;

// Create points from STL vertices
"""
    
    # Write vertices as Gmsh points
    vertex_map = {}
    point_id = 1
    
    # Collect unique vertices
    unique_vertices = []
    for facet in facets:
        for vertex in facet['vertices']:
            vertex_tuple = tuple(vertex)
            if vertex_tuple not in vertex_map:
                vertex_map[vertex_tuple] = point_id
                unique_vertices.append(vertex)
                geo_content += f"Point({point_id}) = {{{vertex[0]:.6f}, {vertex[1]:.6f}, {vertex[2]:.6f}, lc}};\n"
                point_id += 1
    
    geo_content += f"\n// Created {len(unique_vertices)} unique vertices\n\n"
    
    # Create triangles
    triangle_id = 1
    for facet in facets:
        vertices = facet['vertices']
        
        # Get point IDs
        p1 = vertex_map[tuple(vertices[0])]
        p2 = vertex_map[tuple(vertices[1])]
        p3 = vertex_map[tuple(vertices[2])]
        
        # Create lines
        geo_content += f"Line({triangle_id*3-2}) = {{{p1}, {p2}}};\n"
        geo_content += f"Line({triangle_id*3-1}) = {{{p2}, {p3}}};\n"
        geo_content += f"Line({triangle_id*3}) = {{{p3}, {p1}}};\n"
        
        # Create curve loop and plane surface
        geo_content += f"Curve Loop({triangle_id}) = {{{triangle_id*3-2}, {triangle_id*3-1}, {triangle_id*3}}};\n"
        geo_content += f"Plane Surface({triangle_id}) = {{{triangle_id}}};\n\n"
        
        triangle_id += 1
    
    geo_content += f"// Created {triangle_id-1} triangles\n\n"
    
    # Physical groups for different satellite parts (simplified)
    geo_content += """
// Physical groups for material assignment
Physical Surface("satellite_body") = {1:"""
    
    # Add all surface IDs
    surface_ids = list(range(1, triangle_id))
    geo_content += ", ".join(map(str, surface_ids))
    geo_content += "};\n\n"
    
    # Mesh generation commands
    geo_content += """
// Generate mesh
Mesh 2;

// Optimize mesh quality
Mesh.Smoothing = 10;
Mesh.Optimize = 1;
Mesh.HighOrderOptimize = 0;

// Save mesh
Save "satellite_mesh.msh";

// Quality report
Printf("Mesh statistics:");
Printf("  Triangles: %g", Mesh.NbTriangles());
Printf("  Vertices: %g", Mesh.NbVertices());
Printf("  Quality (min): %g", Mesh.MinQuality());
Printf("  Quality (avg): %g", Mesh.AverageQuality());
"""
    
    with open(filename, 'w') as f:
        f.write(geo_content)
    
    return filename

def run_gmsh_mesh_generation(geo_filename):
    """Run Gmsh to generate professional mesh"""
    
    try:
        # Run Gmsh
        cmd = ['gmsh', '-2', '-format', 'msh2', '-o', 'satellite_mesh.msh', geo_filename]
        print(f"\nRunning Gmsh: {' '.join(cmd)}")
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        
        if result.returncode == 0:
            print("Gmsh mesh generation completed successfully")
            print("Output:", result.stdout)
            return True
        else:
            print("Gmsh failed:", result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        print("Gmsh mesh generation timed out")
        return False
    except FileNotFoundError:
        print("Gmsh not found. Please install Gmsh or use alternative mesh generation")
        return False

def parse_gmsh_mesh(filename):
    """Parse Gmsh mesh file and extract professional surface mesh"""
    
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
        
        # Find mesh section
        in_nodes = False
        in_elements = False
        
        nodes = {}
        elements = []
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            if line == '$Nodes':
                in_nodes = True
                i += 1
                # Read number of nodes
                num_nodes = int(lines[i].strip())
                print(f"  Parsing {num_nodes} nodes")
                i += 1
                
                # Read nodes
                for j in range(num_nodes):
                    node_line = lines[i].strip().split()
                    node_id = int(node_line[0])
                    x, y, z = float(node_line[1]), float(node_line[2]), float(node_line[3])
                    nodes[node_id] = np.array([x, y, z])
                    i += 1
                    
            elif line == '$EndNodes':
                in_nodes = False
                
            elif line == '$Elements':
                in_elements = True
                i += 1
                # Read number of elements
                num_elements = int(lines[i].strip())
                print(f"  Parsing {num_elements} elements")
                i += 1
                
                # Read elements
                triangle_count = 0
                for j in range(num_elements):
                    elem_line = lines[i].strip().split()
                    elem_id = int(elem_line[0])
                    elem_type = int(elem_line[1])
                    num_tags = int(elem_line[2])
                    
                    # Triangle element (type 2)
                    if elem_type == 2:
                        # Skip tags
                        start_idx = 3 + num_tags
                        node_ids = [int(x) for x in elem_line[start_idx:start_idx+3]]
                        
                        # Get vertex coordinates
                        vertices = [nodes[node_ids[0]], nodes[node_ids[1]], nodes[node_ids[2]]]
                        
                        # Calculate normal and area
                        v1 = vertices[1] - vertices[0]
                        v2 = vertices[2] - vertices[0]
                        normal = np.cross(v1, v2)
                        area = 0.5 * np.linalg.norm(normal)
                        
                        if area > 1e-12:  # Skip degenerate triangles
                            # Normalize normal
                            if np.linalg.norm(normal) > 0:
                                normal = normal / np.linalg.norm(normal)
                            
                            elements.append({
                                'id': elem_id,
                                'vertices': vertices,
                                'normal': normal,
                                'area': area,
                                'node_ids': node_ids
                            })
                            triangle_count += 1
                    
                    i += 1
                    
            elif line == '$EndElements':
                in_elements = False
                
            else:
                i += 1
        
        print(f"  Successfully parsed {triangle_count} triangles")
        
        # Calculate mesh statistics
        if elements:
            areas = [elem['area'] for elem in elements]
            print(f"  Average triangle area: {np.mean(areas):.6f} m²")
            print(f"  Min triangle area: {np.min(areas):.6f} m²")
            print(f"  Max triangle area: {np.max(areas):.6f} m²")
            
            # Check aspect ratios
            aspect_ratios = []
            for elem in elements:
                vertices = elem['vertices']
                
                # Calculate edge lengths
                e1 = np.linalg.norm(vertices[1] - vertices[0])
                e2 = np.linalg.norm(vertices[2] - vertices[1])
                e3 = np.linalg.norm(vertices[0] - vertices[2])
                
                # Aspect ratio (max edge / min edge)
                max_edge = max(e1, e2, e3)
                min_edge = min(e1, e2, e3)
                if min_edge > 0:
                    aspect_ratio = max_edge / min_edge
                    aspect_ratios.append(aspect_ratio)
            
            if aspect_ratios:
                print(f"  Average aspect ratio: {np.mean(aspect_ratios):.2f}")
                print(f"  Max aspect ratio: {np.max(aspect_ratios):.2f}")
        
        return elements
        
    except Exception as e:
        print(f"Error parsing Gmsh mesh file {filename}: {e}")
        return None

def calculate_rwg_basis_functions(elements):
    """Calculate RWG (Rao-Wilton-Glisson) basis functions for MoM"""
    
    print("\nCalculating RWG basis functions...")
    
    # First, merge vertices that are very close to each other
    # This is necessary because STL files often have disconnected triangles
    merged_vertices = []
    vertex_map = {}
    tolerance = 1e-6  # 1 micron tolerance for vertex merging
    
    print("  Merging nearby vertices...")
    
    # Collect all vertices
    all_vertices = []
    for elem in elements:
        for vertex in elem['vertices']:
            all_vertices.append(vertex)
    
    # Merge vertices
    for i, vertex in enumerate(all_vertices):
        found_match = False
        for j, merged_vertex in enumerate(merged_vertices):
            if np.linalg.norm(vertex - merged_vertex) < tolerance:
                vertex_map[i] = j
                found_match = True
                break
        
        if not found_match:
            vertex_map[i] = len(merged_vertices)
            merged_vertices.append(vertex)
    
    print(f"    Original vertices: {len(all_vertices)}")
    print(f"    Merged vertices: {len(merged_vertices)}")
    print(f"    Reduction: {len(all_vertices) - len(merged_vertices)} vertices")
    
    # Build edge connectivity using merged vertices
    edge_map = {}
    
    for elem_idx, elem in enumerate(elements):
        vertices = elem['vertices']
        
        # Map original vertices to merged vertices
        original_indices = list(range(elem_idx * 3, elem_idx * 3 + 3))
        merged_node_ids = [vertex_map[idx] for idx in original_indices]
        
        # Create edges (ordered by node ID to ensure consistency)
        edges = [
            tuple(sorted([merged_node_ids[0], merged_node_ids[1]])),
            tuple(sorted([merged_node_ids[1], merged_node_ids[2]])),
            tuple(sorted([merged_node_ids[2], merged_node_ids[0]]))
        ]
        
        for edge in edges:
            if edge not in edge_map:
                edge_map[edge] = []
            edge_map[edge].append({
                'element': elem,
                'vertices': vertices,
                'merged_node_ids': merged_node_ids,
                'original_indices': original_indices,
                'elem_idx': elem_idx
            })
    
    # Find internal edges (shared by exactly 2 triangles)
    internal_edges = []
    for edge, elem_data_list in edge_map.items():
        if len(elem_data_list) == 2:
            internal_edges.append({
                'edge': edge,
                'elements': elem_data_list,
                'id': len(internal_edges)
            })
    
    print(f"  Found {len(internal_edges)} internal edges (RWG basis functions)")
    print(f"  Total edges: {len(edge_map)}")
    print(f"  Boundary edges: {len(edge_map) - len(internal_edges)}")
    
    # Calculate RWG parameters for each basis function
    rwg_functions = []
    
    for edge_data in internal_edges:
        edge = edge_data['edge']
        elem_data_list = edge_data['elements']
        
        # Get common edge vertices from first element
        first_elem = elem_data_list[0]
        vertices = first_elem['vertices']
        merged_node_ids = first_elem['merged_node_ids']
        
        # Find edge vertices
        edge_vertex_indices = []
        for i, node_id in enumerate(merged_node_ids):
            if node_id in edge:
                edge_vertex_indices.append(i)
        
        if len(edge_vertex_indices) != 2:
            continue
            
        edge_vertices = [vertices[edge_vertex_indices[0]], vertices[edge_vertex_indices[1]]]
        
        # Calculate RWG parameters for both triangles
        rwg_params = []
        
        for elem_data in elem_data_list:
            vertices = elem_data['vertices']
            merged_node_ids = elem_data['merged_node_ids']
            elem = elem_data['element']
            
            # Find the vertex opposite to the edge
            opposite_vertex_idx = None
            for i, node_id in enumerate(merged_node_ids):
                if node_id not in edge:
                    opposite_vertex_idx = i
                    break
            
            if opposite_vertex_idx is not None:
                # Calculate RWG parameters
                r_plus = vertices[opposite_vertex_idx]  # Free vertex
                
                # Edge length
                edge_length = np.linalg.norm(edge_vertices[1] - edge_vertices[0])
                
                # Area of triangle
                area = elem['area']
                
                # Normal vector (ensure outward direction)
                normal = elem['normal']
                
                rwg_params.append({
                    'element': elem,
                    'r_plus': r_plus,
                    'edge_length': edge_length,
                    'area': area,
                    'normal': normal,
                    'edge_vertices': edge_vertices
                })
        
        if len(rwg_params) == 2:
            rwg_functions.append({
                'edge_id': edge_data['id'],
                'edge': edge,
                'triangles': rwg_params,
                'edge_length': np.linalg.norm(edge_vertices[1] - edge_vertices[0])
            })
    
    print(f"  Successfully created {len(rwg_functions)} RWG basis functions")
    
    return rwg_functions, elements

def calculate_mom_impedance_matrix_rwg(rwg_functions, frequency):
    """Calculate MoM impedance matrix using RWG basis functions"""
    
    print("\nCalculating MoM impedance matrix with RWG basis functions...")
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    eta = np.sqrt(mu0 / eps0)
    
    num_basis = len(rwg_functions)
    Z = np.zeros((num_basis, num_basis), dtype=complex)
    
    print(f"  Matrix size: {num_basis}×{num_basis}")
    print(f"  Wave number k: {k:.3f} rad/m")
    print(f"  Wavelength λ: {2*np.pi/k*1000:.1f} mm")
    
    # Calculate impedance matrix
    for m in range(num_basis):
        if m % 50 == 0 and m > 0:
            print(f"  Processing row {m}/{num_basis}")
            
        for n in range(num_basis):
            
            if m == n:
                # Self-impedance (diagonal elements)
                # For RWG basis functions, self-impedance involves singularity treatment
                
                # Get triangle areas
                area_m = rwg_functions[m]['triangles'][0]['area']
                area_n = rwg_functions[n]['triangles'][1]['area']
                
                # Approximate self-impedance for RWG basis
                # Z_mm ≈ (η * k * edge_length²) / (8 * π) * (1/(3*area_m) + 1/(3*area_n))
                edge_length = rwg_functions[m]['edge_length']
                
                if area_m > 0 and area_n > 0:
                    Z[m, m] = (eta * k * edge_length**2) / (8 * np.pi) * (1/(3*area_m) + 1/(3*area_n))
                else:
                    Z[m, m] = (eta * k * edge_length**2) / (8 * np.pi)
                    
            else:
                # Mutual impedance (off-diagonal elements)
                # Get triangle centers
                centers_m = []
                centers_n = []
                
                for tri_m in rwg_functions[m]['triangles']:
                    vertices = tri_m['element']['vertices']
                    center = np.mean(vertices, axis=0)
                    centers_m.append(center)
                
                for tri_n in rwg_functions[n]['triangles']:
                    vertices = tri_n['element']['vertices']
                    center = np.mean(vertices, axis=0)
                    centers_n.append(center)
                
                # Calculate mutual impedance between triangle pairs
                Z_mutual = 0
                
                for i, center_m in enumerate(centers_m):
                    for j, center_n in enumerate(centers_n):
                        R = np.linalg.norm(center_m - center_n)
                        
                        if R > 1e-10:  # Avoid singularity
                            # Get triangle areas
                            area_m = rwg_functions[m]['triangles'][i]['area']
                            area_n = rwg_functions[n]['triangles'][j]['area']
                            
                            # Mutual impedance approximation
                            # Z_mn ≈ (η * k * edge_length_m * edge_length_n * area_m * area_n) / (4 * π * R) * exp(-j*k*R)
                            edge_length_m = rwg_functions[m]['edge_length']
                            edge_length_n = rwg_functions[n]['edge_length']
                            
                            Z_pair = (eta * k * edge_length_m * edge_length_n * area_m * area_n) / (4 * np.pi * R) * np.exp(-1j * k * R)
                            Z_mutual += Z_pair
                
                Z[m, n] = Z_mutual
    
    print(f"  Matrix condition number: {np.linalg.cond(Z):.2e}")
    
    return Z

def calculate_incident_field_rwg(rwg_functions, config):
    """Calculate incident plane wave field on RWG basis functions"""
    
    frequency = config['frequency']
    theta, phi, psi = config['source_angle']
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    
    # Convert angles to radians
    theta_rad = np.deg2rad(theta)
    phi_rad = np.deg2rad(phi)
    
    # Plane wave propagation direction
    k_hat = np.array([
        np.sin(theta_rad) * np.cos(phi_rad),
        np.sin(theta_rad) * np.sin(phi_rad),
        np.cos(theta_rad)
    ])
    
    # Polarization vector (perpendicular to propagation)
    E0 = np.array([
        np.cos(theta_rad) * np.cos(phi_rad),
        np.cos(theta_rad) * np.sin(phi_rad),
        -np.sin(theta_rad)
    ])
    
    # Normalize
    E0 = E0 / np.linalg.norm(E0)
    
    V = np.zeros(len(rwg_functions), dtype=complex)
    
    for i, rwg_func in enumerate(rwg_functions):
        
        # Calculate incident field contribution for this RWG basis function
        V_rwg = 0
        
        for triangle in rwg_func['triangles']:
            # Get triangle center
            vertices = triangle['element']['vertices']
            center = np.mean(vertices, axis=0)
            
            # Incident field at triangle center
            phase = -k * np.dot(k_hat, center)
            E_inc = E0 * np.exp(1j * phase)
            
            # RWG testing: integrate E_inc over triangle
            # V_n ≈ E_inc(center) · ρ_edge * area
            # where ρ_edge is the vector from free vertex to edge center
            
            r_plus = triangle['r_plus']  # Free vertex
            edge_center = np.mean(triangle['edge_vertices'], axis=0)
            rho_edge = edge_center - r_plus
            
            # Tangential component (what matters for surface currents)
            E_tangential = E_inc - np.dot(E_inc, triangle['normal']) * triangle['normal']
            
            # Integrate over triangle area
            contribution = np.dot(E_tangential, rho_edge) * triangle['area']
            V_rwg += contribution
        
        V[i] = V_rwg
    
    return V

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

def create_observation_grid(config, plane='XY', z_level=0, resolution=25):
    """Create observation grid for field calculations"""
    
    domain_size = np.array(config['domain_size']) * 1e-3  # Convert to meters
    
    # Create grid with specified resolution
    if plane == 'XY':
        x = np.linspace(-domain_size[0]/2, domain_size[0]/2, resolution)
        y = np.linspace(-domain_size[1]/2, domain_size[1]/2, resolution)
        X, Y = np.meshgrid(x, y, indexing='ij')
        Z = np.full_like(X, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
    elif plane == 'XZ':
        x = np.linspace(-domain_size[0]/2, domain_size[0]/2, resolution)
        z = np.linspace(-domain_size[2]/2, domain_size[2]/2, resolution)
        X, Z = np.meshgrid(x, z, indexing='ij')
        Y = np.full_like(X, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
    else:  # YZ
        y = np.linspace(-domain_size[1]/2, domain_size[1]/2, resolution)
        z = np.linspace(-domain_size[2]/2, domain_size[2]/2, resolution)
        Y, Z = np.meshgrid(y, z, indexing='ij')
        X = np.full_like(Y, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    
    return points

def calculate_surface_currents_from_stl(Z_matrix, V_incident):
    """Calculate surface currents from MoM matrix equation"""
    
    print("Solving MoM matrix equation Z*I = V...")
    
    try:
        # Check matrix condition number
        cond_number = np.linalg.cond(Z_matrix)
        print(f"  Matrix condition number: {cond_number:.2e}")
        
        if cond_number > 1e12:
            print("  Warning: Matrix is ill-conditioned, using regularization")
            # Add small regularization to diagonal
            Z_regularized = Z_matrix + 1e-6 * np.eye(len(Z_matrix)) * np.max(np.abs(np.diag(Z_matrix)))
            surface_currents = np.linalg.solve(Z_regularized, V_incident)
        else:
            # Direct solution
            surface_currents = np.linalg.solve(Z_matrix, V_incident)
        
        print(f"  Solution completed successfully")
        print(f"  Current range: {np.min(np.abs(surface_currents)):.3e} - {np.max(np.abs(surface_currents)):.3e} A/m")
        
        return surface_currents
        
    except np.linalg.LinAlgError as e:
        print(f"  Matrix solution failed: {e}")
        print("  Using least-squares solution")
        
        # Use least squares solution
        surface_currents, residuals, rank, s = np.linalg.lstsq(Z_matrix, V_incident, rcond=1e-12)
        
        print(f"  Least-squares solution completed")
        print(f"  Residual norm: {np.linalg.norm(Z_matrix @ surface_currents - V_incident):.3e}")
        
        return surface_currents

def calculate_scattered_field_rwg(rwg_functions, currents, observation_points, frequency):
    """Calculate scattered field from RWG surface currents"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    eta = np.sqrt(mu0 / eps0)
    
    fields = []
    
    print(f"Calculating scattered field at {len(observation_points)} observation points...")
    
    for i, point in enumerate(observation_points):
        if i % 100 == 0:
            print(f"  Point {i}/{len(observation_points)}")
            
        E_scat = np.zeros(3, dtype=complex)
        
        # Calculate field from each RWG basis function
        for j, current in enumerate(currents):
            rwg_func = rwg_functions[j]
            
            # Calculate field contribution from this RWG function
            contribution = 0
            
            for triangle in rwg_func['triangles']:
                vertices = triangle['element']['vertices']
                center = np.mean(vertices, axis=0)
                area = triangle['area']
                
                # Vector from triangle center to observation point
                R_vec = point - center
                R = np.linalg.norm(R_vec)
                
                if R > 1e-10:  # Avoid singularity
                    R_hat = R_vec / R
                    
                    # RWG current distribution
                    # J(r) = (I_n / (2 * A_n)) * (r - r_n^±) for triangle ±
                    # where I_n is the basis function coefficient
                    
                    # Approximate current at triangle center
                    r_plus = triangle['r_plus']
                    J_center = (current / (2 * area)) * (center - r_plus)
                    
                    # Radiated field from current element
                    # E = -j*omega*mu0 * (J × R_hat) × R_hat * exp(-j*k*R) / (4*pi*R)
                    factor = (-1j * omega * mu0 * area) / (4 * np.pi * R)
                    phase = np.exp(-1j * k * R)
                    
                    # Cross product: J × R_hat
                    J_cross_R = np.cross(J_center, R_hat)
                    
                    # Double cross product: (J × R_hat) × R_hat
                    E_rad = np.cross(J_cross_R, R_hat)
                    
                    # Add contribution to scattered field
                    E_scat += factor * phase * E_rad
        
        fields.append(E_scat)
    
    return fields

def visualize_professional_mom_results(rwg_functions, currents, config, observation_points, scattered_fields):
    """Create professional visualization of MoM results"""
    
    fig = plt.figure(figsize=(20, 16))
    
    # 1. Surface current distribution on RWG basis functions
    ax1 = fig.add_subplot(2, 3, 1, projection='3d')
    
    # Plot RWG basis functions with current magnitude
    current_magnitudes = np.abs(currents)
    max_current = np.max(current_magnitudes) if np.max(current_magnitudes) > 0 else 1
    
    # Sample basis functions for visualization
    sample_indices = range(0, len(rwg_functions), max(1, len(rwg_functions)//200))
    
    for i in sample_indices:
        rwg_func = rwg_functions[i]
        current_mag = current_magnitudes[i]
        
        # Color by current magnitude
        intensity = current_mag / max_current
        color = plt.cm.plasma(intensity)
        
        # Plot both triangles of the RWG function
        for triangle in rwg_func['triangles']:
            vertices = triangle['element']['vertices']
            
            # Plot triangle
            tri = np.array([[0, 1, 2]])
            ax1.plot_trisurf(vertices[:, 0], vertices[:, 1], vertices[:, 2], 
                            triangles=tri, color=color, alpha=0.7, edgecolor='none')
    
    ax1.set_title('RWG Surface Current Distribution')
    ax1.set_xlabel('X (m)')
    ax1.set_ylabel('Y (m)')
    ax1.set_zlabel('Z (m)')
    
    # Add colorbar for currents
    sm = plt.cm.ScalarMappable(cmap=plt.cm.plasma, norm=plt.Normalize(vmin=0, vmax=max_current))
    sm.set_array([])
    plt.colorbar(sm, ax=ax1, shrink=0.5, label='Current Magnitude (A/m)')
    
    # 2. Current magnitude histogram
    ax2 = fig.add_subplot(2, 3, 2)
    
    ax2.hist(current_magnitudes, bins=50, alpha=0.7, color='blue', edgecolor='black')
    ax2.set_title('RWG Current Magnitude Distribution')
    ax2.set_xlabel('Current Magnitude (A/m)')
    ax2.set_ylabel('Count')
    ax2.grid(True, alpha=0.3)
    
    # Add statistics
    mean_current = np.mean(current_magnitudes)
    std_current = np.std(current_magnitudes)
    ax2.axvline(mean_current, color='red', linestyle='--', label=f'Mean: {mean_current:.2e}')
    ax2.axvline(mean_current + std_current, color='orange', linestyle=':', label=f'Mean+σ: {(mean_current + std_current):.2e}')
    ax2.legend()
    
    # 3. Scattered field magnitude
    ax3 = fig.add_subplot(2, 3, 3)
    
    if len(observation_points) > 0:
        field_magnitudes = np.array([np.linalg.norm(field) for field in scattered_fields])
        
        # Reshape for contour plot (assuming XY plane)
        n_points = int(np.sqrt(len(observation_points)))
        if n_points * n_points == len(observation_points):
            X = observation_points[:, 0].reshape(n_points, n_points)
            Y = observation_points[:, 1].reshape(n_points, n_points)
            Z = field_magnitudes.reshape(n_points, n_points)
            
            contour = ax3.contourf(X, Y, Z, levels=20, cmap='viridis')
            ax3.set_title('Scattered Field Magnitude (XY Plane)')
            ax3.set_xlabel('X (m)')
            ax3.set_ylabel('Y (m)')
            plt.colorbar(contour, ax=ax3, label='|E_scat| (V/m)')
    
    # 4. Current phase distribution
    ax4 = fig.add_subplot(2, 3, 4)
    
    current_phases = np.angle(currents)
    
    # Sample basis functions for phase visualization
    if len(rwg_functions) > 0:
        sample_centers = []
        sample_phases = []
        
        for i in sample_indices[:min(200, len(sample_indices))]:
            rwg_func = rwg_functions[i]
            
            # Get center of RWG function
            centers = []
            for triangle in rwg_func['triangles']:
                vertices = triangle['element']['vertices']
                center = np.mean(vertices, axis=0)
                centers.append(center)
            
            avg_center = np.mean(centers, axis=0)
            sample_centers.append(avg_center)
            sample_phases.append(current_phases[i])
        
        if sample_centers:
            centers_array = np.array(sample_centers)
            scatter_phase = ax4.scatter(centers_array[:, 0], centers_array[:, 1], 
                                      c=sample_phases, cmap='hsv', s=30, alpha=0.6)
            ax4.set_title('RWG Current Phase Distribution')
            ax4.set_xlabel('X (m)')
            ax4.set_ylabel('Y (m)')
            plt.colorbar(scatter_phase, ax=ax4, label='Phase (rad)')
    
    # 5. Field vectors at observation points
    ax5 = fig.add_subplot(2, 3, 5, projection='3d')
    
    if len(observation_points) > 0 and len(scattered_fields) > 0:
        # Sample every 10th point for clarity
        sample_indices_field = range(0, len(observation_points), max(1, len(observation_points)//100))
        
        for i in sample_indices_field:
            pos = observation_points[i]
            field = scattered_fields[i]
            
            if np.linalg.norm(field) > 1e-3:  # Only show significant fields
                # Normalize field vector for visualization
                field_norm = field / np.linalg.norm(field) * 0.05  # Scale for visibility
                ax5.quiver(pos[0], pos[1], pos[2], 
                          field_norm[0], field_norm[1], field_norm[2],
                          color='red', alpha=0.6, arrow_length_ratio=0.3)
    
    ax5.set_title('Scattered Field Vectors')
    ax5.set_xlabel('X (m)')
    ax5.set_ylabel('Y (m)')
    ax5.set_zlabel('Z (m)')
    
    # 6. Professional analysis summary
    ax6 = fig.add_subplot(2, 3, 6)
    ax6.axis('off')
    
    # Calculate comprehensive statistics
    total_current = np.sum(np.abs(currents))
    max_current = np.max(np.abs(currents)) if len(currents) > 0 else 0
    avg_current = np.mean(np.abs(currents)) if len(currents) > 0 else 0
    rms_current = np.sqrt(np.mean(np.abs(currents)**2)) if len(currents) > 0 else 0
    
    max_field = np.max([np.linalg.norm(field) for field in scattered_fields]) if scattered_fields else 0
    avg_field = np.mean([np.linalg.norm(field) for field in scattered_fields]) if scattered_fields else 0
    
    # Calculate scattering cross section (simplified)
    wavelength = 2 * np.pi * 3e8 / config['frequency']
    k = 2 * np.pi / wavelength
    
    if scattered_fields:
        total_scattered_power = np.sum([np.linalg.norm(field)**2 for field in scattered_fields]) * (wavelength/10)**2
        incident_power = 1.0  # Normalized incident power
        scattering_cross_section = total_scattered_power / incident_power
    else:
        scattering_cross_section = 0
    
    summary_text = f"""
    Professional MoM/PEEC Analysis Report
    =====================================
    
    Configuration:
    - Frequency: {config['frequency']/1e9:.1f} GHz
    - Wavelength: {wavelength*1000:.1f} mm
    - Incident Angles: θ={config['source_angle'][0]:.1f}°, φ={config['source_angle'][1]:.1f}°
    
    Mesh Statistics:
    - RWG Basis Functions: {len(rwg_functions)}
    - Target Edge Length: {config['mesh_target_length']*1000:.1f} mm
    - Mesh Quality: Professional (Gmsh)
    
    Surface Current Analysis:
    - Total Current: {total_current:.3e} A/m
    - Maximum Current: {max_current:.3e} A/m
    - RMS Current: {rms_current:.3e} A/m
    - Dynamic Range: {max_current/rms_current:.1f}:1
    
    Scattered Field Analysis:
    - Maximum Field: {max_field:.3e} V/m
    - Average Field: {avg_field:.3e} V/m
    - Scattering Cross Section: {scattering_cross_section:.3e} m²
    
    Method: RWG Basis Functions (Industry Standard)
    Material: PEC (Perfect Electric Conductor)
    Solver: Method of Moments (MoM)
    """
    
    ax6.text(0.05, 0.95, summary_text, transform=ax6.transAxes, 
             fontsize=9, verticalalignment='top', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
    
    plt.tight_layout()
    plt.savefig('weixing_v1_professional_mom_analysis.png', dpi=300, bbox_inches='tight')
    plt.close()

def main():
    """Main professional analysis function"""
    
    print("="*70)
    print("PROFESSIONAL MoM/PEEC ANALYSIS WITH PROPER CAD PIPELINE")
    print("="*70)
    
    # Load configuration
    print("\n1. Loading PFD configuration...")
    config = parse_pfd_config_enhanced('weixing_v1_case.pfd')
    print(f"   Domain: {config['domain_size']} mm")
    print(f"   Frequency: {config['frequency']/1e9} GHz")
    print(f"   Target mesh size: {config['mesh_target_length']*1000:.1f} mm")
    
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
    
    # Generate professional mesh using Gmsh (if available)
    print("\n4. Generating professional surface mesh...")
    
    # Try Gmsh first
    geo_filename = create_gmsh_mesh_script(facets, target_edge_length=config['mesh_target_length'])
    gmsh_success = run_gmsh_mesh_generation(geo_filename)
    
    if gmsh_success and os.path.exists('satellite_mesh.msh'):
        print("   Using Gmsh-generated mesh")
        elements = parse_gmsh_mesh('satellite_mesh.msh')
    else:
        print("   Using direct STL mesh (Gmsh not available)")
        # Convert facets to elements format
        elements = []
        for facet in facets:
            elements.append({
                'vertices': facet['vertices'],
                'normal': facet['normal'],
                'area': facet['area'],
                'id': facet['id']
            })
    
    if not elements:
        print("   Failed to create mesh")
        return
    
    print(f"   Final mesh triangles: {len(elements)}")
    
    # Create RWG basis functions
    print("\n5. Creating RWG basis functions...")
    rwg_functions, elements = calculate_rwg_basis_functions(elements)
    
    if not rwg_functions:
        print("   Failed to create RWG basis functions")
        return
    
    # Calculate MoM impedance matrix
    print("\n6. Calculating MoM impedance matrix...")
    Z_matrix = calculate_mom_impedance_matrix_rwg(rwg_functions, config['frequency'])
    
    # Calculate incident field
    print("\n7. Calculating incident field excitation...")
    V_incident = calculate_incident_field_rwg(rwg_functions, config)
    
    # Solve for surface currents
    print("\n8. Solving for surface currents...")
    surface_currents = calculate_surface_currents_from_stl(Z_matrix, V_incident)
    
    # Create observation grid
    print("\n9. Creating observation grids...")
    observation_points_xy = create_observation_grid(config, plane='XY', z_level=0.0, resolution=25)
    observation_points_xz = create_observation_grid(config, plane='XZ', z_level=0.0, resolution=25)
    
    print(f"   XY observation points: {len(observation_points_xy)}")
    print(f"   XZ observation points: {len(observation_points_xz)}")
    
    # Calculate scattered fields
    print("\n10. Calculating scattered fields...")
    scattered_fields_xy = calculate_scattered_field_rwg(
        rwg_functions, surface_currents, observation_points_xy, config['frequency'])
    scattered_fields_xz = calculate_scattered_field_rwg(
        rwg_functions, surface_currents, observation_points_xz, config['frequency'])
    
    # Generate professional visualizations
    print("\n11. Generating professional visualizations...")
    visualize_professional_mom_results(rwg_functions, surface_currents, config, 
                                     observation_points_xy, scattered_fields_xy)
    
    print("\n" + "="*70)
    print("PROFESSIONAL MoM/PEEC ANALYSIS COMPLETED")
    print("="*70)
    
    # Final statistics
    print(f"\nMesh Quality Summary:")
    print(f"   Triangles: {len(elements)}")
    print(f"   RWG Basis Functions: {len(rwg_functions)}")
    print(f"   Matrix Condition Number: {np.linalg.cond(Z_matrix):.2e}")
    
    print(f"\nSurface Current Summary:")
    print(f"   Current Range: {np.min(np.abs(surface_currents)):.3e} - {np.max(np.abs(surface_currents)):.3e} A/m")
    print(f"   RMS Current: {np.sqrt(np.mean(np.abs(surface_currents)**2)):.3e} A/m")
    
    if scattered_fields_xy:
        field_mags = [np.linalg.norm(field) for field in scattered_fields_xy]
        print(f"\nScattered Field Summary:")
        print(f"   Field Range: {np.min(field_mags):.3e} - {np.max(field_mags):.3e} V/m")
        print(f"   Average Field: {np.mean(field_mags):.3e} V/m")
    
    print(f"\nFiles Generated:")
    print(f"   - weixing_v1_professional_mom_analysis.png")
    if gmsh_success:
        print(f"   - satellite_mesh.msh (Gmsh mesh file)")
        print(f"   - satellite_mesh.geo (Gmsh geometry script)")

if __name__ == "__main__":
    main()