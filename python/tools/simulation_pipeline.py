#!/usr/bin/env python3
"""
Unified Simulation Pipeline for PulseMoM

This module provides a unified interface for running electromagnetic simulations
using either:
1. Direct C library calls via ctypes (recommended)
2. C executable calls via subprocess (fallback)

The pipeline follows the workflow defined in the architecture:
- Geometry Import
- Mesh Generation
- Basis Function Setup
- Matrix Assembly
- Solver Execution
- Post-Processing
"""

import os
import sys
import json
import tempfile
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Any, Union
from dataclasses import dataclass, asdict
import numpy as np
from datetime import datetime

# Add project root to path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

try:
    from python_interface.mom_peec_ctypes import MoMPEECInterface
    CTYPES_AVAILABLE = True
except ImportError:
    CTYPES_AVAILABLE = False
    print("Warning: ctypes interface not available, will use executable interface")

try:
    from python.tools.c_executable_interface import CExecutableInterface
    EXECUTABLE_INTERFACE_AVAILABLE = True
except ImportError:
    EXECUTABLE_INTERFACE_AVAILABLE = False


@dataclass
class SimulationConfig:
    """Simulation configuration"""
    # Geometry
    geometry_file: str
    geometry_format: str = "stl"  # stl, step, iges
    
    # Solver settings
    solver_type: str = "mom"  # mom, peec, hybrid
    frequency: float = 1e9  # Hz
    frequencies: Optional[List[float]] = None  # For frequency sweep
    
    # Mesh settings
    mesh_density: float = 10.0  # Elements per wavelength
    min_element_size: Optional[float] = None
    max_element_size: Optional[float] = None
    
    # Material settings
    materials: Optional[Dict[str, Dict[str, float]]] = None
    
    # Excitation settings
    excitation_type: str = "plane_wave"  # plane_wave, voltage_source, current_source
    excitation_params: Optional[Dict[str, Any]] = None
    
    # Solver parameters
    tolerance: float = 1e-6
    max_iterations: int = 1000
    use_preconditioner: bool = True
    
    # Output settings
    output_dir: Optional[str] = None
    save_fields: bool = True
    save_currents: bool = True
    save_mesh: bool = False
    
    # Advanced options
    use_parallel: bool = True
    num_threads: Optional[int] = None
    use_gpu: bool = False


@dataclass
class SimulationResult:
    """Simulation results"""
    config: SimulationConfig
    success: bool
    execution_time: float
    
    # Results
    frequencies: Optional[np.ndarray] = None
    currents: Optional[np.ndarray] = None
    fields: Optional[np.ndarray] = None
    scattering_parameters: Optional[Dict] = None
    impedance_matrix: Optional[np.ndarray] = None
    
    # Mesh information
    num_vertices: int = 0
    num_elements: int = 0
    num_basis_functions: int = 0
    
    # Performance metrics
    memory_usage_mb: float = 0.0
    matrix_fill_time: float = 0.0
    solve_time: float = 0.0
    
    # Error information
    error_message: Optional[str] = None
    convergence_info: Optional[Dict] = None
    
    # Output files
    output_files: List[str] = None
    
    def __post_init__(self):
        if self.output_files is None:
            self.output_files = []


class SimulationPipeline:
    """
    Unified simulation pipeline that supports both ctypes and executable interfaces
    """
    
    def __init__(self, use_ctypes: bool = True, executable_path: Optional[str] = None):
        """
        Initialize simulation pipeline
        
        Args:
            use_ctypes: If True, use ctypes interface (recommended). If False, use executable interface.
            executable_path: Path to executable (if using executable interface)
        """
        self.use_ctypes = use_ctypes and CTYPES_AVAILABLE
        self.ctypes_interface = None
        self.executable_interface = None
        
        if self.use_ctypes:
            try:
                self.ctypes_interface = MoMPEECInterface()
                print("Using ctypes interface (direct library calls)")
            except Exception as e:
                print(f"Failed to load ctypes interface: {e}")
                print("Falling back to executable interface")
                self.use_ctypes = False
        
        if not self.use_ctypes:
            if EXECUTABLE_INTERFACE_AVAILABLE:
                try:
                    self.executable_interface = CExecutableInterface()
                    print("Using executable interface (subprocess calls)")
                except Exception as e:
                    print(f"Failed to initialize executable interface: {e}")
            else:
                raise RuntimeError("Neither ctypes nor executable interface is available")
    
    def run_simulation(self, config: SimulationConfig) -> SimulationResult:
        """
        Run complete simulation pipeline
        
        Pipeline steps:
        1. Load geometry
        2. Generate mesh
        3. Setup basis functions
        4. Assemble matrices
        5. Solve
        6. Post-process
        """
        start_time = datetime.now()
        result = SimulationResult(config=config, success=False, execution_time=0.0)
        
        try:
            if self.use_ctypes:
                result = self._run_simulation_ctypes(config)
            else:
                result = self._run_simulation_executable(config)
            
            result.execution_time = (datetime.now() - start_time).total_seconds()
            result.success = True
            
        except Exception as e:
            result.error_message = str(e)
            result.execution_time = (datetime.now() - start_time).total_seconds()
            print(f"Simulation failed: {e}")
        
        return result
    
    def _run_simulation_ctypes(self, config: SimulationConfig) -> SimulationResult:
        """Run simulation using ctypes interface"""
        interface = self.ctypes_interface
        
        # Create simulation
        sim_handle = interface.create_simulation()
        if not sim_handle:
            raise RuntimeError("Failed to create simulation")
        
        try:
            # Step 1: Load geometry
            print(f"Loading geometry: {config.geometry_file}")
            status = interface.load_stl(sim_handle, config.geometry_file)
            if status != 0:
                raise RuntimeError(f"Failed to load geometry: {interface.get_error_string(status)}")
            
            # Step 2: Set materials
            if config.materials:
                for mat_name, mat_props in config.materials.items():
                    status = interface.set_material(
                        sim_handle, mat_name,
                        mat_props.get('epsr', 1.0),
                        mat_props.get('mur', 1.0),
                        mat_props.get('sigma', 0.0)
                    )
                    if status != 0:
                        print(f"Warning: Failed to set material {mat_name}")
            
            # Step 3: Generate mesh
            print("Generating mesh...")
            mesh_config = {
                'density': config.mesh_density,
                'min_size': config.min_element_size,
                'max_size': config.max_element_size
            }
            status = interface.generate_mesh(sim_handle, mesh_config)
            if status != 0:
                raise RuntimeError(f"Failed to generate mesh: {interface.get_error_string(status)}")
            
            # Step 4: Configure solver
            solver_config = {
                'solver_type': config.solver_type,
                'frequency': config.frequency,
                'tolerance': config.tolerance,
                'max_iterations': config.max_iterations,
                'use_preconditioner': config.use_preconditioner,
                'use_parallel': config.use_parallel,
                'num_threads': config.num_threads
            }
            status = interface.configure_solver(sim_handle, solver_config)
            if status != 0:
                raise RuntimeError(f"Failed to configure solver: {interface.get_error_string(status)}")
            
            # Step 5: Set excitation
            if config.excitation_params:
                status = interface.set_excitation(sim_handle, config.excitation_type, config.excitation_params)
                if status != 0:
                    print(f"Warning: Failed to set excitation: {interface.get_error_string(status)}")
            
            # Step 6: Run simulation
            print("Running simulation...")
            status = interface.run_simulation(sim_handle)
            if status != 0:
                raise RuntimeError(f"Simulation failed: {interface.get_error_string(status)}")
            
            # Step 7: Get results
            result = SimulationResult(
                config=config,
                success=True,
                execution_time=0.0
            )
            
            # Get currents
            if config.save_currents:
                currents = interface.get_currents(sim_handle)
                if currents is not None:
                    result.currents = np.array(currents)
            
            # Get fields
            if config.save_fields:
                fields = interface.get_fields(sim_handle)
                if fields is not None:
                    result.fields = np.array(fields)
            
            # Get performance metrics
            perf = interface.get_performance(sim_handle)
            if perf:
                result.memory_usage_mb = perf.get('memory_usage_mb', 0.0)
                result.matrix_fill_time = perf.get('matrix_fill_time', 0.0)
                result.solve_time = perf.get('solve_time', 0.0)
                result.num_vertices = perf.get('num_vertices', 0)
                result.num_elements = perf.get('num_elements', 0)
                result.num_basis_functions = perf.get('num_basis_functions', 0)
            
            # Step 8: Save results
            if config.output_dir:
                self._save_results(result, config.output_dir)
            
            return result
            
        finally:
            # Cleanup
            interface.destroy_simulation(sim_handle)
    
    def _run_simulation_executable(self, config: SimulationConfig) -> SimulationResult:
        """Run simulation using executable interface"""
        interface = self.executable_interface
        
        # Prepare configuration for executable
        exec_config = {
            'geometry_file': config.geometry_file,
            'geometry_format': config.geometry_format,
            'solver_type': config.solver_type,
            'frequency': config.frequency,
            'mesh_density': config.mesh_density,
            'materials': config.materials or {},
            'excitation': {
                'type': config.excitation_type,
                'params': config.excitation_params or {}
            },
            'solver': {
                'tolerance': config.tolerance,
                'max_iterations': config.max_iterations,
                'use_preconditioner': config.use_preconditioner
            },
            'output_dir': config.output_dir or str(Path.cwd() / "simulation_output")
        }
        
        # Run appropriate solver
        if config.solver_type == "mom":
            results = interface.run_mom_solver(exec_config)
        elif config.solver_type == "peec":
            results = interface.run_peec_solver(exec_config)
        elif config.solver_type == "hybrid":
            results = interface.run_combined_solver(exec_config)
        else:
            raise ValueError(f"Unknown solver type: {config.solver_type}")
        
        # Convert results to SimulationResult
        result = SimulationResult(
            config=config,
            success=results.get('success', False),
            execution_time=results.get('execution_time', 0.0),
            frequencies=np.array([config.frequency]) if results.get('frequencies') is None else np.array(results['frequencies']),
            currents=np.array(results['currents']) if 'currents' in results else None,
            fields=np.array(results['fields']) if 'fields' in results else None,
            num_vertices=results.get('num_vertices', 0),
            num_elements=results.get('num_elements', 0),
            num_basis_functions=results.get('num_basis_functions', 0),
            memory_usage_mb=results.get('memory_usage_mb', 0.0),
            matrix_fill_time=results.get('matrix_fill_time', 0.0),
            solve_time=results.get('solve_time', 0.0),
            error_message=results.get('error_message')
        )
        
        # Save results
        if config.output_dir:
            self._save_results(result, config.output_dir)
        
        return result
    
    def _save_results(self, result: SimulationResult, output_dir: str):
        """Save simulation results to files"""
        os.makedirs(output_dir, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Save configuration
        config_file = os.path.join(output_dir, f"config_{timestamp}.json")
        with open(config_file, 'w') as f:
            config_dict = asdict(result.config)
            # Convert numpy arrays to lists for JSON
            if config_dict.get('frequencies'):
                config_dict['frequencies'] = [float(f) for f in config_dict['frequencies']]
            json.dump(config_dict, f, indent=2)
        result.output_files.append(config_file)
        
        # Save results summary
        summary_file = os.path.join(output_dir, f"summary_{timestamp}.json")
        summary = {
            'success': result.success,
            'execution_time': result.execution_time,
            'num_vertices': result.num_vertices,
            'num_elements': result.num_elements,
            'num_basis_functions': result.num_basis_functions,
            'memory_usage_mb': result.memory_usage_mb,
            'matrix_fill_time': result.matrix_fill_time,
            'solve_time': result.solve_time,
            'error_message': result.error_message
        }
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)
        result.output_files.append(summary_file)
        
        # Save currents if available
        if result.currents is not None:
            currents_file = os.path.join(output_dir, f"currents_{timestamp}.npy")
            np.save(currents_file, result.currents)
            result.output_files.append(currents_file)
        
        # Save fields if available
        if result.fields is not None:
            fields_file = os.path.join(output_dir, f"fields_{timestamp}.npy")
            np.save(fields_file, result.fields)
            result.output_files.append(fields_file)
        
        print(f"Results saved to: {output_dir}")


def run_simulation_from_config(config_file: str, output_dir: Optional[str] = None) -> SimulationResult:
    """
    Convenience function to run simulation from JSON configuration file
    
    Args:
        config_file: Path to JSON configuration file
        output_dir: Output directory (if None, uses config or default)
    
    Returns:
        SimulationResult object
    """
    with open(config_file, 'r') as f:
        config_dict = json.load(f)
    
    config = SimulationConfig(**config_dict)
    if output_dir:
        config.output_dir = output_dir
    
    pipeline = SimulationPipeline()
    return pipeline.run_simulation(config)


if __name__ == "__main__":
    # Example usage
    import argparse
    
    parser = argparse.ArgumentParser(description="Run electromagnetic simulation")
    parser.add_argument("--config", type=str, required=True, help="Configuration JSON file")
    parser.add_argument("--output", type=str, help="Output directory")
    parser.add_argument("--use-executable", action="store_true", help="Use executable interface instead of ctypes")
    
    args = parser.parse_args()
    
    pipeline = SimulationPipeline(use_ctypes=not args.use_executable)
    result = run_simulation_from_config(args.config, args.output)
    
    if result.success:
        print(f"\nSimulation completed successfully!")
        print(f"Execution time: {result.execution_time:.2f} seconds")
        print(f"Vertices: {result.num_vertices}, Elements: {result.num_elements}")
        print(f"Basis functions: {result.num_basis_functions}")
        print(f"Results saved to: {result.config.output_dir}")
    else:
        print(f"\nSimulation failed: {result.error_message}")
        sys.exit(1)
