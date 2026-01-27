# File I/O Rules

## Scope
File format I/O for geometry input and results output. These belong to **L6 IO/Workflow/API Layer**.

## Architecture Rules

### L6 Layer (IO/Workflow/API)
- ✅ Handles file formats (reading/writing)
- ✅ Does NOT interpret physics
- ✅ Does NOT change simulation semantics
- ✅ Provides format conversion only

## Supported Input Formats

### Geometry Input
- **STL**: ✅ Fully implemented (ASCII and binary) in L2 layer
- **OBJ**: ✅ Fully implemented in L2 layer
- **DXF**: ✅ Basic implementation in L2 layer (supports 3DFACE and POLYLINE entities)
- **STEP**: ⚠️ Interface defined, implementation in L2 layer (pending)
- **IGES**: ⚠️ Interface defined, implementation in L2 layer (pending)
- **GDSII**: ⚠️ Interface defined, implementation in L2 layer (pending)
- **OASIS**: ⚠️ Interface defined, implementation in L2 layer (pending)
- **GERBER**: ⚠️ Interface defined, implementation in L2 layer (pending)

**Note**: Geometry file parsing is delegated to L2 Discretization layer.

## Supported Output Formats

### 1. Touchstone Format (.s2p, .s4p, etc.)
- **Status**: ✅ Fully implemented
- **Standard**: Touchstone File Format Specification
- **Features**:
  - S-parameters in real/imaginary format
  - Frequency points in Hz
  - Reference impedance (default 50Ω)
  - Multi-port support (2-port, 4-port, etc.)

### 2. CSV Format
- **Status**: ✅ Fully implemented
- **Features**:
  - Frequency column
  - S-parameter columns (Real/Imaginary pairs)
  - Header row with column names
  - Easy import into spreadsheet applications

### 3. JSON Format
- **Status**: ✅ Fully implemented
- **Features**:
  - Structured data format
  - Frequency array
  - S-parameter matrices (3D array: [frequency][port_i][port_j])
  - Human-readable and machine-parseable

### 4. HDF5 Format
- **Status**: ⚠️ Interface defined, requires HDF5 library
- **Features**:
  - Binary format for large datasets
  - Hierarchical data organization
  - Compression support

### 5. VTK Format
- **Status**: ✅ Fully implemented
- **Features**:
  - Visualization format (VTK structured grid)
  - S-parameter magnitude, phase, real, imaginary components
  - Frequency x ports x ports grid structure
  - Compatible with ParaView, VTK viewers

### 6. SPICE Netlist
- **Status**: ✅ Fully implemented
- **Features**:
  - Circuit netlist export
  - S-parameter to Y-parameter conversion
  - Equivalent circuit model (R, L, C elements)
  - AC sweep analysis
  - Compatible with SPICE simulators (ngspice, LTspice, etc.)

## Results Data Structure

L6 layer uses a generic results structure:

```c
typedef struct {
    int num_frequencies;
    real_t* frequencies;          // Frequency points [Hz]
    int num_ports;
    complex_t* s_parameters;      // S-parameters [num_freq * num_ports * num_ports]
    complex_t* z_parameters;      // Z-parameters (optional)
    complex_t* y_parameters;      // Y-parameters (optional)
    char** port_names;            // Port names (optional)
} simulation_results_t;
```

## Implementation Status

| Format | Read | Write | Notes |
|--------|------|-------|-------|
| Touchstone | ❌ | ✅ | Write fully implemented |
| CSV | ❌ | ✅ | Write fully implemented |
| JSON | ❌ | ✅ | Write fully implemented |
| HDF5 | ❌ | ⚠️ | Placeholder (requires HDF5 library) |
| VTK | ❌ | ✅ | Write fully implemented |
| SPICE | ❌ | ✅ | Write fully implemented |

## File Locations

- **L6 Interface**: `src/io/file_formats/file_io.h`
- **L6 Implementation**: `src/io/file_formats/file_io.c`

## Usage

### Write Touchstone Format
```c
simulation_results_t results;
// ... populate results ...

int status = file_io_write_results("output.s2p", OUTPUT_FORMAT_TOUCHSTONE, &results);
```

### Write CSV Format
```c
int status = file_io_write_results("output.csv", OUTPUT_FORMAT_CSV, &results);
```

### Write JSON Format
```c
int status = file_io_write_results("output.json", OUTPUT_FORMAT_JSON, &results);
```

## Core Constraints

- **I/O only**: Handles file formats, does NOT interpret physics
- **No semantics change**: File I/O does NOT alter simulation results
- **Format conversion**: Only converts data format, not data meaning
- **Layer separation**: L6 handles I/O, L5 provides results data

## Notes

- Geometry file reading is delegated to L2 Discretization layer
- Results file writing is implemented in L6 layer
- HDF5 support requires external HDF5 library
- Touchstone format follows industry standard specification
