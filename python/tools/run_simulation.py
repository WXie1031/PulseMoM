#!/usr/bin/env python3
"""
Simple Simulation Script

Easy-to-use script for running electromagnetic simulations.
Supports both command-line arguments and configuration files.
"""

import sys
import os
import json
import argparse
from pathlib import Path

# Add project root to path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

from python.tools.simulation_pipeline import (
    SimulationPipeline,
    SimulationConfig,
    run_simulation_from_config
)


def create_example_config(output_file: str = "example_simulation_config.json"):
    """Create an example simulation configuration file"""
    config = {
        "geometry_file": "tests/patch_antenna_2x2.stl",
        "geometry_format": "stl",
        "solver_type": "mom",
        "frequency": 10e9,
        "mesh_density": 10.0,
        "materials": {
            "PEC": {
                "epsr": 1.0,
                "mur": 1.0,
                "sigma": 1e8
            }
        },
        "excitation_type": "plane_wave",
        "excitation_params": {
            "theta": 0.0,
            "phi": 0.0,
            "polarization": "TE",
            "amplitude": 1.0
        },
        "tolerance": 1e-6,
        "max_iterations": 1000,
        "use_preconditioner": True,
        "output_dir": "simulation_output",
        "save_fields": True,
        "save_currents": True
    }
    
    with open(output_file, 'w') as f:
        json.dump(config, f, indent=2)
    
    print(f"Example configuration saved to: {output_file}")
    return output_file


def main():
    parser = argparse.ArgumentParser(
        description="Run electromagnetic simulation using PulseMoM",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run simulation from config file
  python run_simulation.py --config my_config.json
  
  # Run with command-line arguments
  python run_simulation.py --geometry geometry.stl --frequency 10e9 --solver mom
  
  # Create example config file
  python run_simulation.py --create-config
  
  # Use executable interface instead of ctypes
  python run_simulation.py --config my_config.json --use-executable
        """
    )
    
    # Configuration file option
    parser.add_argument("--config", type=str, help="JSON configuration file")
    
    # Geometry options
    parser.add_argument("--geometry", type=str, help="Geometry file (STL, STEP, etc.)")
    parser.add_argument("--geometry-format", type=str, default="stl", choices=["stl", "step", "iges"],
                       help="Geometry file format")
    
    # Solver options
    parser.add_argument("--solver", type=str, default="mom", choices=["mom", "peec", "hybrid"],
                       help="Solver type")
    parser.add_argument("--frequency", type=float, help="Frequency in Hz")
    parser.add_argument("--frequencies", type=str, help="Comma-separated frequencies for sweep (e.g., '1e9,2e9,3e9')")
    
    # Mesh options
    parser.add_argument("--mesh-density", type=float, default=10.0,
                       help="Mesh density (elements per wavelength)")
    parser.add_argument("--min-element-size", type=float, help="Minimum element size")
    parser.add_argument("--max-element-size", type=float, help="Maximum element size")
    
    # Excitation options
    parser.add_argument("--excitation", type=str, default="plane_wave",
                       choices=["plane_wave", "voltage_source", "current_source"],
                       help="Excitation type")
    parser.add_argument("--excitation-theta", type=float, default=0.0, help="Plane wave theta angle (degrees)")
    parser.add_argument("--excitation-phi", type=float, default=0.0, help="Plane wave phi angle (degrees)")
    
    # Solver parameters
    parser.add_argument("--tolerance", type=float, default=1e-6, help="Solver tolerance")
    parser.add_argument("--max-iterations", type=int, default=1000, help="Maximum iterations")
    parser.add_argument("--no-preconditioner", action="store_true", help="Disable preconditioner")
    
    # Output options
    parser.add_argument("--output", type=str, help="Output directory")
    parser.add_argument("--no-fields", action="store_true", help="Don't save field data")
    parser.add_argument("--no-currents", action="store_true", help="Don't save current data")
    
    # Interface options
    parser.add_argument("--use-executable", action="store_true",
                       help="Use executable interface instead of ctypes")
    
    # Utility options
    parser.add_argument("--create-config", action="store_true",
                       help="Create example configuration file and exit")
    
    args = parser.parse_args()
    
    # Create example config if requested
    if args.create_config:
        create_example_config()
        return
    
    # Build configuration
    if args.config:
        # Load from file
        config = run_simulation_from_config(args.config, args.output)
    else:
        # Build from command-line arguments
        if not args.geometry:
            parser.error("--geometry is required when not using --config")
        if not args.frequency and not args.frequencies:
            parser.error("Either --frequency or --frequencies is required")
        
        frequencies = None
        if args.frequencies:
            frequencies = [float(f.strip()) for f in args.frequencies.split(',')]
        
        excitation_params = {}
        if args.excitation == "plane_wave":
            excitation_params = {
                "theta": args.excitation_theta,
                "phi": args.excitation_phi,
                "polarization": "TE",
                "amplitude": 1.0
            }
        
        config_dict = {
            "geometry_file": args.geometry,
            "geometry_format": args.geometry_format,
            "solver_type": args.solver,
            "frequency": args.frequency or frequencies[0] if frequencies else 1e9,
            "frequencies": frequencies,
            "mesh_density": args.mesh_density,
            "min_element_size": args.min_element_size,
            "max_element_size": args.max_element_size,
            "excitation_type": args.excitation,
            "excitation_params": excitation_params,
            "tolerance": args.tolerance,
            "max_iterations": args.max_iterations,
            "use_preconditioner": not args.no_preconditioner,
            "output_dir": args.output or "simulation_output",
            "save_fields": not args.no_fields,
            "save_currents": not args.no_currents
        }
        
        sim_config = SimulationConfig(**config_dict)
        pipeline = SimulationPipeline(use_ctypes=not args.use_executable)
        config = pipeline.run_simulation(sim_config)
    
    # Print results
    if config.success:
        print("\n" + "="*60)
        print("Simulation Completed Successfully!")
        print("="*60)
        print(f"Execution time: {config.execution_time:.2f} seconds")
        print(f"Mesh: {config.num_vertices} vertices, {config.num_elements} elements")
        print(f"Basis functions: {config.num_basis_functions}")
        print(f"Memory usage: {config.memory_usage_mb:.2f} MB")
        print(f"Matrix fill time: {config.matrix_fill_time:.2f} seconds")
        print(f"Solve time: {config.solve_time:.2f} seconds")
        if config.output_files:
            print(f"\nOutput files:")
            for f in config.output_files:
                print(f"  - {f}")
    else:
        print(f"\nSimulation failed: {config.error_message}")
        sys.exit(1)


if __name__ == "__main__":
    main()
