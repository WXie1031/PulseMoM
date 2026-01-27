# Mesh Rules

## Scope
Mesh generation and validation. These belong to **L2 Discretization Layer**.

## Architecture Rules

### L2 Layer (Discretization)
- ✅ Generates meshes from geometry
- ✅ Validates mesh quality
- ✅ Provides mesh topology
- ❌ Does NOT interpret physics
- ❌ Does NOT make solver decisions

## Core Constraints
- **Solver-agnostic**: Mesh works with any solver (MoM, PEEC, hybrid)
- **No physics assumptions**: Mesh is purely geometric
- **Explicit topology**: All connectivity is explicit
- **Deterministic indexing**: Same geometry → same mesh indices

## Mandatory Features
- Explicit topology (vertices, edges, faces, elements)
- Deterministic indexing
- Quality metrics (aspect ratio, skewness, etc.)
- Mesh validation

## Forbidden Patterns
- Solver-specific mesh tweaks
- Physics-dependent mesh refinement
- Hidden mesh modifications

## Supported Mesh Types

### Surface Meshes (for MoM)
- **Triangular**: ✅ Fully implemented
  - Delaunay triangulation
  - Constrained triangulation
  - Quality-based refinement
- **Quadrilateral**: ⚠️ Basic support
  - Manhattan geometry support

### Volume Meshes (for PEEC)
- **Tetrahedral**: ⚠️ Basic support
  - Extrusion from triangular mesh
- **Hexahedral**: ✅ Fully implemented
  - Manhattan mesh generation
  - Structured grid support

## Mesh Generation Methods

### 1. Triangular Mesh Generation
- **Status**: ✅ Fully implemented
- **Location**: `src/discretization/mesh/mesh_engine.c`
- **Methods**:
  - Delaunay triangulation
  - Constrained Delaunay
  - Quality-based refinement
  - Adaptive refinement

### 2. Manhattan Mesh Generation
- **Status**: ✅ Fully implemented
- **Location**: `src/mesh/mesh_algorithms.c`
- **Features**:
  - Rectangular element generation
  - Structured grid support
  - PEEC-compatible

### 3. Tetrahedral Mesh Generation
- **Status**: ⚠️ Basic support
- **Location**: `src/mesh/mesh_algorithms.c`
- **Note**: Uses triangular mesh extrusion

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| Triangular mesh | ✅ | `generate_triangular_mesh()` | Fully implemented |
| Manhattan mesh | ✅ | `generate_manhattan_mesh()` | Fully implemented |
| Tetrahedral mesh | ⚠️ | `generate_tetrahedral_mesh()` | Basic (extrusion) |
| Hexahedral mesh | ✅ | `generate_hexahedral_mesh()` | Fully implemented |
| Mesh validation | ✅ | `mesh_validate()` | Fully implemented |
| Quality metrics | ✅ | Various functions | Fully implemented |

## File Locations

- **L2 Interface**: `src/discretization/mesh/mesh_engine.h`
- **L2 Implementation**: `src/discretization/mesh/mesh_engine.c`
- **Mesh Algorithms**: `src/mesh/mesh_algorithms.c`

## Usage

### Generate Triangular Mesh
```c
mesh_request_t request;
request.preferred_type = MESH_ELEMENT_TRIANGLE;
request.target_size = wavelength / 10.0;
request.adaptive_refinement = true;

mesh_result_t result;
if (generate_triangular_mesh(&request, &result)) {
    // Use mesh
}
```

### Validate Mesh
```c
if (mesh_validate(&mesh)) {
    // Mesh is valid
}
```

## Core Constraints

- **No physics**: Mesh generation is geometric only
- **Solver-agnostic**: Same mesh works for MoM, PEEC, hybrid
- **Deterministic**: Same input produces same mesh
- **Layer separation**: L2 generates mesh, L3 uses for operators

