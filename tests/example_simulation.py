#!/usr/bin/env python3
"""
Example Simulation Script

Demonstrates how to use the unified simulation pipeline to run
electromagnetic simulations with PulseMoM.
"""

import sys
from pathlib import Path

# Add project root to path
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from python.tools.simulation_pipeline import SimulationPipeline, SimulationConfig


def example_mom_simulation():
    """Example: MoM simulation of a patch antenna"""
    
    print("="*60)
    print("Example: MoM Simulation")
    print("="*60)
    
    # Create simulation configuration
    config = SimulationConfig(
        geometry_file="tests/patch_antenna_2x2.stl",
        geometry_format="stl",
        solver_type="mom",
        frequency=10e9,  # 10 GHz
        mesh_density=10.0,  # 10 elements per wavelength
        materials={
            "PEC": {
                "epsr": 1.0,
                "mur": 1.0,
                "sigma": 1e8  # Perfect conductor
            }
        },
        excitation_type="plane_wave",
        excitation_params={
            "theta": 0.0,  # Normal incidence
            "phi": 0.0,
            "polarization": "TE",
            "amplitude": 1.0
        },
        tolerance=1e-6,
        max_iterations=1000,
        use_preconditioner=True,
        output_dir="simulation_output/mom_example",
        save_fields=True,
        save_currents=True
    )
    
    # Create pipeline and run simulation
    pipeline = SimulationPipeline(use_ctypes=True)
    result = pipeline.run_simulation(config)
    
    # Print results
    if result.success:
        print("\n✓ Simulation completed successfully!")
        print(f"  Execution time: {result.execution_time:.2f} seconds")
        print(f"  Mesh: {result.num_vertices} vertices, {result.num_elements} elements")
        print(f"  Basis functions: {result.num_basis_functions}")
        print(f"  Memory usage: {result.memory_usage_mb:.2f} MB")
        print(f"  Results saved to: {result.config.output_dir}")
    else:
        print(f"\n✗ Simulation failed: {result.error_message}")
    
    return result


def example_frequency_sweep():
    """Example: Frequency sweep simulation"""
    
    print("\n" + "="*60)
    print("Example: Frequency Sweep")
    print("="*60)
    
    frequencies = [1e9, 2e9, 5e9, 10e9]  # 1, 2, 5, 10 GHz
    
    config = SimulationConfig(
        geometry_file="tests/patch_antenna_2x2.stl",
        solver_type="mom",
        frequencies=frequencies,
        frequency=frequencies[0],  # Initial frequency
        mesh_density=10.0,
        excitation_type="plane_wave",
        excitation_params={"theta": 0.0, "phi": 0.0, "polarization": "TE"},
        output_dir="simulation_output/frequency_sweep"
    )
    
    pipeline = SimulationPipeline()
    result = pipeline.run_simulation(config)
    
    if result.success:
        print(f"\n✓ Frequency sweep completed for {len(frequencies)} frequencies")
        print(f"  Results saved to: {result.config.output_dir}")
    
    return result


def example_peec_simulation():
    """Example: PEEC circuit simulation"""
    
    print("\n" + "="*60)
    print("Example: PEEC Simulation")
    print("="*60)
    
    config = SimulationConfig(
        geometry_file="tests/pcb_cst_exp2.stp",
        geometry_format="step",
        solver_type="peec",
        frequency=1e9,
        mesh_density=5.0,
        materials={
            "Copper": {
                "epsr": 1.0,
                "mur": 1.0,
                "sigma": 5.8e7  # Copper conductivity
            },
            "FR4": {
                "epsr": 4.4,
                "mur": 1.0,
                "sigma": 0.0
            }
        },
        excitation_type="voltage_source",
        excitation_params={
            "voltage": 1.0,
            "position": [0.0, 0.0, 0.0]
        },
        output_dir="simulation_output/peec_example"
    )
    
    pipeline = SimulationPipeline()
    result = pipeline.run_simulation(config)
    
    if result.success:
        print(f"\n✓ PEEC simulation completed")
        print(f"  Results saved to: {result.config.output_dir}")
    
    return result


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Run example simulations")
    parser.add_argument("--example", type=str, 
                       choices=["mom", "frequency_sweep", "peec", "all"],
                       default="mom",
                       help="Which example to run")
    
    args = parser.parse_args()
    
    if args.example == "mom" or args.example == "all":
        example_mom_simulation()
    
    if args.example == "frequency_sweep" or args.example == "all":
        example_frequency_sweep()
    
    if args.example == "peec" or args.example == "all":
        example_peec_simulation()
