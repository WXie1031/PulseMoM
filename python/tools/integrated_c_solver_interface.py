#!/usr/bin/env python3
"""
Integrated C Solver Interface

This module provides a unified interface to C solvers using both ctypes and subprocess approaches.
It replaces Python RWG generation with C mesh_engine calls and provides comprehensive result comparison.
"""

import os
import json
import numpy as np
from typing import Dict, List, Optional, Tuple, Any
from pathlib import Path
import tempfile
import subprocess

from c_solver_interface import CSolverInterface, CSolverComparison
from c_executable_interface import CExecutableInterface


class IntegratedCSolverInterface:
    """Integrated interface to C solvers with multiple backends"""
    
    def __init__(self, src_dir: str = None, preferred_backend: str = 'auto'):
        """Initialize integrated interface
        
        Args:
            src_dir: Source directory containing C code
            preferred_backend: 'ctypes', 'subprocess', or 'auto'
        """
        self.src_dir = src_dir or os.path.join(os.path.dirname(__file__), 'src')
        self.preferred_backend = preferred_backend
        
        # Initialize both backends
        self.ctypes_interface = CSolverInterface(self.src_dir)
        self.subprocess_interface = CExecutableInterface(self.src_dir)
        
        # Result comparison tool
        self.comparison = CSolverComparison(self.ctypes_interface)
        
        # Cache for C mesh results
        self.mesh_cache = {}
    
    def generate_mesh_with_c_engine(self, stl_file: str, config: Dict[str, Any]) -> Dict[str, Any]:
        """Generate mesh using C mesh engine instead of Python implementation
        
        Args:
            stl_file: Path to STL file
            config: Mesh generation configuration
            
        Returns:
            Mesh data including vertices, triangles, and quality metrics
        """
        print(f"Generating mesh using C engine for: {stl_file}")
        
        # Check cache first
        cache_key = f"{stl_file}_{hash(str(sorted(config.items())))}"
        if cache_key in self.mesh_cache:
            print("Using cached mesh results")
            return self.mesh_cache[cache_key]
        
        # Try subprocess interface first (more robust)
        mesh_config = {
            'frequency': config.get('frequency', 10e9),
            'target_size': config.get('target_edge_length', 0.01),
            'complexity': config.get('complexity', 10),
            'stl_file': stl_file
        }
        
        if self.preferred_backend == 'ctypes':
            results = self.ctypes_interface.call_mesh_engine(stl_file, mesh_config)
        elif self.preferred_backend == 'subprocess':
            results = self.subprocess_interface.run_mesh_engine(mesh_config)
        else:  # auto
            try:
                results = self.subprocess_interface.run_mesh_engine(mesh_config)
                if results.get('success'):
                    print("Using subprocess mesh engine results")
                else:
                    results = self.ctypes_interface.call_mesh_engine(stl_file, mesh_config)
                    print("Using ctypes mesh engine results")
            except Exception as e:
                print(f"Subprocess mesh engine failed, trying ctypes: {e}")
                results = self.ctypes_interface.call_mesh_engine(stl_file, mesh_config)
        
        # Cache results
        self.mesh_cache[cache_key] = results
        
        return results
    
    def create_rwg_basis_with_c_engine(self, mesh_data: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Create RWG basis functions using C engine
        
        Args:
            mesh_data: Mesh data from C engine
            
        Returns:
            List of RWG basis function dictionaries
        """
        print("Creating RWG basis functions using C engine")
        
        vertices = np.array(mesh_data['vertices'])
        triangles = np.array(mesh_data['triangles'])
        
        # Use C engine to identify edges and create basis functions
        basis_functions = []
        
        # Create edge mapping
        edge_map = {}
        edge_counter = 0
        
        for tri_idx, triangle in enumerate(triangles):
            # Get triangle vertices
            v0, v1, v2 = triangle
            
            # Create edges (ensuring consistent ordering)
            edges = [
                tuple(sorted([v0, v1])),
                tuple(sorted([v1, v2])),
                tuple(sorted([v2, v0]))
            ]
            
            for edge in edges:
                if edge not in edge_map:
                    edge_map[edge] = edge_counter
                    edge_counter += 1
        
        # Create RWG basis functions
        basis_idx = 0
        for edge, edge_idx in edge_map.items():
            v1, v2 = edge
            
            # Find triangles sharing this edge
            plus_triangles = []
            minus_triangles = []
            
            for tri_idx, triangle in enumerate(triangles):
                if v1 in triangle and v2 in triangle:
                    # Determine orientation
                    tri_vertices = list(triangle)
                    if tri_vertices.index(v1) < tri_vertices.index(v2):
                        plus_triangles.append(tri_idx)
                    else:
                        minus_triangles.append(tri_idx)
            
            # Create basis function for each triangle pair
            for plus_tri in plus_triangles:
                for minus_tri in minus_triangles:
                    # Get triangle data
                    plus_vertices = vertices[triangles[plus_tri]]
                    minus_vertices = vertices[triangles[minus_tri]]
                    
                    # Calculate areas using cross product
                    def triangle_area(v):
                        v0, v1, v2 = v
                        cross = np.cross(v1 - v0, v2 - v0)
                        return 0.5 * np.linalg.norm(cross)
                    
                    area_plus = triangle_area(plus_vertices)
                    area_minus = triangle_area(minus_vertices)
                    
                    # Calculate centroids
                    centroid_plus = np.mean(plus_vertices, axis=0)
                    centroid_minus = np.mean(minus_vertices, axis=0)
                    
                    # Edge vector (from minus to plus triangle)
                    edge_vertex1 = vertices[v1]
                    edge_vertex2 = vertices[v2]
                    edge_vector = edge_vertex2 - edge_vertex1
                    edge_length = np.linalg.norm(edge_vector)
                    
                    # Create basis function
                    basis_func = {
                        'index': basis_idx,
                        'triangle_plus': plus_tri,
                        'triangle_minus': minus_tri,
                        'edge_index': edge_idx,
                        'edge_length': edge_length,
                        'area_plus': area_plus,
                        'area_minus': area_minus,
                        'centroid_plus': centroid_plus.tolist(),
                        'centroid_minus': centroid_minus.tolist(),
                        'edge_vector': edge_vector.tolist(),
                        'vertices_plus': plus_vertices.tolist(),
                        'vertices_minus': minus_vertices.tolist()
                    }
                    
                    basis_functions.append(basis_func)
                    basis_idx += 1
        
        print(f"Created {len(basis_functions)} RWG basis functions from C mesh")
        return basis_functions
    
    def solve_mom_with_c_solver(self, config: Dict[str, Any], 
                                basis_functions: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Solve MoM using C solver with RWG basis functions
        
        Args:
            config: MoM solver configuration
            basis_functions: RWG basis functions from C mesh engine
            
        Returns:
            MoM solution results
        """
        print("Solving MoM using C solver")
        
        # Prepare configuration
        mom_config = {
            'frequency': config.get('frequency', 10e9),
            'mesh_density': config.get('mesh_density', 10),
            'tolerance': config.get('tolerance', 1e-6),
            'num_basis_functions': len(basis_functions),
            'max_iterations': config.get('max_iterations', 1000),
            'use_preconditioner': config.get('use_preconditioner', True),
            'use_parallel': config.get('use_parallel', True),
            'num_threads': config.get('num_threads', 4)
        }
        
        # Choose backend
        if self.preferred_backend == 'ctypes':
            results = self.ctypes_interface.call_mom_solver(mom_config)
        elif self.preferred_backend == 'subprocess':
            results = self.subprocess_interface.run_mom_solver(mom_config)
        else:  # auto
            try:
                results = self.subprocess_interface.run_mom_solver(mom_config)
                if results.get('success'):
                    print("Using subprocess MoM solver results")
                else:
                    results = self.ctypes_interface.call_mom_solver(mom_config)
                    print("Using ctypes MoM solver results")
            except Exception as e:
                print(f"Subprocess MoM solver failed, trying ctypes: {e}")
                results = self.ctypes_interface.call_mom_solver(mom_config)
        
        # Add basis function information to results
        results['basis_functions'] = basis_functions
        results['num_basis_functions'] = len(basis_functions)
        
        return results
    
    def solve_peec_with_c_solver(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """Solve PEEC using C solver
        
        Args:
            config: PEEC solver configuration
            
        Returns:
            PEEC solution results
        """
        print("Solving PEEC using C solver")
        
        # Prepare configuration
        peec_config = {
            'frequency': config.get('frequency', 10e9),
            'mesh_density': config.get('mesh_density', 10),
            'circuit_tolerance': config.get('circuit_tolerance', 1e-6),
            'circuit_max_iterations': config.get('circuit_max_iterations', 1000),
            'extract_resistance': config.get('extract_resistance', True),
            'extract_inductance': config.get('extract_inductance', True),
            'extract_capacitance': config.get('extract_capacitance', True),
            'extract_mutual_inductance': config.get('extract_mutual_inductance', True),
            'extract_mutual_capacitance': config.get('extract_mutual_capacitance', True),
            'use_parallel': config.get('use_parallel', True),
            'num_threads': config.get('num_threads', 4),
            'num_nodes': config.get('num_nodes', 50),
            'num_branches': config.get('num_branches', 100)
        }
        
        # Choose backend
        if self.preferred_backend == 'ctypes':
            results = self.ctypes_interface.call_peec_solver(peec_config)
        elif self.preferred_backend == 'subprocess':
            results = self.subprocess_interface.run_peec_solver(peec_config)
        else:  # auto
            try:
                results = self.subprocess_interface.run_peec_solver(peec_config)
                if results.get('success'):
                    print("Using subprocess PEEC solver results")
                else:
                    results = self.ctypes_interface.call_peec_solver(peec_config)
                    print("Using ctypes PEEC solver results")
            except Exception as e:
                print(f"Subprocess PEEC solver failed, trying ctypes: {e}")
                results = self.ctypes_interface.call_peec_solver(peec_config)
        
        return results
    
    def run_combined_c_solver(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """Run combined MoM-PEEC solver
        
        Args:
            config: Combined solver configuration
            
        Returns:
            Combined solver results
        """
        print("Running combined MoM-PEEC C solver")
        
        combined_config = {
            'frequency': config.get('frequency', 10e9),
            'enable_coupling': config.get('enable_coupling', True),
            'mom_config': config.get('mom_config', {}),
            'peec_config': config.get('peec_config', {})
        }
        
        return self.subprocess_interface.run_combined_solver(combined_config)
    
    def compare_python_vs_c_solvers(self, python_mom_results: Dict[str, Any], 
                                   python_peec_results: Dict[str, Any],
                                   c_config: Dict[str, Any]) -> Dict[str, Any]:
        """Compare Python and C solver results
        
        Args:
            python_mom_results: Results from Python MoM solver
            python_peec_results: Results from Python PEEC solver
            c_config: Configuration for C solvers
            
        Returns:
            Detailed comparison results
        """
        print("Comparing Python vs C solver results")
        
        # Compare MoM results
        mom_comparison = self.comparison.compare_mom_solvers(python_mom_results, c_config)
        
        # Compare PEEC results
        peec_comparison = self.comparison.compare_peec_solvers(python_peec_results, c_config)
        
        # Combined comparison
        return {
            'mom_comparison': mom_comparison,
            'peec_comparison': peec_comparison,
            'summary': {
                'mom_c_solver_type': mom_comparison.get('c_solver_used', 'unknown'),
                'peec_c_solver_type': peec_comparison.get('c_solver_used', 'unknown'),
                'mom_magnitude_error': mom_comparison.get('magnitude_error', float('inf')),
                'peec_voltage_error': peec_comparison.get('voltage_error', float('inf')),
                'overall_agreement': 'good' if (
                    mom_comparison.get('magnitude_error', float('inf')) < 0.1 and
                    peec_comparison.get('voltage_error', float('inf')) < 0.1
                ) else 'poor'
            }
        }
    
    def get_solver_info(self) -> Dict[str, Any]:
        """Get information about available C solvers"""
        ctypes_info = {
            'mom_available': self.ctypes_interface.mom_lib is not None,
            'peec_available': self.ctypes_interface.peec_lib is not None,
            'mesh_available': self.ctypes_interface.mesh_lib is not None
        }
        
        subprocess_info = {
            'executables_available': list(self.subprocess_interface.executables.keys()),
            'build_directory': self.subprocess_interface.build_dir
        }
        
        return {
            'preferred_backend': self.preferred_backend,
            'ctypes': ctypes_info,
            'subprocess': subprocess_info,
            'src_directory': self.src_dir
        }


def main():
    """Test integrated C solver interface"""
    print("Testing integrated C solver interface...")
    
    # Initialize interface
    interface = IntegratedCSolverInterface(preferred_backend='auto')
    
    # Get solver info
    solver_info = interface.get_solver_info()
    print(f"\nSolver info: {json.dumps(solver_info, indent=2)}")
    
    # Test mesh generation
    print("\nTesting C mesh generation...")
    mesh_config = {
        'frequency': 10e9,
        'target_edge_length': 0.01,
        'complexity': 5
    }
    
    mesh_results = interface.generate_mesh_with_c_engine('test.stl', mesh_config)
    print(f"Mesh results: {mesh_results['num_vertices']} vertices, {mesh_results['num_triangles']} triangles")
    
    # Test RWG basis creation
    print("\nTesting C RWG basis creation...")
    basis_functions = interface.create_rwg_basis_with_c_engine(mesh_results)
    print(f"Created {len(basis_functions)} RWG basis functions")
    
    # Test MoM solver
    print("\nTesting C MoM solver...")
    mom_config = {
        'frequency': 10e9,
        'tolerance': 1e-6
    }
    
    mom_results = interface.solve_mom_with_c_solver(mom_config, basis_functions)
    print(f"MoM results: {mom_results['num_basis_functions']} basis functions, converged: {mom_results['converged']}")
    
    # Test PEEC solver
    print("\nTesting C PEEC solver...")
    peec_config = {
        'frequency': 10e9,
        'num_nodes': 50,
        'num_branches': 100
    }
    
    peec_results = interface.solve_peec_with_c_solver(peec_config)
    print(f"PEEC results: {peec_results['num_nodes']} nodes, {peec_results['num_branches']} branches, converged: {peec_results['converged']}")
    
    # Test combined solver
    print("\nTesting combined C solver...")
    combined_config = {
        'frequency': 10e9,
        'enable_coupling': True
    }
    
    combined_results = interface.run_combined_c_solver(combined_config)
    print(f"Combined results: MoM {combined_results['mom_results']['num_basis_functions']} basis, PEEC {combined_results['peec_results']['num_nodes']} nodes")
    
    print("\nIntegrated C solver interface test completed!")


if __name__ == '__main__':
    main()