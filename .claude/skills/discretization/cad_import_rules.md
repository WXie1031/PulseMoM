# CAD Import Rules

## Scope
CAD file to geometry/mesh translation. These belong to **L2 Discretization Layer** and **L6 I/O Layer**.

## Architecture Rules

### L6 Layer (I/O)
- ✅ Handles file format detection
- ✅ Opens and reads files
- ✅ Delegates parsing to L2 layer
- ❌ Does NOT interpret geometry

### L2 Layer (Discretization)
- ✅ Parses geometry from CAD files
- ✅ Converts to internal geometry representation
- ✅ Validates geometric entities
- ❌ Does NOT interpret physics
- ❌ Does NOT make solver decisions

## Core Constraints
- **No physics decisions**: CAD import is purely geometric
- **No solver selection**: Imported geometry works with any solver
- **Layer separation**: L6 handles I/O, L2 handles parsing

## Supported CAD Formats

### Implemented Formats ✅
- **STL (Stereolithography)**: ✅ Fully implemented
  - ASCII and binary formats
  - Triangle mesh import
  - Location: `src/discretization/geometry/geometry_engine.c`
- **OBJ (Wavefront OBJ)**: ✅ Fully implemented
  - Vertex and face parsing
  - Polygon triangulation
  - Location: `src/discretization/geometry/geometry_engine.c`
- **DXF (Drawing Exchange Format)**: ✅ Basic implementation
  - 3DFACE entity support
  - POLYLINE entity support
  - Location: `src/discretization/geometry/geometry_engine.c`

### Pending Formats ⚠️
- **STEP (ISO 10303)**: ⚠️ Interface defined
  - Requires specialized parser (OpenCASCADE, etc.)
- **IGES (Initial Graphics Exchange Specification)**: ⚠️ Interface defined
  - Requires specialized parser
- **GDSII**: ⚠️ Interface defined
  - For semiconductor layout
- **OASIS**: ⚠️ Interface defined
  - For semiconductor layout
- **GERBER**: ⚠️ Interface defined
  - For PCB layout (see `src/io/pcb_file_io.c` for partial implementation)

## Implementation Status

| Format | Read | Write | Location | Notes |
|--------|------|-------|----------|-------|
| STL | ✅ | ❌ | `geometry_engine.c` | ASCII and binary |
| OBJ | ✅ | ❌ | `geometry_engine.c` | With triangulation |
| DXF | ✅ | ❌ | `geometry_engine.c` | Basic 3DFACE/POLYLINE |
| STEP | ⚠️ | ❌ | Interface only | Requires parser |
| IGES | ⚠️ | ❌ | Interface only | Requires parser |
| GDSII | ⚠️ | ❌ | Interface only | Requires parser |
| GERBER | ⚠️ | ❌ | `pcb_file_io.c` | Partial implementation |

## File Locations

- **L6 I/O Interface**: `src/io/file_formats/file_io.h`
- **L6 I/O Implementation**: `src/io/file_formats/file_io.c`
- **L2 Geometry Engine**: `src/discretization/geometry/geometry_engine.h`
- **L2 Geometry Implementation**: `src/discretization/geometry/geometry_engine.c`
- **PCB-specific**: `src/io/pcb_file_io.c`

## Usage

### Import CAD File (L6 → L2)
```c
void* geometry_data = NULL;
int status = file_io_import_geometry("model.stl", FILE_FORMAT_STL, &geometry_data);
if (status == STATUS_SUCCESS) {
    // geometry_data is a geometry_engine pointer
    // Use geometry_engine functions to access geometry
}
```

### Direct L2 Import
```c
void* engine = geometry_engine_create();
int status = geometry_engine_import_file(engine, "model.stl", GEOM_FORMAT_STL);
if (status == STATUS_SUCCESS) {
    // Geometry imported
}
```

## Core Constraints

- **No physics**: CAD import is geometric only
- **No solver awareness**: Imported geometry is solver-agnostic
- **Layer separation**: L6 handles I/O, L2 handles parsing
- **Format-agnostic**: Internal representation is format-independent

