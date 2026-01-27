#!/usr/bin/env python3
"""
Python ctypes interface for MoM/PEEC C Library

Copyright (C) 2025 PulseEM Technologies

Provides Python bindings for calling C solvers via ctypes
This is the primary interface for MoM/PEEC electromagnetic simulations.
"""

import ctypes
import os
import numpy as np
from typing import Optional, Tuple, Dict, Any
import json
import platform

class MoMPEECInterface:
    """
    Python interface to MoM/PEEC C library using ctypes.
    
    This class provides a Python wrapper around the C library functions,
    enabling Python code to call the actual C solvers in src/.
    This is a general-purpose interface for electromagnetic simulations,
    not limited to any specific application.
    """
    
    def __init__(self, library_path: Optional[str] = None):
        """
        Initialize the interface by loading the C library.
        
        Args:
            library_path: Path to the shared library. If None, attempts to find it automatically.
        """
        self.lib = None
        self._load_library(library_path)
        self._setup_function_signatures()
        
    def _load_library(self, library_path: Optional[str] = None):
        """Load the C shared library."""
        if library_path is None:
            # Try to find the library automatically
            possible_paths = self._get_default_library_paths()
            for path in possible_paths:
                if os.path.exists(path):
                    library_path = path
                    break
        
        if library_path is None or not os.path.exists(library_path):
            raise FileNotFoundError(f"Could not find MoM/PEEC library at {library_path}")
        
        try:
            self.lib = ctypes.CDLL(library_path)
            print(f"Successfully loaded C library: {library_path}")
        except OSError as e:
            raise RuntimeError(f"Failed to load C library: {e}")
    
    def _get_default_library_paths(self) -> list:
        """Get list of possible library paths based on platform."""
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        
        if platform.system() == "Windows":
            lib_name = "pulsemom_core.dll"
            possible_paths = [
                os.path.join(base_dir, "build", "Release", lib_name),
                os.path.join(base_dir, "build", "Debug", lib_name),
                os.path.join(base_dir, "lib", lib_name),
                os.path.join(base_dir, "bin", lib_name),
            ]
        elif platform.system() == "Darwin":  # macOS
            lib_name = "libpulsemom_core.dylib"
            possible_paths = [
                os.path.join(base_dir, "build", lib_name),
                os.path.join(base_dir, "lib", lib_name),
                os.path.join("/usr/local/lib", lib_name),
                os.path.join("/usr/lib", lib_name),
            ]
        else:  # Linux
            lib_name = "libpulsemom_core.so"
            possible_paths = [
                os.path.join(base_dir, "build", lib_name),
                os.path.join(base_dir, "lib", lib_name),
                os.path.join("/usr/local/lib", lib_name),
                os.path.join("/usr/lib", lib_name),
            ]
        
        return possible_paths
    
    def _setup_function_signatures(self):
        """Setup ctypes function signatures for all library functions."""
        if not self.lib:
            return
        
        # Basic types
        self.lib.simulation_create.restype = ctypes.c_void_p
        self.lib.simulation_create.argtypes = []
        
        self.lib.simulation_destroy.restype = None
        self.lib.simulation_destroy.argtypes = [ctypes.c_void_p]
        
        self.lib.get_error_string.restype = ctypes.c_char_p
        self.lib.get_error_string.argtypes = [ctypes.c_int]
        
        self.lib.get_version.restype = ctypes.c_char_p
        self.lib.get_version.argtypes = []
        
        # Geometry functions
        self.lib.load_stl.restype = ctypes.c_int
        self.lib.load_stl.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        
        self.lib.set_material.restype = ctypes.c_int
        self.lib.set_material.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        
        self.lib.generate_mesh.restype = ctypes.c_int
        self.lib.generate_mesh.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        
        self.lib.get_mesh_info.restype = ctypes.c_int
        self.lib.get_mesh_info.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
        
        # Solver configuration
        self.lib.configure_solver.restype = ctypes.c_int
        self.lib.configure_solver.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        
        self.lib.set_excitation.restype = ctypes.c_int
        self.lib.set_excitation.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        
        # Simulation execution
        self.lib.run_simulation.restype = ctypes.c_int
        self.lib.run_simulation.argtypes = [ctypes.c_void_p]
        
        # Results retrieval
        self.lib.get_currents.restype = ctypes.c_int
        self.lib.get_currents.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
        
        self.lib.get_fields.restype = ctypes.c_int
        self.lib.get_fields.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
        
        self.lib.get_performance.restype = ctypes.c_int
        self.lib.get_performance.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
        
        # Memory management
        self.lib.free_currents.restype = None
        self.lib.free_currents.argtypes = [ctypes.c_void_p]
        
        self.lib.free_fields.restype = None
        self.lib.free_fields.argtypes = [ctypes.c_void_p]
        
        self.lib.free_performance.restype = None
        self.lib.free_performance.argtypes = [ctypes.c_void_p]
    
    def create_simulation(self) -> int:
        """Create a new simulation instance. Returns handle to simulation."""
        return self.lib.simulation_create()
    
    def destroy_simulation(self, sim_handle: int):
        """Destroy a simulation instance."""
        self.lib.simulation_destroy(sim_handle)
    
    def get_error_string(self, error_code: int) -> str:
        """Get human-readable error string."""
        return self.lib.get_error_string(error_code).decode('utf-8')
    
    def get_version(self) -> str:
        """Get library version string."""
        return self.lib.get_version().decode('utf-8')
    
    def load_stl(self, sim_handle: int, stl_filename: str) -> int:
        """Load STL geometry file."""
        return self.lib.load_stl(sim_handle, stl_filename.encode('utf-8'))
    
    def set_material(self, sim_handle: int, material: Dict[str, Any]) -> int:
        """Set material properties."""
        # Define material structure
        class MaterialStruct(ctypes.Structure):
            _fields_ = [
                ("name", ctypes.c_char * 64),
                ("eps_r", ctypes.c_double),
                ("mu_r", ctypes.c_double),
                ("sigma", ctypes.c_double),
                ("tan_delta", ctypes.c_double),
                ("material_id", ctypes.c_int),
            ]
        
        mat = MaterialStruct()
        mat.name = material.get('name', 'Material').encode('utf-8')
        mat.eps_r = material.get('eps_r', 1.0)
        mat.mu_r = material.get('mu_r', 1.0)
        mat.sigma = material.get('sigma', 0.0)
        mat.tan_delta = material.get('tan_delta', 0.0)
        mat.material_id = material.get('material_id', 1)
        
        return self.lib.set_material(sim_handle, ctypes.byref(mat))
    
    def generate_mesh(self, sim_handle: int, mesh_params: Dict[str, Any]) -> int:
        """Generate mesh with specified parameters."""
        class MeshParamsStruct(ctypes.Structure):
            _fields_ = [
                ("target_edge_length", ctypes.c_double),
                ("max_facets", ctypes.c_int),
                ("min_quality", ctypes.c_double),
                ("adaptive_refinement", ctypes.c_bool),
                ("mesh_algorithm", ctypes.c_char * 32),
            ]
        
        params = MeshParamsStruct()
        params.target_edge_length = mesh_params.get('target_edge_length', 0.03)
        params.max_facets = mesh_params.get('max_facets', 10000)
        params.min_quality = mesh_params.get('min_quality', 0.3)
        params.adaptive_refinement = mesh_params.get('adaptive_refinement', False)
        params.mesh_algorithm = mesh_params.get('mesh_algorithm', 'delaunay').encode('utf-8')
        
        return self.lib.generate_mesh(sim_handle, ctypes.byref(params))
    
    def get_mesh_info(self, sim_handle: int) -> Tuple[int, int]:
        """Get mesh information (num_vertices, num_triangles)."""
        num_vertices = ctypes.c_int()
        num_triangles = ctypes.c_int()
        
        error = self.lib.get_mesh_info(sim_handle, ctypes.byref(num_vertices), ctypes.byref(num_triangles))
        if error != 0:
            raise RuntimeError(f"Failed to get mesh info: {self.get_error_string(error)}")
        
        return num_vertices.value, num_triangles.value
    
    def configure_solver(self, sim_handle: int, solver_config: Dict[str, Any]) -> int:
        """Configure solver parameters."""
        class SolverConfigStruct(ctypes.Structure):
            _fields_ = [
                ("solver_type", ctypes.c_char * 16),
                ("frequency", ctypes.c_double),
                ("basis_order", ctypes.c_int),
                ("formulation", ctypes.c_char * 16),
                ("tolerance", ctypes.c_double),
                ("max_iterations", ctypes.c_int),
                ("use_preconditioner", ctypes.c_bool),
                ("use_fast_solver", ctypes.c_bool),
            ]
        
        config = SolverConfigStruct()
        config.solver_type = solver_config.get('solver_type', 'mom').encode('utf-8')
        config.frequency = solver_config.get('frequency', 10e9)
        config.basis_order = solver_config.get('basis_order', 1)
        config.formulation = solver_config.get('formulation', 'efie').encode('utf-8')
        config.tolerance = solver_config.get('tolerance', 1e-6)
        config.max_iterations = solver_config.get('max_iterations', 1000)
        config.use_preconditioner = solver_config.get('use_preconditioner', True)
        config.use_fast_solver = solver_config.get('use_fast_solver', False)
        
        return self.lib.configure_solver(sim_handle, ctypes.byref(config))
    
    def set_excitation(self, sim_handle: int, excitation: Dict[str, Any]) -> int:
        """Set plane wave excitation."""
        class ExcitationStruct(ctypes.Structure):
            _fields_ = [
                ("frequency", ctypes.c_double),
                ("amplitude", ctypes.c_double),
                ("phase", ctypes.c_double),
                ("theta", ctypes.c_double),
                ("phi", ctypes.c_double),
                ("polarization", ctypes.c_double),
            ]
        
        exc = ExcitationStruct()
        exc.frequency = excitation.get('frequency', 10e9)
        exc.amplitude = excitation.get('amplitude', 1.0)
        exc.phase = excitation.get('phase', 0.0)
        exc.theta = excitation.get('theta', 0.0)
        exc.phi = excitation.get('phi', 0.0)
        exc.polarization = excitation.get('polarization', 0.0)
        
        return self.lib.set_excitation(sim_handle, ctypes.byref(exc))
    
    def run_simulation(self, sim_handle: int) -> int:
        """Run the electromagnetic simulation."""
        return self.lib.run_simulation(sim_handle)
    
    def get_currents(self, sim_handle: int) -> Dict[str, np.ndarray]:
        """Get surface current distribution."""
        currents_ptr = ctypes.c_void_p()
        error = self.lib.get_currents(sim_handle, ctypes.byref(currents_ptr))
        
        if error != 0:
            raise RuntimeError(f"Failed to get currents: {self.get_error_string(error)}")
        
        # Define currents structure
        class CurrentsStruct(ctypes.Structure):
            _fields_ = [
                ("currents", ctypes.POINTER(ctypes.c_double)),  # Complex currents as real/imag pairs
                ("magnitude", ctypes.POINTER(ctypes.c_double)),
                ("phase", ctypes.POINTER(ctypes.c_double)),
                ("num_basis", ctypes.c_int),
            ]
        
        if currents_ptr.value is None:
            raise RuntimeError("No currents data available")
        
        currents = ctypes.cast(currents_ptr, ctypes.POINTER(CurrentsStruct)).contents
        num_basis = currents.num_basis
        
        # Convert to numpy arrays
        magnitude = np.ctypeslib.as_array(currents.magnitude, shape=(num_basis,))
        phase = np.ctypeslib.as_array(currents.phase, shape=(num_basis,))
        
        # Complex currents (stored as real/imag pairs)
        currents_array = np.ctypeslib.as_array(currents.currents, shape=(num_basis * 2,))
        complex_currents = currents_array[::2] + 1j * currents_array[1::2]
        
        # Free the C memory
        self.lib.free_currents(currents_ptr)
        
        return {
            'currents': complex_currents,
            'magnitude': magnitude,
            'phase': phase,
            'num_basis': num_basis
        }
    
    def get_fields(self, sim_handle: int, observation_points: np.ndarray) -> Dict[str, np.ndarray]:
        """Get electromagnetic fields at observation points."""
        if observation_points.ndim != 2 or observation_points.shape[1] != 3:
            raise ValueError("observation_points must be Nx3 array")
        
        num_points = observation_points.shape[0]
        
        # Create observation points structure
        class ObservationPointsStruct(ctypes.Structure):
            _fields_ = [
                ("x", ctypes.POINTER(ctypes.c_double)),
                ("y", ctypes.POINTER(ctypes.c_double)),
                ("z", ctypes.POINTER(ctypes.c_double)),
                ("num_points", ctypes.c_int),
            ]
        
        # Convert numpy arrays to ctypes
        x_coords = observation_points[:, 0].ctypes.data_as(ctypes.POINTER(ctypes.c_double))
        y_coords = observation_points[:, 1].ctypes.data_as(ctypes.POINTER(ctypes.c_double))
        z_coords = observation_points[:, 2].ctypes.data_as(ctypes.POINTER(ctypes.c_double))
        
        points = ObservationPointsStruct()
        points.x = x_coords
        points.y = y_coords
        points.z = z_coords
        points.num_points = num_points
        
        fields_ptr = ctypes.c_void_p()
        error = self.lib.get_fields(sim_handle, ctypes.byref(points), ctypes.byref(fields_ptr))
        
        if error != 0:
            raise RuntimeError(f"Failed to get fields: {self.get_error_string(error)}")
        
        # Define fields structure
        class FieldsStruct(ctypes.Structure):
            _fields_ = [
                ("e_field", ctypes.POINTER(ctypes.c_double)),  # Complex E-field
                ("h_field", ctypes.POINTER(ctypes.c_double)),  # Complex H-field
                ("e_magnitude", ctypes.POINTER(ctypes.c_double)),
                ("h_magnitude", ctypes.POINTER(ctypes.c_double)),
                ("num_points", ctypes.c_int),
            ]
        
        if fields_ptr.value is None:
            raise RuntimeError("No fields data available")
        
        fields = ctypes.cast(fields_ptr, ctypes.POINTER(FieldsStruct)).contents
        
        # Convert to numpy arrays
        e_magnitude = np.ctypeslib.as_array(fields.e_magnitude, shape=(num_points,))
        h_magnitude = np.ctypeslib.as_array(fields.h_magnitude, shape=(num_points,))
        
        # Complex fields (stored as real/imag pairs)
        e_field_array = np.ctypeslib.as_array(fields.e_field, shape=(num_points * 6,))  # 3 components * 2 (real/imag)
        h_field_array = np.ctypeslib.as_array(fields.h_field, shape=(num_points * 6,))
        
        # Reshape complex fields
        e_field = np.zeros((num_points, 3), dtype=complex)
        h_field = np.zeros((num_points, 3), dtype=complex)
        
        for i in range(num_points):
            for j in range(3):
                e_field[i, j] = e_field_array[i*6 + j*2] + 1j * e_field_array[i*6 + j*2 + 1]
                h_field[i, j] = h_field_array[i*6 + j*2] + 1j * h_field_array[i*6 + j*2 + 1]
        
        # Free the C memory
        self.lib.free_fields(fields_ptr)
        
        return {
            'e_field': e_field,
            'h_field': h_field,
            'e_magnitude': e_magnitude,
            'h_magnitude': h_magnitude,
            'num_points': num_points
        }
    
    def get_performance(self, sim_handle: int) -> Dict[str, Any]:
        """Get simulation performance metrics."""
        perf_ptr = ctypes.c_void_p()
        error = self.lib.get_performance(sim_handle, ctypes.byref(perf_ptr))
        
        if error != 0:
            raise RuntimeError(f"Failed to get performance: {self.get_error_string(error)}")
        
        # Define performance structure
        class PerformanceStruct(ctypes.Structure):
            _fields_ = [
                ("mesh_generation_time", ctypes.c_double),
                ("matrix_assembly_time", ctypes.c_double),
                ("solver_time", ctypes.c_double),
                ("total_time", ctypes.c_double),
                ("memory_usage", ctypes.c_size_t),
                ("num_unknowns", ctypes.c_int),
                ("converged", ctypes.c_bool),
            ]
        
        if perf_ptr.value is None:
            raise RuntimeError("No performance data available")
        
        perf = ctypes.cast(perf_ptr, ctypes.POINTER(PerformanceStruct)).contents
        
        result = {
            'mesh_generation_time': perf.mesh_generation_time,
            'matrix_assembly_time': perf.matrix_assembly_time,
            'solver_time': perf.solver_time,
            'total_time': perf.total_time,
            'memory_usage': perf.memory_usage,
            'num_unknowns': perf.num_unknowns,
            'converged': perf.converged,
        }
        
        # Free the C memory
        self.lib.free_performance(perf_ptr)
        
        return result

# Convenience function for running complete simulation
def run_simulation(stl_file: str, frequency: float = 10e9, 
                 solver_type: str = 'mom', **kwargs) -> Dict[str, Any]:
    """
    Run a complete electromagnetic simulation using the C library.
    
    Args:
        stl_file: Path to STL geometry file
        frequency: Analysis frequency in Hz
        solver_type: 'mom' or 'peec'
        **kwargs: Additional parameters
    
    Returns:
        Dictionary containing simulation results
    """
    # Create interface
    interface = MoMPEECInterface()
    
    # Create simulation
    sim_handle = interface.create_simulation()
    
    try:
        # Load geometry
        error = interface.load_stl(sim_handle, stl_file)
        if error != 0:
            raise RuntimeError(f"Failed to load STL: {interface.get_error_string(error)}")
        
        # Set material (default to PEC)
        material = {
            'name': 'PEC',
            'eps_r': 1.0,
            'mu_r': 1.0,
            'sigma': 1e20,
            'tan_delta': 0.0,
            'material_id': 1
        }
        material.update(kwargs.get('material', {}))
        error = interface.set_material(sim_handle, material)
        if error != 0:
            raise RuntimeError(f"Failed to set material: {interface.get_error_string(error)}")
        
        # Generate mesh
        mesh_params = {
            'target_edge_length': kwargs.get('target_edge_length', 0.03),  # 3cm at 10GHz
            'max_facets': kwargs.get('max_facets', 10000),
            'min_quality': kwargs.get('min_quality', 0.3),
            'adaptive_refinement': kwargs.get('adaptive_refinement', False),
            'mesh_algorithm': kwargs.get('mesh_algorithm', 'delaunay')
        }
        error = interface.generate_mesh(sim_handle, mesh_params)
        if error != 0:
            raise RuntimeError(f"Failed to generate mesh: {interface.get_error_string(error)}")
        
        # Get mesh info
        num_vertices, num_triangles = interface.get_mesh_info(sim_handle)
        
        # Configure solver
        solver_config = {
            'solver_type': solver_type,
            'frequency': frequency,
            'basis_order': kwargs.get('basis_order', 1),
            'formulation': kwargs.get('formulation', 'efie'),
            'tolerance': kwargs.get('tolerance', 1e-6),
            'max_iterations': kwargs.get('max_iterations', 1000),
            'use_preconditioner': kwargs.get('use_preconditioner', True),
            'use_fast_solver': kwargs.get('use_fast_solver', False)
        }
        error = interface.configure_solver(sim_handle, solver_config)
        if error != 0:
            raise RuntimeError(f"Failed to configure solver: {interface.get_error_string(error)}")
        
        # Set excitation
        excitation = {
            'frequency': frequency,
            'amplitude': kwargs.get('amplitude', 1.0),
            'phase': kwargs.get('phase', 0.0),
            'theta': kwargs.get('theta', 0.0),
            'phi': kwargs.get('phi', 0.0),
            'polarization': kwargs.get('polarization', 0.0)
        }
        error = interface.set_excitation(sim_handle, excitation)
        if error != 0:
            raise RuntimeError(f"Failed to set excitation: {interface.get_error_string(error)}")
        
        # Run simulation
        error = interface.run_simulation(sim_handle)
        if error != 0:
            raise RuntimeError(f"Simulation failed: {interface.get_error_string(error)}")
        
        # Get results
        currents = interface.get_currents(sim_handle)
        performance = interface.get_performance(sim_handle)
        
        # Create observation points (default to a grid around the object)
        if 'observation_points' in kwargs:
            obs_points = kwargs['observation_points']
        else:
            # Default grid around origin
            x = np.linspace(-2, 2, 21)
            y = np.linspace(-2, 2, 21)
            z = np.linspace(-2, 2, 21)
            X, Y, Z = np.meshgrid(x, y, z, indexing='ij')
            obs_points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
        fields = interface.get_fields(sim_handle, obs_points)
        
        return {
            'currents': currents,
            'fields': fields,
            'performance': performance,
            'mesh_info': {
                'num_vertices': num_vertices,
                'num_triangles': num_triangles
            },
            'configuration': {
                'stl_file': stl_file,
                'frequency': frequency,
                'solver_type': solver_type,
                'material': material,
                'mesh_params': mesh_params,
                'solver_config': solver_config,
                'excitation': excitation
            }
        }
        
    finally:
        # Clean up
        interface.destroy_simulation(sim_handle)

# Backward compatibility aliases
SatelliteMoMPEECInterface = MoMPEECInterface
run_satellite_c_simulation = run_simulation

# Example usage and testing
if __name__ == "__main__":
    print("Testing MoM/PEEC C Library Interface...")
    
    # Create interface
    try:
        interface = MoMPEECInterface()
        print(f"Library version: {interface.get_version()}")
        
        # Test basic functionality
        sim = interface.create_simulation()
        print(f"Created simulation handle: {sim}")
        
        # Clean up
        interface.destroy_simulation(sim)
        print("Interface test completed successfully!")
        
    except Exception as e:
        print(f"Interface test failed: {e}")
        print("Make sure the C library is built and available.")
