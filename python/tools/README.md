# Simulation Pipeline and Tools

This directory contains unified simulation tools for PulseMoM.

## Main Components

### 1. `simulation_pipeline.py`
Unified simulation pipeline that supports:
- **ctypes interface** (recommended): Direct calls to C library
- **Executable interface** (fallback): Subprocess calls to C executables

The pipeline follows the standard workflow:
1. Geometry Import
2. Mesh Generation
3. Basis Function Setup
4. Matrix Assembly
5. Solver Execution
6. Post-Processing

### 2. `run_simulation.py`
Command-line tool for running simulations with:
- Configuration file support (JSON)
- Command-line argument support
- Example config generation

### 3. `c_executable_interface.py`
Subprocess-based interface for calling C executables when ctypes is not available.

## Usage

### Basic Usage with Configuration File

1. Create a configuration file (or use the example):
```bash
python python/tools/run_simulation.py --create-config
```

2. Edit the configuration file as needed

3. Run the simulation:
```bash
python python/tools/run_simulation.py --config my_config.json
```

### Command-Line Usage

```bash
python python/tools/run_simulation.py \
    --geometry tests/patch_antenna_2x2.stl \
    --frequency 10e9 \
    --solver mom \
    --output simulation_output
```

### Python API Usage

```python
from python.tools.simulation_pipeline import SimulationPipeline, SimulationConfig

# Create configuration
config = SimulationConfig(
    geometry_file="geometry.stl",
    solver_type="mom",
    frequency=10e9,
    mesh_density=10.0
)

# Run simulation
pipeline = SimulationPipeline(use_ctypes=True)
result = pipeline.run_simulation(config)

# Access results
if result.success:
    print(f"Currents: {result.currents}")
    print(f"Fields: {result.fields}")
```

## Example Scripts

See `tests/example_simulation.py` for complete examples of:
- MoM simulation
- Frequency sweep
- PEEC simulation

## Configuration File Format

```json
{
  "geometry_file": "path/to/geometry.stl",
  "geometry_format": "stl",
  "solver_type": "mom",
  "frequency": 1e9,
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

## Interface Selection

The pipeline automatically selects the best available interface:
1. **ctypes** (preferred): Direct library calls, faster, more reliable
2. **Executable** (fallback): Subprocess calls, works when library not available

You can force executable interface:
```python
pipeline = SimulationPipeline(use_ctypes=False)
```

## Output Files

Simulation results are saved to the specified output directory:
- `config_TIMESTAMP.json`: Simulation configuration
- `summary_TIMESTAMP.json`: Simulation summary and metrics
- `currents_TIMESTAMP.npy`: Current distribution (numpy array)
- `fields_TIMESTAMP.npy`: Field distribution (numpy array)

## Error Handling

The pipeline includes comprehensive error handling:
- Automatic fallback between interfaces
- Detailed error messages
- Graceful degradation when features unavailable
