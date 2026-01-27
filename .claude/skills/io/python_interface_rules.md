# Python Interface Rules

## Overview

PulseMoM uses Python as the primary interface for testing and integration. All C test executables have been removed in favor of Python-based testing and workflow scripts.

## Architecture

Python interfaces belong to **L6 IO/Workflow/API Layer**:
- ✅ Python bindings via ctypes
- ✅ Python workflow scripts
- ✅ Python test frameworks
- ✅ Python visualization tools

## Python Interface Organization

### Primary Interface Location
- `python_interface/mom_peec_ctypes.py` - Main ctypes interface to C library (general-purpose)
- `python/tools/c_solver_interface.py` - C solver interface with compilation support

### Interface Types

#### 1. ctypes Interface (Recommended)
- **Location**: `python_interface/mom_peec_ctypes.py`
- **Class**: `MoMPEECInterface`
- **Note**: This is a general-purpose interface, not limited to any specific application
- **Features**:
  - Direct C library binding via ctypes
  - Automatic library path detection
  - Type-safe function signatures
  - Memory management
  - Error handling

#### 2. C Solver Interface
- **Location**: `python/tools/c_solver_interface.py`
- **Class**: `CSolverInterface`
- **Features**:
  - Compile C libraries on demand
  - Support for MoM, PEEC, and mesh libraries
  - Cross-platform compilation
  - Mock library fallback

## Usage Rules

### ✅ Allowed
- Python scripts for testing and validation
- Python workflow automation
- Python visualization and post-processing
- Python bindings to C library (L6 layer)
- Python configuration management

### 🚫 Forbidden
- Python code that implements physics (belongs to L1)
- Python code that implements discretization (belongs to L2)
- Python code that implements operators (belongs to L3)
- Python code that implements numerical backend (belongs to L4)
- Python code that implements orchestration logic (belongs to L5)

## Testing Strategy

### Removed C Test Files
- ❌ `src/satellite_main.c` - Use Python scripts instead
- ❌ `src/satellite_main_simple.c` - Use Python scripts instead
- ❌ `src/c_interface/satellite_mom_peec_interface.c/h` - Use Python ctypes instead

### Python Testing
- All tests should use Python interface
- Test scripts in `tests/` directory
- Use `python_interface/mom_peec_ctypes.py` for C library access
- Use `python/tools/` for workflow and analysis tools

## Interface Functions

### Core Functions (via ctypes)
- `create_simulation()` - Create simulation instance
- `destroy_simulation()` - Clean up simulation
- `load_stl()` - Load geometry
- `set_material()` - Set material properties
- `generate_mesh()` - Generate mesh
- `configure_solver()` - Configure solver
- `set_excitation()` - Set excitation
- `run_simulation()` - Execute simulation
- `get_currents()` - Get current distribution
- `get_fields()` - Get field distribution
- `get_performance()` - Get performance metrics

### Convenience Functions
- `run_simulation()` - Complete simulation workflow

## Example Usage

```python
from python_interface.mom_peec_ctypes import MoMPEECInterface

# Create interface
interface = MoMPEECInterface()

# Create simulation
sim = interface.create_simulation()

# Load geometry
interface.load_stl(sim, "geometry.stl")

# Configure and run
interface.configure_solver(sim, {
    'solver_type': 'mom',
    'frequency': 10e9,
    'tolerance': 1e-6
})
interface.run_simulation(sim)

# Get results
currents = interface.get_currents(sim)
performance = interface.get_performance(sim)

# Clean up
interface.destroy_simulation(sim)
```

## Library Path Resolution

The interface automatically searches for the C library in:
1. `build/Release/` (Windows)
2. `build/Debug/` (Windows)
3. `lib/` (all platforms)
4. `bin/` (all platforms)
5. System library paths (Linux/macOS)

## Error Handling

- All functions return error codes (0 = success)
- Use `get_error_string()` for human-readable errors
- Python exceptions are raised for critical failures
- Memory is automatically managed by the interface

## Platform Support

- **Windows**: `.dll` libraries
- **Linux**: `.so` libraries
- **macOS**: `.dylib` libraries

## Migration Notes

- Old C test executables have been removed
- All testing should use Python interface
- Python scripts provide better flexibility and maintainability
- Python interface is the single source of truth for API usage
