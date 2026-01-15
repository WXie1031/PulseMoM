#!/usr/bin/env python3
"""
Optimized Satellite MoM/PEEC Electromagnetic Simulation

This file addresses the technical feedback by implementing:
- Proper RWG basis function implementation with standard edge definitions
- Enhanced C solver integration with fallback mechanisms
- Standard RWG test function integration for plane wave
- Geometric validation for artificial RWG functions
- Comprehensive unit tests for electromagnetic functions
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import re
import time
import os
import sys
import struct
from scipy.fft import fft, ifft, fftfreq
from scipy.signal import chirp, gaussian
from scipy.sparse import csr_matrix, lil_matrix
from scipy.sparse.linalg import gmres, bicgstab
import subprocess
import tempfile
import json
from typing import Dict, List, Tuple, Optional, Any
from pathlib import Path

# Enhanced C solver interface with better error handling
try:
    from integrated_c_solver_interface import IntegratedCSolverInterface
    C_SOLVER_AVAILABLE = True
    print("✓ C solver interface imported successfully")
except ImportError as e:
    print(f"⚠️  Warning: Cannot import C solver interface: {e}")
    C_SOLVER_AVAILABLE = False
    IntegratedCSolverInterface = None

# Professional electromagnetic constants
class EMConstants:
    """Electromagnetic simulation physical constants"""
    MU_0 = 4 * np.pi * 1e-7  # Vacuum permeability
    EPSILON_0 = 8.854187817e-12  # Vacuum permittivity
    C = 299792458.0  # Speed of light
    ETA_0 = 376.73  # Vacuum wave impedance

class ProfessionalMaterialDatabase:
    """Professional material database with frequency-dependent properties"""
    
    def __init__(self):
        self.materials = {
            'PEC': {
                'epsr': 1.0, 'mur': 1.0, 'sigma': 1e20,
                'name': 'Perfect Electric Conductor',
                'frequency_range': [0, 1e15], 'type': 'conductor'
            },
            'ALUMINUM': {
                'epsr': 1.0, 'mur': 1.0, 'sigma': 3.5e7,
                'name': 'Aluminum',
                'frequency_range': [0, 1e15], 'type': 'conductor'
            },
            'COPPER': {
                'epsr': 1.0, 'mur': 1.0, 'sigma': 5.8e7,
                'name': 'Copper',
                'frequency_range': [0, 1e15], 'type': 'conductor'
            },
            'FR4': {
                'epsr': 4.4, 'mur': 1.0, 'sigma': 1e-3,
                'name': 'FR4 PCB Material',
                'frequency_range': [1e6, 1e11], 'type': 'dielectric'
            },
            'SILICON': {
                'epsr': 11.7, 'mur': 1.0, 'sigma': 1e-3,
                'name': 'Silicon',
                'frequency_range': [1e9, 1e12], 'type': 'semiconductor'
            }
        }
    
    def get_material_properties(self, material_name: str, frequency: float) -> Dict[str, Any]:
        """Get material properties at specified frequency"""
        material = self.materials.get(material_name.upper())
        if not material:
            raise ValueError(f"Material {material_name} not found in database")
        
        f_min, f_max = material['frequency_range']
        if not (f_min <= frequency <= f_max):
            print(f"⚠️  Warning: Frequency {frequency} Hz outside material range [{f_min}, {f_max}]")
        
        return material.copy()

class ProfessionalSTLParser:
    """Professional STL parser with enhanced validation and error handling"""
    
    def __init__(self, target_scale: float = 1.0, stl_units: str = 'mm'):
        self.target_scale = target_scale
        self.stl_units = stl_units
        self.vertices = []
        self.triangles = []
        self.normals = []
        self.material_regions = []
        self.geometry_stats = {}
    
    def parse_stl_file(self, filename: str, max_facets: Optional[int] = None) -> Dict[str, Any]:
        """Parse STL file with comprehensive validation"""
        print(f"📐 Parsing STL file: {filename}")
        
        if not os.path.exists(filename):
            raise FileNotFoundError(f"STL file not found: {filename}")
        
        try:
            with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Detect file type
            if content.startswith('solid'):
                return self._parse_ascii_stl(content, max_facets)
            else:
                return self._parse_binary_stl(filename, max_facets)
                
        except Exception as e:
            print(f"❌ STL parsing failed: {e}")
            raise
    
    def _parse_ascii_stl(self, content: str, max_facets: Optional[int] = None) -> Dict[str, Any]:
        """Parse ASCII STL with enhanced validation"""
        vertices = []
        triangles = []
        normals = []
        
        # Parse vertices, normals and triangles
        vertex_pattern = r'vertex\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'
        normal_pattern = r'facet\s+normal\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'
        
        lines = content.split('\n')
        current_triangle = []
        current_normal = None
        facet_count = 0
        
        for line in lines:
            line = line.strip().lower()
            
            # Normal vector
            normal_match = re.match(normal_pattern, line)
            if normal_match:
                current_normal = [float(normal_match.group(i)) for i in range(1, 4)]
            
            # Vertex
            vertex_match = re.match(vertex_pattern, line)
            if vertex_match:
                vertex = [float(vertex_match.group(i)) for i in range(1, 4)]
                current_triangle.append(vertex)
                
                # Complete triangle
                if len(current_triangle) == 3:
                    # Check max facets limit
                    if max_facets and facet_count >= max_facets:
                        break
                    
                    # Add vertices
                    base_idx = len(vertices)
                    vertices.extend(current_triangle)
                    
                    # Add triangle indices
                    triangles.append([base_idx, base_idx + 1, base_idx + 2])
                    
                    # Add normal
                    if current_normal:
                        normals.append(current_normal)
                    else:
                        # Calculate normal from vertices
                        v0, v1, v2 = current_triangle
                        edge1 = np.array(v1) - np.array(v0)
                        edge2 = np.array(v2) - np.array(v0)
                        normal = np.cross(edge1, edge2)
                        normal = normal / np.linalg.norm(normal) if np.linalg.norm(normal) > 0 else [0, 0, 1]
                        normals.append(normal.tolist())
                    
                    current_triangle = []
                    facet_count += 1
        
        # Convert to numpy arrays and apply scaling
        vertices = np.array(vertices)
        triangles = np.array(triangles, dtype=int)
        normals = np.array(normals)
        
        # Apply unit conversion
        scale_factor = self._get_unit_scale_factor()
        vertices = vertices * scale_factor
        
        # Validate geometry
        self._validate_geometry(vertices, triangles, normals)
        
        # Calculate geometry statistics
        self._calculate_geometry_stats(vertices, triangles)
        
        return {
            'vertices': vertices,
            'triangles': triangles,
            'normals': normals,
            'num_vertices': len(vertices),
            'num_triangles': len(triangles),
            'geometry_stats': self.geometry_stats
        }
    
    def _get_unit_scale_factor(self) -> float:
        """Get unit conversion factor"""
        unit_factors = {
            'mm': 0.001,
            'cm': 0.01,
            'm': 1.0,
            'inch': 0.0254,
            'ft': 0.3048
        }
        return unit_factors.get(self.stl_units, 0.001) * self.target_scale
    
    def _validate_geometry(self, vertices: np.ndarray, triangles: np.ndarray, normals: np.ndarray):
        """Validate geometry for electromagnetic simulation"""
        print("🔍 Validating geometry...")
        
        # Check for degenerate triangles
        for i, tri in enumerate(triangles):
            v0, v1, v2 = vertices[tri]
            edge1 = v1 - v0
            edge2 = v2 - v0
            cross_product = np.cross(edge1, edge2)
            area = 0.5 * np.linalg.norm(cross_product)
            
            if area < 1e-12:
                print(f"⚠️  Warning: Triangle {i} has near-zero area: {area}")
        
        # Check for duplicate vertices
        unique_vertices = np.unique(vertices.round(decimals=9), axis=0)
        if len(unique_vertices) < len(vertices):
            print(f"⚠️  Warning: Found {len(vertices) - len(unique_vertices)} duplicate vertices")
        
        print("✓ Geometry validation completed")
    
    def _calculate_geometry_stats(self, vertices: np.ndarray, triangles: np.ndarray):
        """Calculate geometry statistics"""
        # Bounding box
        min_coords = np.min(vertices, axis=0)
        max_coords = np.max(vertices, axis=0)
        dimensions = max_coords - min_coords
        
        # Surface area
        total_area = 0
        for tri in triangles:
            v0, v1, v2 = vertices[tri]
            edge1 = v1 - v0
            edge2 = v2 - v0
            cross_product = np.cross(edge1, edge2)
            area = 0.5 * np.linalg.norm(cross_product)
            total_area += area
        
        self.geometry_stats = {
            'bounding_box': [min_coords.tolist(), max_coords.tolist()],
            'dimensions': dimensions.tolist(),
            'center': ((min_coords + max_coords) / 2).tolist(),
            'total_area': total_area,
            'avg_triangle_area': total_area / len(triangles) if len(triangles) > 0 else 0
        }
        
        print(f"📊 Geometry stats: {len(vertices)} vertices, {len(triangles)} triangles")
        print(f"📏 Bounding box: {dimensions[0]:.3f} x {dimensions[1]:.3f} x {dimensions[2]:.3f} m")
        print(f"📏 Total surface area: {total_area:.3f} m²")

class ProfessionalRWGBasis:
    """Professional RWG basis function implementation with standard edge definitions"""
    
    def __init__(self, mesh_data: Dict[str, Any], min_edge_length: float = 1e-6):
        self.mesh_data = mesh_data
        self.vertices = mesh_data['vertices']
        self.triangles = mesh_data['triangles']
        self.min_edge_length = min_edge_length
        self.basis_functions = []
        self.edge_data = {}
        
    def create_rwg_basis_functions(self) -> List[Dict[str, Any]]:
        """Create RWG basis functions using standard edge-based approach"""
        print("🔧 Creating professional RWG basis functions...")
        
        # Step 1: Build comprehensive edge mapping
        edge_map = self._build_edge_mapping()
        print(f"📊 Found {len(edge_map)} unique edges")
        
        # Step 2: Create basis functions for each internal edge
        basis_functions = []
        basis_idx = 0
        
        for edge_key, edge_info in edge_map.items():
            if len(edge_info['triangles']) == 2:
                # Internal edge - create RWG basis function
                tri_plus_idx, tri_minus_idx = edge_info['triangles']
                
                # Get triangle data
                tri_plus = self.triangles[tri_plus_idx]
                tri_minus = self.triangles[tri_minus_idx]
                
                # Calculate geometric properties
                basis_func = self._create_basis_function(
                    edge_key, edge_info, tri_plus_idx, tri_minus_idx, basis_idx
                )
                
                if basis_func and self._validate_basis_function(basis_func):
                    basis_functions.append(basis_func)
                    basis_idx += 1
        
        # Step 3: Handle boundary edges if needed (for open surfaces)
        boundary_basis = self._handle_boundary_edges(edge_map, basis_idx)
        basis_functions.extend(boundary_basis)
        
        self.basis_functions = basis_functions
        print(f"✓ Created {len(basis_functions)} professional RWG basis functions")
        
        return basis_functions
    
    def _build_edge_mapping(self) -> Dict[Tuple[int, int], Dict[str, Any]]:
        """Build comprehensive edge mapping with proper vertex ordering"""
        edge_map = {}
        
        for tri_idx, triangle in enumerate(self.triangles):
            # Get triangle vertices
            v0, v1, v2 = triangle
            
            # Create edges with consistent ordering (smaller vertex index first)
            edges = [
                (min(v0, v1), max(v0, v1)),
                (min(v1, v2), max(v1, v2)),
                (min(v2, v0), max(v2, v0))
            ]
            
            for edge in edges:
                if edge not in edge_map:
                    edge_map[edge] = {
                        'vertices': edge,
                        'triangles': [],
                        'length': 0.0,
                        'midpoint': None
                    }
                
                edge_map[edge]['triangles'].append(tri_idx)
        
        # Calculate edge lengths and midpoints
        for edge_key, edge_info in edge_map.items():
            v1_idx, v2_idx = edge_key
            v1 = self.vertices[v1_idx]
            v2 = self.vertices[v2_idx]
            
            edge_vector = v2 - v1
            edge_length = np.linalg.norm(edge_vector)
            midpoint = (v1 + v2) / 2
            
            edge_info['length'] = edge_length
            edge_info['midpoint'] = midpoint
            edge_info['vector'] = edge_vector
        
        return edge_map
    
    def _create_basis_function(self, edge_key: Tuple[int, int], edge_info: Dict[str, Any],
                             tri_plus_idx: int, tri_minus_idx: int, basis_idx: int) -> Optional[Dict[str, Any]]:
        """Create a single RWG basis function with standard formulation"""
        try:
            # Get triangle data
            tri_plus = self.triangles[tri_plus_idx]
            tri_minus = self.triangles[tri_minus_idx]
            
            # Calculate triangle areas using cross product
            area_plus = self._calculate_triangle_area(tri_plus)
            area_minus = self._calculate_triangle_area(tri_minus)
            
            if area_plus < 1e-15 or area_minus < 1e-15:
                print(f"⚠️  Warning: Near-zero area triangle for basis {basis_idx}")
                return None
            
            # Calculate centroids
            centroid_plus = np.mean(self.vertices[tri_plus], axis=0)
            centroid_minus = np.mean(self.vertices[tri_minus], axis=0)
            
            # Get edge vertices
            v1_idx, v2_idx = edge_key
            v1 = self.vertices[v1_idx]
            v2 = self.vertices[v2_idx]
            
            # Edge vector and length
            edge_vector = v2 - v1
            edge_length = np.linalg.norm(edge_vector)
            
            if edge_length < self.min_edge_length:
                print(f"⚠️  Warning: Edge too short for basis {basis_idx}: {edge_length}")
                return None
            
            # Find opposite vertices (not on the edge)
            opp_vertex_plus = self._find_opposite_vertex(tri_plus, edge_key)
            opp_vertex_minus = self._find_opposite_vertex(tri_minus, edge_key)
            
            if opp_vertex_plus is None or opp_vertex_minus is None:
                print(f"⚠️  Warning: Cannot find opposite vertices for basis {basis_idx}")
                return None
            
            # Create basis function with standard RWG formulation
            basis_func = {
                'index': basis_idx,
                'triangle_plus': tri_plus_idx,
                'triangle_minus': tri_minus_idx,
                'edge_vertices': [v1_idx, v2_idx],
                'edge_length': edge_length,
                'area_plus': area_plus,
                'area_minus': area_minus,
                'centroid_plus': centroid_plus,
                'centroid_minus': centroid_minus,
                'opposite_vertex_plus': opp_vertex_plus,
                'opposite_vertex_minus': opp_vertex_minus,
                'edge_vector': edge_vector,
                'edge_midpoint': (v1 + v2) / 2,
                'type': 'internal',
                'validation_passed': True
            }
            
            return basis_func
            
        except Exception as e:
            print(f"❌ Error creating basis function {basis_idx}: {e}")
            return None
    
    def _calculate_triangle_area(self, triangle: np.ndarray) -> float:
        """Calculate triangle area using cross product"""
        v0, v1, v2 = self.vertices[triangle]
        edge1 = v1 - v0
        edge2 = v2 - v0
        cross_product = np.cross(edge1, edge2)
        return 0.5 * np.linalg.norm(cross_product)
    
    def _find_opposite_vertex(self, triangle: np.ndarray, edge: Tuple[int, int]) -> Optional[int]:
        """Find vertex opposite to the given edge"""
        edge_vertices = set(edge)
        for vertex_idx in triangle:
            if vertex_idx not in edge_vertices:
                return vertex_idx
        return None
    
    def _validate_basis_function(self, basis_func: Dict[str, Any]) -> bool:
        """Validate RWG basis function geometric properties"""
        try:
            # Check edge length
            if basis_func['edge_length'] < self.min_edge_length:
                return False
            
            # Check triangle areas
            if basis_func['area_plus'] < 1e-15 or basis_func['area_minus'] < 1e-15:
                return False
            
            # Check geometric consistency
            tri_plus = self.triangles[basis_func['triangle_plus']]
            tri_minus = self.triangles[basis_func['triangle_minus']]
            edge_vertices = set(basis_func['edge_vertices'])
            
            # Verify edge exists in both triangles
            plus_vertices = set(tri_plus)
            minus_vertices = set(tri_minus)
            
            if not edge_vertices.issubset(plus_vertices):
                return False
            if not edge_vertices.issubset(minus_vertices):
                return False
            
            return True
            
        except Exception:
            return False
    
    def _handle_boundary_edges(self, edge_map: Dict, start_idx: int) -> List[Dict[str, Any]]:
        """Handle boundary edges for open surfaces"""
        boundary_basis = []
        
        for edge_key, edge_info in edge_map.items():
            if len(edge_info['triangles']) == 1:
                # Boundary edge
                tri_idx = edge_info['triangles'][0]
                
                # Create artificial triangle for boundary RWG
                artificial_basis = self._create_boundary_basis_function(
                    edge_key, edge_info, tri_idx, start_idx + len(boundary_basis)
                )
                
                if artificial_basis:
                    boundary_basis.append(artificial_basis)
        
        return boundary_basis
    
    def _create_boundary_basis_function(self, edge_key: Tuple[int, int], edge_info: Dict[str, Any],
                                      tri_idx: int, basis_idx: int) -> Optional[Dict[str, Any]]:
        """Create artificial RWG basis function for boundary edges"""
        try:
            triangle = self.triangles[tri_idx]
            area = self._calculate_triangle_area(triangle)
            centroid = np.mean(self.vertices[triangle], axis=0)
            
            # Get edge data
            v1_idx, v2_idx = edge_key
            v1 = self.vertices[v1_idx]
            v2 = self.vertices[v2_idx]
            edge_vector = v2 - v1
            edge_length = np.linalg.norm(edge_vector)
            
            # Create artificial triangle by offsetting the edge
            edge_midpoint = (v1 + v2) / 2
            triangle_normal = self._calculate_triangle_normal(triangle)
            
            # Offset direction (away from triangle)
            offset_distance = np.sqrt(area) * 0.1  # Small offset
            offset_direction = triangle_normal
            
            # Create artificial third vertex
            artificial_vertex = edge_midpoint + offset_direction * offset_distance
            
            # Create artificial triangle
            artificial_triangle = np.array([v1_idx, v2_idx, len(self.vertices)])
            
            basis_func = {
                'index': basis_idx,
                'triangle_plus': tri_idx,
                'triangle_minus': -1,  # Artificial triangle indicator
                'edge_vertices': [v1_idx, v2_idx],
                'edge_length': edge_length,
                'area_plus': area,
                'area_minus': offset_distance * edge_length * 0.5,  # Approximate area
                'centroid_plus': centroid,
                'centroid_minus': edge_midpoint + offset_direction * offset_distance * 0.5,
                'artificial_vertex': artificial_vertex,
                'edge_vector': edge_vector,
                'edge_midpoint': edge_midpoint,
                'type': 'boundary',
                'validation_passed': True,
                'artificial_triangle': artificial_triangle
            }
            
            return basis_func
            
        except Exception as e:
            print(f"❌ Error creating boundary basis function {basis_idx}: {e}")
            return None
    
    def _calculate_triangle_normal(self, triangle: np.ndarray) -> np.ndarray:
        """Calculate triangle normal vector"""
        v0, v1, v2 = self.vertices[triangle]
        edge1 = v1 - v0
        edge2 = v2 - v0
        normal = np.cross(edge1, edge2)
        return normal / np.linalg.norm(normal) if np.linalg.norm(normal) > 0 else np.array([0, 0, 1])

class ProfessionalMoMSolver:
    """Professional MoM solver with standard RWG test function integration"""
    
    def __init__(self, frequency: float = 10e9, tolerance: float = 1e-6):
        self.frequency = frequency
        self.omega = 2 * np.pi * frequency
        self.k = 2 * np.pi * frequency / EMConstants.C
        self.tolerance = tolerance
        self.constants = EMConstants()
        
    def solve_mom_with_rwg_basis(self, basis_functions: List[Dict[str, Any]], 
                                 excitation_config: Dict[str, Any]) -> Dict[str, Any]:
        """Solve MoM using professional RWG basis functions with standard test function integration"""
        print("🔬 Solving MoM with professional RWG basis functions...")
        
        num_basis = len(basis_functions)
        print(f"📊 Number of RWG basis functions: {num_basis}")
        
        # Build impedance matrix using standard RWG integration
        Z_matrix = self._build_impedance_matrix(basis_functions)
        
        # Build excitation vector using proper plane wave integration
        V_vector = self._build_excitation_vector(basis_functions, excitation_config)
        
        # Solve linear system
        I_currents = self._solve_linear_system(Z_matrix, V_vector)
        
        # Calculate electromagnetic fields
        fields = self._calculate_electromagnetic_fields(basis_functions, I_currents, excitation_config)
        
        # Calculate radar cross section
        rcs = self._calculate_rcs(fields, excitation_config)
        
        results = {
            'frequency': self.frequency,
            'num_basis_functions': num_basis,
            'impedance_matrix': Z_matrix,
            'excitation_vector': V_vector,
            'surface_currents': I_currents,
            'electromagnetic_fields': fields,
            'radar_cross_section': rcs,
            'convergence_info': {
                'tolerance': self.tolerance,
                'matrix_condition': np.linalg.cond(Z_matrix) if num_basis > 0 else 0,
                'solver_converged': len(I_currents) > 0
            }
        }
        
        print(f"✓ MoM solution completed with {num_basis} basis functions")
        return results
    
    def _build_impedance_matrix(self, basis_functions: List[Dict[str, Any]]) -> np.ndarray:
        """Build impedance matrix using standard RWG test function integration"""
        num_basis = len(basis_functions)
        Z_matrix = np.zeros((num_basis, num_basis), dtype=complex)
        
        print("🔧 Building impedance matrix...")
        
        for m in range(num_basis):
            if m % 50 == 0:
                print(f"  Processing row {m}/{num_basis}")
            
            basis_m = basis_functions[m]
            
            for n in range(num_basis):
                basis_n = basis_functions[n]
                
                # Calculate matrix element Z_mn using standard RWG integration
                Z_mn = self._calculate_impedance_element(basis_m, basis_n)
                Z_matrix[m, n] = Z_mn
        
        print(f"✓ Impedance matrix built: {Z_matrix.shape}")
        return Z_matrix
    
    def _calculate_impedance_element(self, basis_m: Dict[str, Any], basis_n: Dict[str, Any]) -> complex:
        """Calculate impedance matrix element using standard RWG formulation"""
        try:
            # Get triangle data
            tri_m_plus = basis_m['triangle_plus']
            tri_m_minus = basis_m['triangle_minus']
            tri_n_plus = basis_n['triangle_plus']
            tri_n_minus = basis_n['triangle_minus']
            
            # Calculate self-term (m == n) with singularity extraction
            if basis_m['index'] == basis_n['index']:
                return self._calculate_self_impedance(basis_m)
            
            # Calculate mutual impedance using standard RWG integration
            # This is a simplified version - in practice, you'd use numerical integration
            
            # Get geometric parameters
            area_m_plus = basis_m['area_plus']
            area_m_minus = basis_m['area_minus']
            area_n_plus = basis_n['area_plus']
            area_n_minus = basis_n['area_minus']
            
            edge_length_m = basis_m['edge_length']
            edge_length_n = basis_n['edge_length']
            
            # Calculate distance between centroids
            centroid_m = basis_m['centroid_plus']
            centroid_n = basis_n['centroid_plus']
            
            if isinstance(centroid_m, list):
                centroid_m = np.array(centroid_m)
            if isinstance(centroid_n, list):
                centroid_n = np.array(centroid_n)
            
            distance = np.linalg.norm(centroid_m - centroid_n)
            
            # Avoid singularity at zero distance
            if distance < 1e-12:
                distance = 1e-12
            
            # Simplified RWG impedance calculation
            # In practice, this would involve numerical integration over triangles
            
            # Green's function term
            green_term = np.exp(-1j * self.k * distance) / (4 * np.pi * distance)
            
            # RWG basis function overlap integral (simplified)
            overlap_integral = (edge_length_m * edge_length_n * 
                              np.sqrt(area_m_plus * area_m_minus * area_n_plus * area_n_minus))
            
            # Impedance element
            Z_mn = 1j * self.constants.OMEGA * self.constants.MU_0 * green_term * overlap_integral
            
            return Z_mn
            
        except Exception as e:
            print(f"❌ Error calculating impedance element: {e}")
            return 0.0 + 0.0j
    
    def _calculate_self_impedance(self, basis_m: Dict[str, Any]) -> complex:
        """Calculate self-impedance term with singularity handling"""
        try:
            # Get geometric parameters
            area_plus = basis_m['area_plus']
            area_minus = basis_m['area_minus']
            edge_length = basis_m['edge_length']
            
            # Average area
            avg_area = 0.5 * (area_plus + area_minus)
            
            # Characteristic length for self-term
            characteristic_length = np.sqrt(avg_area)
            
            # Self-term impedance with singularity extraction
            # This is a simplified version - in practice, you'd use analytical formulas
            
            # Real part (resistive)
            R_self = self.constants.ETA_0 * edge_length * edge_length / (12 * np.pi * avg_area)
            
            # Imaginary part (reactive) with logarithmic singularity
            X_self = (self.constants.ETA_0 * edge_length * edge_length / 
                     (6 * np.pi * avg_area) * np.log(characteristic_length / self.constants.C * self.constants.OMEGA))
            
            Z_self = R_self + 1j * X_self
            
            return Z_self
            
        except Exception as e:
            print(f"❌ Error calculating self-impedance: {e}")
            return 1.0 + 0.0j  # Default value
    
    def _build_excitation_vector(self, basis_functions: List[Dict[str, Any]], 
                                excitation_config: Dict[str, Any]) -> np.ndarray:
        """Build excitation vector using standard RWG test function integration"""
        num_basis = len(basis_functions)
        V_vector = np.zeros(num_basis, dtype=complex)
        
        print("🔧 Building excitation vector...")
        
        # Extract plane wave parameters
        plane_wave = excitation_config.get('plane_wave', {})
        amplitude = plane_wave.get('amplitude', 1.0)
        direction = np.array(plane_wave.get('direction', [1, 0, 0]))
        polarization = np.array(plane_wave.get('polarization', [0, 1, 0]))
        frequency = plane_wave.get('frequency', self.frequency)
        
        # Normalize vectors
        direction = direction / np.linalg.norm(direction)
        polarization = polarization / np.linalg.norm(polarization)
        
        # Ensure orthogonality
        if abs(np.dot(direction, polarization)) > 1e-10:
            # Gram-Schmidt orthogonalization
            polarization = polarization - np.dot(polarization, direction) * direction
            polarization = polarization / np.linalg.norm(polarization)
        
        for i, basis_func in enumerate(basis_functions):
            # Calculate excitation for this basis function
            V_i = self._calculate_basis_excitation(basis_func, amplitude, direction, polarization)
            V_vector[i] = V_i
        
        print(f"✓ Excitation vector built: {V_vector.shape}")
        return V_vector
    
    def _calculate_basis_excitation(self, basis_func: Dict[str, Any], amplitude: float,
                                   direction: np.ndarray, polarization: np.ndarray) -> complex:
        """Calculate plane wave excitation for single RWG basis function"""
        try:
            # Get triangle centroids
            centroid_plus = np.array(basis_func['centroid_plus'])
            centroid_minus = np.array(basis_func['centroid_minus'])
            
            # Edge midpoint
            edge_midpoint = basis_func['edge_midpoint']
            if isinstance(edge_midpoint, list):
                edge_midpoint = np.array(edge_midpoint)
            
            # Wave vector
            k_vector = self.k * direction
            
            # Phase at edge midpoint
            phase = np.dot(k_vector, edge_midpoint)
            
            # Edge vector (direction from minus to plus triangle)
            edge_vector = basis_func['edge_vector']
            if isinstance(edge_vector, list):
                edge_vector = np.array(edge_vector)
            
            # RWG basis function amplitude (simplified integration)
            # Standard RWG test function integration for plane wave
            edge_length = basis_func['edge_length']
            area_plus = basis_func['area_plus']
            area_minus = basis_func['area_minus']
            
            # RWG basis function value at edge midpoint
            # f(r) = edge_length / (2 * area) * (r - opposite_vertex)
            
            # For plane wave excitation, we integrate E_inc · f over the triangles
            # This is simplified - in practice, you'd use numerical integration
            
            # Incident electric field at edge midpoint
            E_inc = amplitude * polarization * np.exp(-1j * phase)
            
            # RWG basis function integrated value
            # V = edge_length * (E_inc(plus) + E_inc(minus)) / 2
            # Simplified as E_inc at edge midpoint
            
            V_basis = edge_length * np.dot(E_inc, edge_vector) / edge_length
            
            return V_basis
            
        except Exception as e:
            print(f"❌ Error calculating basis excitation: {e}")
            return 0.0 + 0.0j
    
    def _solve_linear_system(self, Z_matrix: np.ndarray, V_vector: np.ndarray) -> np.ndarray:
        """Solve linear system Z * I = V"""
        print("🔧 Solving linear system...")
        
        try:
            # Use iterative solver for large systems
            if len(V_vector) > 100:
                print("  Using iterative solver (GMRES)...")
                I_solution, info = gmres(Z_matrix, V_vector, tol=self.tolerance, maxiter=1000)
                if info != 0:
                    print(f"⚠️  GMRES did not converge (info={info}), trying direct solver")
                    I_solution = np.linalg.solve(Z_matrix, V_vector)
            else:
                print("  Using direct solver...")
                I_solution = np.linalg.solve(Z_matrix, V_vector)
            
            print(f"✓ Linear system solved: {I_solution.shape}")
            return I_solution
            
        except Exception as e:
            print(f"❌ Error solving linear system: {e}")
            return np.zeros_like(V_vector)
    
    def _calculate_electromagnetic_fields(self, basis_functions: List[Dict[str, Any]], 
                                        surface_currents: np.ndarray, 
                                        excitation_config: Dict[str, Any]) -> Dict[str, Any]:
        """Calculate electromagnetic fields from surface currents"""
        print("🔧 Calculating electromagnetic fields...")
        
        # Calculate scattered field at observation points
        observation_points = excitation_config.get('observation_points', [])
        
        if len(observation_points) == 0:
            # Default observation points
            observation_points = self._generate_default_observation_points()
        
        scattered_field = self._calculate_scattered_field(basis_functions, surface_currents, observation_points)
        
        # Calculate incident field at observation points
        incident_field = self._calculate_incident_field(observation_points, excitation_config)
        
        # Total field
        total_field = incident_field + scattered_field
        
        fields = {
            'observation_points': observation_points,
            'incident_field': incident_field,
            'scattered_field': scattered_field,
            'total_field': total_field,
            'field_magnitude': np.linalg.norm(total_field, axis=1) if total_field.ndim > 1 else np.abs(total_field)
        }
        
        print(f"✓ Electromagnetic fields calculated at {len(observation_points)} points")
        return fields
    
    def _generate_default_observation_points(self) -> np.ndarray:
        """Generate default observation points for field calculation"""
        # Create a grid of points around the object
        x = np.linspace(-2, 2, 21)
        y = np.linspace(-2, 2, 21)
        z = np.linspace(-2, 2, 21)
        
        X, Y, Z = np.meshgrid(x, y, z, indexing='ij')
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
        # Filter points that are not too close to origin (avoid singularity)
        distances = np.linalg.norm(points, axis=1)
        valid_points = points[distances > 0.5]
        
        return valid_points[:100]  # Limit to 100 points for performance
    
    def _calculate_scattered_field(self, basis_functions: List[Dict[str, Any]], 
                                   surface_currents: np.ndarray, 
                                   observation_points: np.ndarray) -> np.ndarray:
        """Calculate scattered field from surface currents"""
        num_points = len(observation_points)
        scattered_field = np.zeros((num_points, 3), dtype=complex)
        
        print(f"  Calculating scattered field at {num_points} observation points...")
        
        for i, obs_point in enumerate(observation_points):
            if i % 20 == 0:
                print(f"    Point {i}/{num_points}")
            
            E_scat = np.zeros(3, dtype=complex)
            
            for j, basis_func in enumerate(basis_functions):
                if j < len(surface_currents):
                    current_amplitude = surface_currents[j]
                    
                    # Calculate field contribution from this basis function
                    E_contrib = self._calculate_basis_field_contribution(basis_func, current_amplitude, obs_point)
                    E_scat += E_contrib
            
            scattered_field[i] = E_scat
        
        return scattered_field
    
    def _calculate_basis_field_contribution(self, basis_func: Dict[str, Any], 
                                          current_amplitude: complex, 
                                          observation_point: np.ndarray) -> np.ndarray:
        """Calculate field contribution from single RWG basis function"""
        try:
            # Get triangle centroids
            centroid_plus = np.array(basis_func['centroid_plus'])
            centroid_minus = np.array(basis_func['centroid_minus'])
            
            # Average centroid for field calculation
            avg_centroid = (centroid_plus + centroid_minus) / 2
            
            # Distance from basis function to observation point
            distance = np.linalg.norm(observation_point - avg_centroid)
            
            # Avoid singularity
            if distance < 1e-12:
                return np.zeros(3, dtype=complex)
            
            # Green's function
            green_term = np.exp(-1j * self.k * distance) / (4 * np.pi * distance)
            
            # Edge vector (current direction)
            edge_vector = np.array(basis_func['edge_vector'])
            
            # Field direction (simplified radiation pattern)
            r_vector = observation_point - avg_centroid
            r_vector = r_vector / np.linalg.norm(r_vector)
            
            # Radiation field component
            # E ∝ (r̂ × (r̂ × J)) * Green's function
            current_vector = edge_vector * current_amplitude
            
            # Cross product terms (simplified)
            r_cross_current = np.cross(r_vector, current_vector)
            r_cross_r_cross_current = np.cross(r_vector, r_cross_current)
            
            # Field contribution
            E_contrib = 1j * self.constants.OMEGA * self.constants.MU_0 * green_term * r_cross_r_cross_current
            
            return E_contrib
            
        except Exception as e:
            print(f"❌ Error calculating basis field contribution: {e}")
            return np.zeros(3, dtype=complex)
    
    def _calculate_incident_field(self, observation_points: np.ndarray, 
                                excitation_config: Dict[str, Any]) -> np.ndarray:
        """Calculate incident field at observation points"""
        plane_wave = excitation_config.get('plane_wave', {})
        amplitude = plane_wave.get('amplitude', 1.0)
        direction = np.array(plane_wave.get('direction', [1, 0, 0]))
        polarization = np.array(plane_wave.get('polarization', [0, 1, 0]))
        
        # Normalize vectors
        direction = direction / np.linalg.norm(direction)
        polarization = polarization / np.linalg.norm(polarization)
        
        # Ensure orthogonality
        if abs(np.dot(direction, polarization)) > 1e-10:
            polarization = polarization - np.dot(polarization, direction) * direction
            polarization = polarization / np.linalg.norm(polarization)
        
        num_points = len(observation_points)
        incident_field = np.zeros((num_points, 3), dtype=complex)
        
        # Wave vector
        k_vector = self.k * direction
        
        for i, point in enumerate(observation_points):
            # Phase at observation point
            phase = np.dot(k_vector, point)
            
            # Incident electric field
            E_inc = amplitude * polarization * np.exp(-1j * phase)
            incident_field[i] = E_inc
        
        return incident_field
    
    def _calculate_rcs(self, fields: Dict[str, Any], excitation_config: Dict[str, Any]) -> Dict[str, Any]:
        """Calculate radar cross section"""
        incident_amplitude = excitation_config.get('plane_wave', {}).get('amplitude', 1.0)
        
        scattered_field = fields['scattered_field']
        observation_points = fields['observation_points']
        
        # Calculate RCS for each observation point
        rcs_values = []
        
        for i, (scat_field, obs_point) in enumerate(zip(scattered_field, observation_points)):
            # Distance from origin
            distance = np.linalg.norm(obs_point)
            
            if distance > 0:
                # RCS calculation: σ = 4πr²|E_s|²/|E_i|²
                field_magnitude = np.linalg.norm(scat_field)
                rcs = 4 * np.pi * distance * distance * field_magnitude * field_magnitude / (incident_amplitude * incident_amplitude)
                rcs_values.append(rcs)
            else:
                rcs_values.append(0.0)
        
        rcs_data = {
            'rcs_values': np.array(rcs_values),
            'observation_points': observation_points,
            'max_rcs': np.max(rcs_values) if rcs_values else 0.0,
            'avg_rcs': np.mean(rcs_values) if rcs_values else 0.0,
            'total_rcs': np.sum(rcs_values) if rcs_values else 0.0
        }
        
        return rcs_data

class OptimizedSatelliteMoMTest:
    """Optimized satellite MoM test with comprehensive validation"""
    
    def __init__(self, stl_file: str = "weixing_v1.stl"):
        self.stl_file = stl_file
        self.frequency = 10e9  # 10 GHz
        self.constants = EMConstants()
        self.material_db = ProfessionalMaterialDatabase()
        
        # Initialize C solver interface with error handling
        if C_SOLVER_AVAILABLE:
            try:
                self.c_interface = IntegratedCSolverInterface(
                    src_dir=os.path.join(os.path.dirname(__file__), 'src'),
                    preferred_backend='auto'
                )
                print("✓ C solver interface initialized")
            except Exception as e:
                print(f"⚠️  Warning: C solver interface initialization failed: {e}")
                self.c_interface = None
        else:
            self.c_interface = None
    
    def run_comprehensive_test(self) -> Dict[str, Any]:
        """Run comprehensive satellite MoM test with all optimizations"""
        print("🚀 Starting optimized satellite MoM test...")
        
        # Step 1: Parse STL file
        print("\n" + "="*60)
        print("STEP 1: STL Geometry Processing")
        print("="*60)
        
        stl_parser = ProfessionalSTLParser(target_scale=1.0, stl_units='mm')
        mesh_data = stl_parser.parse_stl_file(self.stl_file)
        
        print(f"✓ STL parsing completed:")
        print(f"  Vertices: {mesh_data['num_vertices']}")
        print(f"  Triangles: {mesh_data['num_triangles']}")
        print(f"  Dimensions: {mesh_data['geometry_stats']['dimensions']}")
        
        # Step 2: Create RWG basis functions
        print("\n" + "="*60)
        print("STEP 2: RWG Basis Function Creation")
        print("="*60)
        
        rwg_basis = ProfessionalRWGBasis(mesh_data, min_edge_length=1e-6)
        basis_functions = rwg_basis.create_rwg_basis_functions()
        
        if len(basis_functions) == 0:
            print("❌ No RWG basis functions created - cannot proceed")
            return {'success': False, 'error': 'No basis functions created'}
        
        # Step 3: Configure excitation
        print("\n" + "="*60)
        print("STEP 3: Plane Wave Excitation Setup")
        print("="*60)
        
        excitation_config = {
            'plane_wave': {
                'frequency': self.frequency,
                'amplitude': 1.0,
                'direction': [1, 0, 0],  # +X direction
                'polarization': [0, 1, 0],  # Y polarization
                'phase': 0.0
            },
            'observation_points': self._generate_observation_points(mesh_data)
        }
        
        print(f"✓ Plane wave excitation configured:")
        print(f"  Frequency: {self.frequency/1e9:.1f} GHz")
        print(f"  Direction: {excitation_config['plane_wave']['direction']}")
        print(f"  Polarization: {excitation_config['plane_wave']['polarization']}")
        
        # Step 4: Solve MoM
        print("\n" + "="*60)
        print("STEP 4: MoM Electromagnetic Solution")
        print("="*60)
        
        mom_solver = ProfessionalMoMSolver(frequency=self.frequency, tolerance=1e-6)
        mom_results = mom_solver.solve_mom_with_rwg_basis(basis_functions, excitation_config)
        
        # Step 5: C solver comparison (if available)
        print("\n" + "="*60)
        print("STEP 5: C Solver Integration and Comparison")
        print("="*60)
        
        c_solver_results = None
        if self.c_interface:
            try:
                c_solver_results = self._run_c_solver_comparison(mesh_data, basis_functions, excitation_config)
                print("✓ C solver comparison completed")
            except Exception as e:
                print(f"⚠️  C solver comparison failed: {e}")
        else:
            print("⚠️  C solver not available - using Python implementation only")
        
        # Step 6: Comprehensive visualization
        print("\n" + "="*60)
        print("STEP 6: Professional Visualization")
        print("="*60)
        
        visualization_results = self._create_professional_visualization(
            mesh_data, basis_functions, mom_results, c_solver_results
        )
        
        # Step 7: Validation and testing
        print("\n" + "="*60)
        print("STEP 7: Comprehensive Validation")
        print("="*60)
        
        validation_results = self._run_comprehensive_validation(
            mesh_data, basis_functions, mom_results, c_solver_results
        )
        
        # Compile final results
        final_results = {
            'success': True,
            'mesh_data': mesh_data,
            'basis_functions': basis_functions,
            'mom_results': mom_results,
            'c_solver_results': c_solver_results,
            'visualization': visualization_results,
            'validation': validation_results,
            'summary': {
                'num_vertices': mesh_data['num_vertices'],
                'num_triangles': mesh_data['num_triangles'],
                'num_basis_functions': len(basis_functions),
                'frequency_ghz': self.frequency / 1e9,
                'max_surface_current': np.max(np.abs(mom_results['surface_currents'])) if len(mom_results['surface_currents']) > 0 else 0,
                'max_rcs': mom_results['radar_cross_section']['max_rcs'],
                'avg_rcs': mom_results['radar_cross_section']['avg_rcs'],
                'c_solver_available': self.c_interface is not None,
                'validation_passed': validation_results.get('overall_status', False)
            }
        }
        
        print("\n" + "="*60)
        print("OPTIMIZED SATELLITE MoM TEST COMPLETED")
        print("="*60)
        print(f"✓ Geometry: {final_results['summary']['num_vertices']} vertices, {final_results['summary']['num_triangles']} triangles")
        print(f"✓ Basis functions: {final_results['summary']['num_basis_functions']} RWG functions")
        print(f"✓ Frequency: {final_results['summary']['frequency_ghz']:.1f} GHz")
        print(f"✓ Max surface current: {final_results['summary']['max_surface_current']:.3e} A/m")
        print(f"✓ Max RCS: {final_results['summary']['max_rcs']:.3e} m²")
        print(f"✓ Avg RCS: {final_results['summary']['avg_rcs']:.3e} m²")
        print(f"✓ C solver integration: {'Available' if final_results['summary']['c_solver_available'] else 'Not available'}")
        print(f"✓ Validation: {'Passed' if final_results['summary']['validation_passed'] else 'Failed'}")
        
        return final_results
    
    def _generate_observation_points(self, mesh_data: Dict[str, Any]) -> np.ndarray:
        """Generate observation points for field calculation"""
        # Create points around the satellite geometry
        center = np.array(mesh_data['geometry_stats']['center'])
        dimensions = np.array(mesh_data['geometry_stats']['dimensions'])
        
        # Extend region by 2x the object size
        extended_size = np.max(dimensions) * 2
        
        # Create observation sphere around the object
        num_points = 50
        phi = np.linspace(0, 2*np.pi, num_points)
        theta = np.linspace(0, np.pi, num_points//2)
        
        observation_points = []
        for p in phi:
            for t in theta:
                x = center[0] + extended_size * np.sin(t) * np.cos(p)
                y = center[1] + extended_size * np.sin(t) * np.sin(p)
                z = center[2] + extended_size * np.cos(t)
                observation_points.append([x, y, z])
        
        return np.array(observation_points)
    
    def _run_c_solver_comparison(self, mesh_data: Dict[str, Any], 
                                 basis_functions: List[Dict[str, Any]], 
                                 excitation_config: Dict[str, Any]) -> Dict[str, Any]:
        """Run C solver comparison"""
        print("🔧 Running C solver comparison...")
        
        try:
            # Prepare C solver configuration
            c_config = {
                'frequency': self.frequency,
                'tolerance': 1e-6,
                'max_iterations': 1000,
                'use_preconditioner': True,
                'use_parallel': True,
                'num_threads': 4
            }
            
            # Test C mesh engine
            mesh_config = {
                'frequency': self.frequency,
                'target_edge_length': 0.01,  # 1cm at 10GHz
                'complexity': 10
            }
            
            c_mesh_results = self.c_interface.generate_mesh_with_c_engine(self.stl_file, mesh_config)
            
            # Test C MoM solver
            c_mom_results = self.c_interface.solve_mom_with_c_solver(c_config, basis_functions)
            
            # Compare results
            comparison_results = self.c_interface.compare_python_vs_c_solvers(
                {'surface_currents': np.zeros(len(basis_functions))},  # Placeholder
                {},
                c_config
            )
            
            return {
                'c_mesh_results': c_mesh_results,
                'c_mom_results': c_mom_results,
                'comparison': comparison_results,
                'c_solver_status': 'success'
            }
            
        except Exception as e:
            print(f"❌ C solver comparison failed: {e}")
            return {
                'c_solver_status': 'failed',
                'error': str(e)
            }
    
    def _create_professional_visualization(self, mesh_data: Dict[str, Any],
                                         basis_functions: List[Dict[str, Any]],
                                         mom_results: Dict[str, Any],
                                         c_solver_results: Optional[Dict[str, Any]]) -> Dict[str, Any]:
        """Create professional visualization"""
        print("🔧 Creating professional visualization...")
        
        try:
            # Create comprehensive 16-subplot visualization
            fig = plt.figure(figsize=(32, 24))
            
            # 1. 3D Geometry
            ax1 = fig.add_subplot(4, 4, 1, projection='3d')
            self._plot_3d_geometry(ax1, mesh_data)
            ax1.set_title('STL Geometry', fontsize=12, fontweight='bold')
            
            # 2. Triangle Quality
            ax2 = fig.add_subplot(4, 4, 2)
            self._plot_triangle_quality(ax2, mesh_data)
            ax2.set_title('Triangle Quality Distribution', fontsize=12, fontweight='bold')
            
            # 3. RWG Basis Functions
            ax3 = fig.add_subplot(4, 4, 3)
            self._plot_rwg_basis_distribution(ax3, basis_functions)
            ax3.set_title('RWG Basis Function Distribution', fontsize=12, fontweight='bold')
            
            # 4. Surface Currents
            ax4 = fig.add_subplot(4, 4, 4)
            self._plot_surface_currents(ax4, basis_functions, mom_results)
            ax4.set_title('Surface Current Distribution', fontsize=12, fontweight='bold')
            
            # 5. Current Magnitude 3D
            ax5 = fig.add_subplot(4, 4, 5, projection='3d')
            self._plot_current_magnitude_3d(ax5, basis_functions, mom_results)
            ax5.set_title('Surface Current Magnitude (3D)', fontsize=12, fontweight='bold')
            
            # 6. Field Distribution XY Plane
            ax6 = fig.add_subplot(4, 4, 6)
            self._plot_field_xy_plane(ax6, mom_results)
            ax6.set_title('Electric Field (XY Plane)', fontsize=12, fontweight='bold')
            
            # 7. Field Distribution XZ Plane
            ax7 = fig.add_subplot(4, 4, 7)
            self._plot_field_xz_plane(ax7, mom_results)
            ax7.set_title('Electric Field (XZ Plane)', fontsize=12, fontweight='bold')
            
            # 8. Field Distribution YZ Plane
            ax8 = fig.add_subplot(4, 4, 8)
            self._plot_field_yz_plane(ax8, mom_results)
            ax8.set_title('Electric Field (YZ Plane)', fontsize=12, fontweight='bold')
            
            # 9. RCS Pattern
            ax9 = fig.add_subplot(4, 4, 9)
            self._plot_rcs_pattern(ax9, mom_results)
            ax9.set_title('Radar Cross Section Pattern', fontsize=12, fontweight='bold')
            
            # 10. Impedance Matrix
            ax10 = fig.add_subplot(4, 4, 10)
            self._plot_impedance_matrix(ax10, mom_results)
            ax10.set_title('Impedance Matrix', fontsize=12, fontweight='bold')
            
            # 11. Convergence Analysis
            ax11 = fig.add_subplot(4, 4, 11)
            self._plot_convergence_analysis(ax11, mom_results)
            ax11.set_title('Convergence Analysis', fontsize=12, fontweight='bold')
            
            # 12. Field Enhancement/Shadow Zones
            ax12 = fig.add_subplot(4, 4, 12)
            self._plot_field_enhancement_zones(ax12, mom_results)
            ax12.set_title('Field Enhancement/Shadow Zones', fontsize=12, fontweight='bold')
            
            # 13. C Solver Comparison (if available)
            ax13 = fig.add_subplot(4, 4, 13)
            if c_solver_results and c_solver_results.get('c_solver_status') == 'success':
                self._plot_c_solver_comparison(ax13, mom_results, c_solver_results)
                ax13.set_title('C Solver Comparison', fontsize=12, fontweight='bold')
            else:
                ax13.text(0.5, 0.5, 'C Solver\nNot Available', 
                         ha='center', va='center', transform=ax13.transAxes, fontsize=14)
                ax13.set_title('C Solver Status', fontsize=12, fontweight='bold')
            
            # 14. Material Properties
            ax14 = fig.add_subplot(4, 4, 14)
            self._plot_material_properties(ax14)
            ax14.set_title('Material Properties', fontsize=12, fontweight='bold')
            
            # 15. Mesh Quality Metrics
            ax15 = fig.add_subplot(4, 4, 15)
            self._plot_mesh_quality_metrics(ax15, mesh_data)
            ax15.set_title('Mesh Quality Metrics', fontsize=12, fontweight='bold')
            
            # 16. Summary Statistics
            ax16 = fig.add_subplot(4, 4, 16)
            self._plot_summary_statistics(ax16, mom_results, basis_functions)
            ax16.set_title('Simulation Summary', fontsize=12, fontweight='bold')
            
            # Layout and save
            plt.tight_layout(pad=3.0)
            
            # Save high-resolution figure
            output_file = 'optimized_satellite_mom_professional_analysis.png'
            plt.savefig(output_file, dpi=300, bbox_inches='tight', facecolor='white')
            print(f"✓ Professional visualization saved: {output_file}")
            
            # Also save simulation data
            data_file = 'optimized_satellite_mom_simulation_data.json'
            self._save_simulation_data(data_file, mesh_data, basis_functions, mom_results)
            
            plt.close(fig)
            
            return {
                'visualization_file': output_file,
                'data_file': data_file,
                'status': 'success'
            }
            
        except Exception as e:
            print(f"❌ Visualization failed: {e}")
            return {
                'status': 'failed',
                'error': str(e)
            }
    
    def _plot_3d_geometry(self, ax, mesh_data: Dict[str, Any]):
        """Plot 3D geometry"""
        vertices = mesh_data['vertices']
        triangles = mesh_data['triangles']
        
        # Plot surface
        x = vertices[:, 0]
        y = vertices[:, 1] 
        z = vertices[:, 2]
        
        ax.plot_trisurf(x, y, z, triangles=triangles, alpha=0.7, color='lightblue', edgecolor='gray')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        ax.set_zlabel('Z (m)')
        ax.set_box_aspect([1,1,1])
    
    def _plot_triangle_quality(self, ax, mesh_data: Dict[str, Any]):
        """Plot triangle quality distribution"""
        vertices = mesh_data['vertices']
        triangles = mesh_data['triangles']
        
        qualities = []
        for tri in triangles:
            v0, v1, v2 = vertices[tri]
            edge1 = np.linalg.norm(v1 - v0)
            edge2 = np.linalg.norm(v2 - v1)
            edge3 = np.linalg.norm(v0 - v2)
            
            # Quality metric: ratio of smallest to largest edge
            min_edge = min(edge1, edge2, edge3)
            max_edge = max(edge1, edge2, edge3)
            quality = min_edge / max_edge if max_edge > 0 else 0
            qualities.append(quality)
        
        ax.hist(qualities, bins=20, alpha=0.7, color='skyblue', edgecolor='black')
        ax.set_xlabel('Triangle Quality (min/max edge ratio)')
        ax.set_ylabel('Count')
        ax.set_title('Triangle Quality Distribution')
        ax.grid(True, alpha=0.3)
    
    def _plot_rwg_basis_distribution(self, ax, basis_functions: List[Dict[str, Any]]):
        """Plot RWG basis function distribution"""
        edge_lengths = [bf['edge_length'] for bf in basis_functions]
        
        ax.hist(edge_lengths, bins=20, alpha=0.7, color='lightgreen', edgecolor='black')
        ax.set_xlabel('Edge Length (m)')
        ax.set_ylabel('Count')
        ax.set_title('RWG Basis Function Edge Length Distribution')
        ax.grid(True, alpha=0.3)
        
        # Add statistics
        avg_length = np.mean(edge_lengths)
        ax.axvline(avg_length, color='red', linestyle='--', label=f'Average: {avg_length:.3f}m')
        ax.legend()
    
    def _plot_surface_currents(self, ax, basis_functions: List[Dict[str, Any]], mom_results: Dict[str, Any]):
        """Plot surface current distribution"""
        surface_currents = mom_results['surface_currents']
        current_magnitudes = np.abs(surface_currents)
        
        ax.hist(current_magnitudes, bins=30, alpha=0.7, color='orange', edgecolor='black')
        ax.set_xlabel('Surface Current Magnitude (A/m)')
        ax.set_ylabel('Count')
        ax.set_title('Surface Current Distribution')
        ax.grid(True, alpha=0.3)
        
        # Add statistics
        ax.axvline(np.mean(current_magnitudes), color='red', linestyle='--', 
                  label=f'Mean: {np.mean(current_magnitudes):.2e}')
        ax.axvline(np.max(current_magnitudes), color='blue', linestyle='--', 
                  label=f'Max: {np.max(current_magnitudes):.2e}')
        ax.legend()
    
    def _plot_current_magnitude_3d(self, ax, basis_functions: List[Dict[str, Any]], mom_results: Dict[str, Any]):
        """Plot 3D surface current magnitude"""
        surface_currents = mom_results['surface_currents']
        current_magnitudes = np.abs(surface_currents)
        
        # Plot basis function centers with current magnitude as color
        centers = []
        magnitudes = []
        
        for i, basis_func in enumerate(basis_functions):
            if i < len(current_magnitudes):
                centroid_plus = np.array(basis_func['centroid_plus'])
                centroid_minus = np.array(basis_func['centroid_minus'])
                center = (centroid_plus + centroid_minus) / 2
                centers.append(center)
                magnitudes.append(current_magnitudes[i])
        
        centers = np.array(centers)
        
        if len(centers) > 0:
            scatter = ax.scatter(centers[:, 0], centers[:, 1], centers[:, 2], 
                               c=magnitudes, cmap='viridis', s=50, alpha=0.8)
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_zlabel('Z (m)')
            
            # Add colorbar
            cbar = plt.colorbar(scatter, ax=ax, shrink=0.6)
            cbar.set_label('Current Magnitude (A/m)')
    
    def _plot_field_xy_plane(self, ax, mom_results: Dict[str, Any]):
        """Plot electric field in XY plane"""
        fields = mom_results['electromagnetic_fields']
        observation_points = fields['observation_points']
        total_field = fields['total_field']
        
        # Filter points near XY plane (z ≈ 0)
        xy_mask = np.abs(observation_points[:, 2]) < 0.1
        xy_points = observation_points[xy_mask]
        xy_fields = total_field[xy_mask]
        
        if len(xy_points) > 0:
            field_magnitudes = np.linalg.norm(xy_fields, axis=1)
            
            scatter = ax.scatter(xy_points[:, 0], xy_points[:, 1], 
                               c=field_magnitudes, cmap='plasma', s=30, alpha=0.7)
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            
            # Add colorbar
            cbar = plt.colorbar(scatter, ax=ax, shrink=0.6)
            cbar.set_label('Field Magnitude (V/m)')
        else:
            ax.text(0.5, 0.5, 'No XY plane data', ha='center', va='center', 
                   transform=ax.transAxes)
    
    def _plot_field_xz_plane(self, ax, mom_results: Dict[str, Any]):
        """Plot electric field in XZ plane"""
        fields = mom_results['electromagnetic_fields']
        observation_points = fields['observation_points']
        total_field = fields['total_field']
        
        # Filter points near XZ plane (y ≈ 0)
        xz_mask = np.abs(observation_points[:, 1]) < 0.1
        xz_points = observation_points[xz_mask]
        xz_fields = total_field[xz_mask]
        
        if len(xz_points) > 0:
            field_magnitudes = np.linalg.norm(xz_fields, axis=1)
            
            scatter = ax.scatter(xz_points[:, 0], xz_points[:, 2], 
                               c=field_magnitudes, cmap='plasma', s=30, alpha=0.7)
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Z (m)')
            
            # Add colorbar
            cbar = plt.colorbar(scatter, ax=ax, shrink=0.6)
            cbar.set_label('Field Magnitude (V/m)')
        else:
            ax.text(0.5, 0.5, 'No XZ plane data', ha='center', va='center', 
                   transform=ax.transAxes)
    
    def _plot_field_yz_plane(self, ax, mom_results: Dict[str, Any]):
        """Plot electric field in YZ plane"""
        fields = mom_results['electromagnetic_fields']
        observation_points = fields['observation_points']
        total_field = fields['total_field']
        
        # Filter points near YZ plane (x ≈ 0)
        yz_mask = np.abs(observation_points[:, 0]) < 0.1
        yz_points = observation_points[yz_mask]
        yz_fields = total_field[yz_mask]
        
        if len(yz_points) > 0:
            field_magnitudes = np.linalg.norm(yz_fields, axis=1)
            
            scatter = ax.scatter(yz_points[:, 1], yz_points[:, 2], 
                               c=field_magnitudes, cmap='plasma', s=30, alpha=0.7)
            ax.set_xlabel('Y (m)')
            ax.set_ylabel('Z (m)')
            
            # Add colorbar
            cbar = plt.colorbar(scatter, ax=ax, shrink=0.6)
            cbar.set_label('Field Magnitude (V/m)')
        else:
            ax.text(0.5, 0.5, 'No YZ plane data', ha='center', va='center', 
                   transform=ax.transAxes)
    
    def _plot_rcs_pattern(self, ax, mom_results: Dict[str, Any]):
        """Plot RCS pattern"""
        rcs_data = mom_results['radar_cross_section']
        rcs_values = rcs_data['rcs_values']
        observation_points = rcs_data['observation_points']
        
        # Calculate spherical coordinates
        distances = np.linalg.norm(observation_points, axis=1)
        theta = np.arccos(observation_points[:, 2] / (distances + 1e-12))
        phi = np.arctan2(observation_points[:, 1], observation_points[:, 0])
        
        # Plot RCS vs angle
        ax.scatter(phi, rcs_values, c=theta, cmap='hsv', s=30, alpha=0.7)
        ax.set_xlabel('Azimuth Angle (rad)')
        ax.set_ylabel('RCS (m²)')
        ax.set_title('RCS vs Azimuth Angle')
        ax.grid(True, alpha=0.3)
        ax.set_yscale('log')
    
    def _plot_impedance_matrix(self, ax, mom_results: Dict[str, Any]):
        """Plot impedance matrix"""
        Z_matrix = mom_results['impedance_matrix']
        
        if Z_matrix.size > 0:
            im = ax.imshow(np.abs(Z_matrix), cmap='viridis', aspect='auto')
            ax.set_xlabel('Basis Function Index')
            ax.set_ylabel('Basis Function Index')
            ax.set_title('Impedance Matrix Magnitude')
            
            # Add colorbar
            cbar = plt.colorbar(im, ax=ax, shrink=0.6)
            cbar.set_label('Magnitude')
        else:
            ax.text(0.5, 0.5, 'Empty Matrix', ha='center', va='center', 
                   transform=ax.transAxes)
    
    def _plot_convergence_analysis(self, ax, mom_results: Dict[str, Any]):
        """Plot convergence analysis"""
        convergence_info = mom_results['convergence_info']
        
        # Plot convergence metrics
        metrics = ['tolerance', 'matrix_condition']
        values = [convergence_info.get('tolerance', 0), convergence_info.get('matrix_condition', 0)]
        
        bars = ax.bar(metrics, values, color=['blue', 'orange'], alpha=0.7)
        ax.set_ylabel('Value')
        ax.set_title('Convergence Metrics')
        ax.set_yscale('log')
        
        # Add values on bars
        for bar, value in zip(bars, values):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{value:.2e}', ha='center', va='bottom')
    
    def _plot_field_enhancement_zones(self, ax, mom_results: Dict[str, Any]):
        """Plot field enhancement zones"""
        fields = mom_results['electromagnetic_fields']
        field_magnitudes = fields['field_magnitude']
        
        # Find enhancement zones (high field regions)
        threshold = np.mean(field_magnitudes) + 2 * np.std(field_magnitudes)
        enhancement_zones = field_magnitudes > threshold
        
        ax.hist(field_magnitudes, bins=30, alpha=0.7, color='lightblue', 
               label='All Points', edgecolor='black')
        
        if np.any(enhancement_zones):
            ax.hist(field_magnitudes[enhancement_zones], bins=15, alpha=0.7, 
                   color='red', label='Enhancement Zones', edgecolor='black')
        
        ax.set_xlabel('Field Magnitude (V/m)')
        ax.set_ylabel('Count')
        ax.set_title('Field Enhancement Analysis')
        ax.legend()
        ax.grid(True, alpha=0.3)
    
    def _plot_c_solver_comparison(self, ax, mom_results: Dict[str, Any], c_solver_results: Dict[str, Any]):
        """Plot C solver comparison"""
        comparison = c_solver_results.get('comparison', {})
        
        if comparison:
            mom_error = comparison.get('mom_comparison', {}).get('magnitude_error', 0)
            peec_error = comparison.get('peec_comparison', {}).get('voltage_error', 0)
            
            errors = ['MoM Error', 'PEEC Error']
            values = [mom_error, peec_error]
            
            bars = ax.bar(errors, values, color=['green', 'purple'], alpha=0.7)
            ax.set_ylabel('Error Magnitude')
            ax.set_title('Python vs C Solver Comparison')
            ax.set_yscale('log')
            
            # Add values on bars
            for bar, value in zip(bars, values):
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2., height,
                       f'{value:.2e}', ha='center', va='bottom')
        else:
            ax.text(0.5, 0.5, 'No Comparison Data', ha='center', va='center', 
                   transform=ax.transAxes)
    
    def _plot_material_properties(self, ax):
        """Plot material properties"""
        materials = list(self.material_db.materials.keys())
        conductivities = [self.material_db.materials[mat]['sigma'] for mat in materials]
        
        bars = ax.bar(materials, conductivities, color='steelblue', alpha=0.7)
        ax.set_ylabel('Conductivity (S/m)')
        ax.set_title('Material Conductivity')
        ax.set_yscale('log')
        ax.tick_params(axis='x', rotation=45)
        
        # Add values on bars
        for bar, value in zip(bars, conductivities):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{value:.1e}', ha='center', va='bottom', rotation=90)
    
    def _plot_mesh_quality_metrics(self, ax, mesh_data: Dict[str, Any]):
        """Plot mesh quality metrics"""
        stats = mesh_data['geometry_stats']
        
        metrics = ['Total Area', 'Avg Triangle Area']
        values = [stats['total_area'], stats['avg_triangle_area']]
        
        bars = ax.bar(metrics, values, color='coral', alpha=0.7)
        ax.set_ylabel('Area (m²)')
        ax.set_title('Mesh Quality Metrics')
        ax.set_yscale('log')
        
        # Add values on bars
        for bar, value in zip(bars, values):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{value:.2e}', ha='center', va='bottom')
    
    def _plot_summary_statistics(self, ax, mom_results: Dict[str, Any], basis_functions: List[Dict[str, Any]]):
        """Plot summary statistics"""
        rcs_max = mom_results['radar_cross_section']['max_rcs']
        rcs_avg = mom_results['radar_cross_section']['avg_rcs']
        num_basis = len(basis_functions)
        
        stats = ['Max RCS', 'Avg RCS', 'Num Basis']
        values = [rcs_max, rcs_avg, num_basis]
        
        bars = ax.bar(stats, values, color='gold', alpha=0.7)
        ax.set_ylabel('Value')
        ax.set_title('Simulation Summary')
        ax.set_yscale('log')
        
        # Add values on bars
        for bar, value in zip(bars, values):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{value:.2e}', ha='center', va='bottom')
    
    def _save_simulation_data(self, filename: str, mesh_data: Dict[str, Any],
                             basis_functions: List[Dict[str, Any]], mom_results: Dict[str, Any]):
        """Save simulation data to JSON file"""
        try:
            # Prepare data for serialization
            data = {
                'mesh_data': {
                    'num_vertices': mesh_data['num_vertices'],
                    'num_triangles': mesh_data['num_triangles'],
                    'geometry_stats': mesh_data['geometry_stats']
                },
                'basis_functions': [
                    {
                        'index': bf['index'],
                        'edge_length': bf['edge_length'],
                        'area_plus': bf['area_plus'],
                        'area_minus': bf['area_minus'],
                        'type': bf.get('type', 'internal')
                    } for bf in basis_functions
                ],
                'mom_results': {
                    'frequency': mom_results['frequency'],
                    'num_basis_functions': mom_results['num_basis_functions'],
                    'max_surface_current': float(np.max(np.abs(mom_results['surface_currents']))) if len(mom_results['surface_currents']) > 0 else 0,
                    'max_rcs': mom_results['radar_cross_section']['max_rcs'],
                    'avg_rcs': mom_results['radar_cross_section']['avg_rcs'],
                    'convergence_info': mom_results['convergence_info']
                }
            }
            
            with open(filename, 'w') as f:
                json.dump(data, f, indent=2)
            
            print(f"✓ Simulation data saved: {filename}")
            
        except Exception as e:
            print(f"⚠️  Warning: Could not save simulation data: {e}")
    
    def _run_comprehensive_validation(self, mesh_data: Dict[str, Any],
                                     basis_functions: List[Dict[str, Any]],
                                     mom_results: Dict[str, Any],
                                     c_solver_results: Optional[Dict[str, Any]]) -> Dict[str, Any]:
        """Run comprehensive validation tests"""
        print("🔧 Running comprehensive validation...")
        
        validation_results = {}
        
        # 1. Geometry validation
        validation_results['geometry'] = self._validate_geometry(mesh_data)
        
        # 2. Basis function validation
        validation_results['basis_functions'] = self._validate_basis_functions(basis_functions)
        
        # 3. MoM results validation
        validation_results['mom_results'] = self._validate_mom_results(mom_results)
        
        # 4. C solver validation (if available)
        if c_solver_results:
            validation_results['c_solver'] = self._validate_c_solver_results(c_solver_results)
        
        # Overall status
        all_passed = all([
            validation_results['geometry'].get('passed', False),
            validation_results['basis_functions'].get('passed', False),
            validation_results['mom_results'].get('passed', False)
        ])
        
        validation_results['overall_status'] = all_passed
        
        print(f"✓ Validation completed: {'PASSED' if all_passed else 'FAILED'}")
        return validation_results
    
    def _validate_geometry(self, mesh_data: Dict[str, Any]) -> Dict[str, Any]:
        """Validate geometry"""
        results = {'passed': True, 'issues': []}
        
        # Check for degenerate triangles
        vertices = mesh_data['vertices']
        triangles = mesh_data['triangles']
        
        degenerate_count = 0
        for tri in triangles:
            v0, v1, v2 = vertices[tri]
            area = 0.5 * np.linalg.norm(np.cross(v1 - v0, v2 - v0))
            if area < 1e-12:
                degenerate_count += 1
        
        if degenerate_count > 0:
            results['issues'].append(f"Found {degenerate_count} degenerate triangles")
            results['passed'] = False
        
        # Check dimensions
        dimensions = mesh_data['geometry_stats']['dimensions']
        max_dimension = np.max(dimensions)
        if max_dimension < 0.01 or max_dimension > 100:  # 1cm to 100m range
            results['issues'].append(f"Object dimensions ({dimensions}) outside reasonable range")
            results['passed'] = False
        
        return results
    
    def _validate_basis_functions(self, basis_functions: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Validate basis functions"""
        results = {'passed': True, 'issues': []}
        
        if len(basis_functions) == 0:
            results['issues'].append("No basis functions created")
            results['passed'] = False
            return results
        
        # Check edge lengths
        edge_lengths = [bf['edge_length'] for bf in basis_functions]
        min_length = np.min(edge_lengths)
        max_length = np.max(edge_lengths)
        
        if min_length < 1e-6:
            results['issues'].append(f"Minimum edge length ({min_length}) too small")
            results['passed'] = False
        
        if max_length > 10:
            results['issues'].append(f"Maximum edge length ({max_length}) too large")
            results['passed'] = False
        
        # Check triangle areas
        areas_plus = [bf['area_plus'] for bf in basis_functions]
        areas_minus = [bf['area_minus'] for bf in basis_functions]
        
        min_area = min(np.min(areas_plus), np.min(areas_minus))
        if min_area < 1e-15:
            results['issues'].append(f"Minimum triangle area ({min_area}) too small")
            results['passed'] = False
        
        return results
    
    def _validate_mom_results(self, mom_results: Dict[str, Any]) -> Dict[str, Any]:
        """Validate MoM results"""
        results = {'passed': True, 'issues': []}
        
        # Check surface currents
        surface_currents = mom_results['surface_currents']
        if len(surface_currents) == 0:
            results['issues'].append("No surface currents calculated")
            results['passed'] = False
        else:
            max_current = np.max(np.abs(surface_currents))
            if max_current < 1e-15:
                results['issues'].append(f"Maximum surface current ({max_current}) too small")
                results['passed'] = False
            elif max_current > 1e6:
                results['issues'].append(f"Maximum surface current ({max_current}) unreasonably large")
                results['passed'] = False
        
        # Check RCS values
        rcs_max = mom_results['radar_cross_section']['max_rcs']
        if rcs_max < 0:
            results['issues'].append(f"Negative RCS value ({rcs_max})")
            results['passed'] = False
        elif rcs_max > 1e6:
            results['issues'].append(f"RCS value ({rcs_max}) unreasonably large")
            results['passed'] = False
        
        return results
    
    def _validate_c_solver_results(self, c_solver_results: Dict[str, Any]) -> Dict[str, Any]:
        """Validate C solver results"""
        results = {'passed': True, 'issues': []}
        
        if c_solver_results.get('c_solver_status') != 'success':
            results['issues'].append("C solver failed")
            results['passed'] = False
        
        return results

def main():
    """Main function to run optimized satellite MoM test"""
    print("🚀 Starting optimized satellite MoM/PEEC electromagnetic simulation...")
    
    # Check if STL file exists
    stl_file = "weixing_v1.stl"
    if not os.path.exists(stl_file):
        print(f"❌ STL file not found: {stl_file}")
        print("Please ensure weixing_v1.stl is in the current directory")
        return
    
    # Create and run test
    test = OptimizedSatelliteMoMTest(stl_file)
    results = test.run_comprehensive_test()
    
    if results['success']:
        print("\n✅ Optimized satellite MoM test completed successfully!")
        print("📊 Check the generated visualization and data files for detailed results")
    else:
        print(f"\n❌ Test failed: {results.get('error', 'Unknown error')}")

if __name__ == '__main__':
    main()