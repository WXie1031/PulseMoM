# Geometry Rules

## Scope
Pure geometry handling and CAD file import. These belong to **L2 Discretization Layer**.

## Architecture Rules

### L2 Layer (Discretization)
- ✅ Handles geometry import and processing
- ✅ Defines geometric primitives (points, lines, triangles, etc.)
- ✅ Provides bounding box computation
- ✅ Validates geometric entities
- ❌ Does NOT interpret physics
- ❌ Does NOT make solver decisions

## Supported Geometry Types

### Basic Primitives
- **Point**: Single 3D coordinate
- **Line**: Two endpoints
- **Triangle**: Three vertices (most common for MoM)
- **Quadrilateral**: Four vertices
- **Rectangle**: Manhattan geometry (for PEEC)
- **Polygon**: N-sided polygon
- **Circle**: 2D circle

### Volume Elements
- **Tetrahedron**: 4-vertex volume element
- **Hexahedron**: 8-vertex volume element (for PEEC)
- **Prism**: Extruded polygon
- **Pyramid**: Base polygon + apex

## Supported File Formats

### Implemented Formats ✅
- **STL**: ✅ Fully implemented (ASCII and binary)
  - Location: `src/discretization/geometry/geometry_engine.c`
  - Features: Triangle mesh import, normal vectors
- **OBJ**: ✅ Fully implemented
  - Location: `src/discretization/geometry/geometry_engine.c`
  - Features: Vertex and face parsing, polygon triangulation
- **DXF**: ✅ Basic implementation
  - Location: `src/discretization/geometry/geometry_engine.c`
  - Features: 3DFACE and POLYLINE entity support

### Pending Formats ⚠️
- **STEP**: ⚠️ Interface defined, implementation pending
- **IGES**: ⚠️ Interface defined, implementation pending
- **GDSII**: ⚠️ Interface defined, implementation pending
- **OASIS**: ⚠️ Interface defined, implementation pending
- **GERBER**: ⚠️ Interface defined, implementation pending

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| Geometry engine creation | ✅ | `geometry_engine_create()` | Fully implemented |
| Geometry entity addition | ✅ | `geometry_engine_add_entity()` | Fully implemented |
| Bounding box computation | ✅ | `geometry_engine_get_bbox()` | Fully implemented |
| Geometry validation | ✅ | `geometry_engine_validate()` | Fully implemented |
| STL import | ✅ | `parse_stl_ascii()`, `parse_stl_binary()` | ASCII and binary |
| OBJ import | ✅ | `parse_obj_file()` | With triangulation |
| DXF import | ✅ | `parse_dxf_file()` | Basic 3DFACE/POLYLINE |
| STEP import | ⚠️ | Interface only | Requires specialized parser |
| IGES import | ⚠️ | Interface only | Requires specialized parser |

## File Locations

- **L2 Interface**: `src/discretization/geometry/geometry_engine.h`
- **L2 Implementation**: `src/discretization/geometry/geometry_engine.c`
- **L6 I/O Interface**: `src/io/file_formats/file_io.h`

## Usage

### Create Geometry Engine
```c
void* engine = geometry_engine_create();
if (!engine) {
    // Handle error
}
```

### Import Geometry File
```c
int status = geometry_engine_import_file(engine, "model.stl", GEOM_FORMAT_STL);
if (status != STATUS_SUCCESS) {
    // Handle error
}
```

### Add Geometry Entity
```c
geom_entity_t entity;
entity.type = GEOM_TYPE_TRIANGLE;
entity.num_vertices = 3;
// ... set vertices ...
geometry_engine_add_entity(engine, &entity);
```

## Core Constraints

- **No physics**: Geometry engine does not interpret electromagnetic properties
- **No solver awareness**: Geometry is solver-agnostic
- **Layer separation**: L2 handles geometry, L6 handles I/O, L3 uses geometry for operators
