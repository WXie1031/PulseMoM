# Simulation Pipeline Guide

## Overview

This document describes the unified simulation pipeline for PulseMoM, which provides a Python interface for running electromagnetic simulations using either:
1. **ctypes interface** (recommended): Direct calls to C library
2. **Executable interface** (fallback): Subprocess calls to C executables

## Architecture

The simulation pipeline follows the workflow defined in the architecture (L5 Execution Orchestration Layer):

```
Geometry Import → Mesh Generation → Basis Function Setup → 
Matrix Assembly → Solver Execution → Post-Processing
```

## Components

### 1. Simulation Pipeline (`python/tools/simulation_pipeline.py`)

Main pipeline class that orchestrates the complete simulation workflow.

**Key Features:**
- Automatic interface selection (ctypes preferred, executable fallback)
- Complete workflow automation
- Error handling and recovery
- Result saving and organization

**Usage:**
```python
from python.tools.simulation_pipeline import SimulationPipeline, SimulationConfig

config = SimulationConfig(
    geometry_file="geometry.stl",
    solver_type="mom",
    frequency=10e9
)

pipeline = SimulationPipeline(use_ctypes=True)
result = pipeline.run_simulation(config)
```

### 2. Command-Line Tool (`python/tools/run_simulation.py`)

Easy-to-use command-line interface for running simulations.

**Basic Usage:**
```bash
# Using configuration file
python python/tools/run_simulation.py --config my_config.json

# Using command-line arguments
python python/tools/run_simulation.py \
    --geometry geometry.stl \
    --frequency 10e9 \
    --solver mom \
    --output results/
```

**Create Example Config:**
```bash
python python/tools/run_simulation.py --create-config
```

### 3. Example Scripts (`tests/example_simulation.py`)

Complete examples demonstrating:
- MoM simulation
- Frequency sweep
- PEEC simulation

## Configuration

### Configuration File Format

```json
{
  "geometry_file": "path/to/geometry.stl",
  "geometry_format": "stl",
  "solver_type": "mom",
  "frequency": 1e9,
  "frequencies": [1e9, 2e9, 5e9],
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
    "polarization": "TE"
  },
  "tolerance": 1e-6,
  "max_iterations": 1000,
  "output_dir": "simulation_output"
}
```

### Configuration Options

#### Geometry
- `geometry_file`: Path to geometry file (STL, STEP, IGES)
- `geometry_format`: File format ("stl", "step", "iges")

#### Solver
- `solver_type`: "mom", "peec", or "hybrid"
- `frequency`: Single frequency (Hz)
- `frequencies`: List of frequencies for sweep

#### Mesh
- `mesh_density`: Elements per wavelength
- `min_element_size`: Minimum element size (optional)
- `max_element_size`: Maximum element size (optional)

#### Materials
- Dictionary mapping material names to properties:
  - `epsr`: Relative permittivity
  - `mur`: Relative permeability
  - `sigma`: Conductivity (S/m)

#### Excitation
- `excitation_type`: "plane_wave", "voltage_source", "current_source"
- `excitation_params`: Excitation-specific parameters

#### Solver Parameters
- `tolerance`: Convergence tolerance
- `max_iterations`: Maximum solver iterations
- `use_preconditioner`: Enable/disable preconditioner

## Interface Selection

The pipeline automatically selects the best available interface:

1. **ctypes** (preferred):
   - Direct library calls
   - Faster execution
   - More reliable
   - Better error handling

2. **Executable** (fallback):
   - Subprocess calls
   - Works when library not available
   - Requires executable to be built

You can force a specific interface:
```python
# Force ctypes
pipeline = SimulationPipeline(use_ctypes=True)

# Force executable
pipeline = SimulationPipeline(use_ctypes=False)
```

## Output Files

Simulation results are automatically saved to the specified output directory:

- `config_TIMESTAMP.json`: Complete simulation configuration
- `summary_TIMESTAMP.json`: Simulation summary and performance metrics
- `currents_TIMESTAMP.npy`: Current distribution (NumPy array)
- `fields_TIMESTAMP.npy`: Field distribution (NumPy array)

## Examples

### Example 1: Basic MoM Simulation

```python
from python.tools.simulation_pipeline import SimulationPipeline, SimulationConfig

config = SimulationConfig(
    geometry_file="antenna.stl",
    solver_type="mom",
    frequency=10e9,
    mesh_density=10.0,
    output_dir="results/mom_simulation"
)

pipeline = SimulationPipeline()
result = pipeline.run_simulation(config)

if result.success:
    print(f"Simulation completed in {result.execution_time:.2f} seconds")
    print(f"Basis functions: {result.num_basis_functions}")
```

### Example 2: Frequency Sweep

```python
config = SimulationConfig(
    geometry_file="antenna.stl",
    solver_type="mom",
    frequencies=[1e9, 2e9, 5e9, 10e9],
    frequency=1e9,  # Initial frequency
    output_dir="results/frequency_sweep"
)

pipeline = SimulationPipeline()
result = pipeline.run_simulation(config)
```

### Example 3: PEEC Circuit Simulation

```python
config = SimulationConfig(
    geometry_file="pcb.step",
    geometry_format="step",
    solver_type="peec",
    frequency=1e9,
    materials={
        "Copper": {"epsr": 1.0, "mur": 1.0, "sigma": 5.8e7},
        "FR4": {"epsr": 4.4, "mur": 1.0, "sigma": 0.0}
    },
    excitation_type="voltage_source",
    excitation_params={"voltage": 1.0},
    output_dir="results/peec_simulation"
)

pipeline = SimulationPipeline()
result = pipeline.run_simulation(config)
```

## Integration with Existing Code

The pipeline integrates with existing Python interfaces:

- **ctypes interface**: Uses `python_interface/mom_peec_ctypes.py`
- **Executable interface**: Uses `python/tools/c_executable_interface.py`

## Error Handling

The pipeline includes comprehensive error handling:

- Automatic interface fallback
- Detailed error messages
- Graceful degradation
- Result validation

## Performance Considerations

- **ctypes interface**: Lower overhead, faster execution
- **Executable interface**: Higher overhead, but more robust for complex workflows
- **Parallel execution**: Supported when configured
- **GPU acceleration**: Available when enabled

## Troubleshooting

### Library Not Found
If ctypes interface fails:
1. Check that library is built: `build/Release/pulsemom_core.dll` (Windows)
2. Verify library path in configuration
3. Fallback to executable interface

### Executable Not Found
If executable interface fails:
1. Check that executables are built
2. Verify executable paths
3. Check build configuration

### Simulation Fails
1. Check geometry file exists and is valid
2. Verify material properties
3. Check mesh generation parameters
4. Review error messages in result object

## Best Practices

1. **Use configuration files** for complex simulations
2. **Use ctypes interface** when possible (faster)
3. **Save results** to organized output directories
4. **Validate inputs** before running simulation
5. **Monitor performance metrics** for optimization

## Future Enhancements

- Batch simulation support
- Distributed execution
- Real-time progress monitoring
- Advanced visualization integration
- Result comparison tools
