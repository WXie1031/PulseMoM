# Basis Function Rules

## Scope
Basis functions for discretization. These belong to **L2 Discretization Layer**.

## Architecture Rules

### L2 Layer (Discretization)
- ✅ Defines basis function structures
- ✅ Computes basis function geometric properties
- ✅ Provides basis function evaluation
- ❌ Does NOT interpret physics
- ❌ Does NOT evaluate operators

## Supported Basis Functions

### 1. RWG (Rao-Wilton-Glisson) Basis Functions
- **Status**: ✅ Fully implemented
- **Type**: Vector basis functions for surface currents
- **Location**: `src/discretization/basis/rwg_basis.c`
- **Features**:
  - Triangle pair support (positive and negative triangles)
  - Edge-based definition
  - Divergence-conforming (divergence is constant on triangles)
  - Support region computation
  - Geometric property calculation (edge length, areas, centroids)

### 2. Higher-Order Basis Functions
- **Status**: ⚠️ Interface defined
- **Location**: `src/discretization/basis/higher_order_basis.c`
- **Note**: For future expansion

### 3. Rooftop Basis Functions
- **Status**: ⚠️ Interface defined
- **Location**: `src/discretization/basis/rooftop_basis.c`
- **Note**: For PEEC and rectangular elements

## RWG Basis Function Properties

### Structure
```c
typedef struct {
    int id;                        // Basis function index
    int triangle_plus;             // Positive triangle index
    int triangle_minus;            // Negative triangle index
    int edge_index;                // Common edge index
    
    // Geometric properties
    real_t edge_length;            // Edge length
    real_t area_plus;              // Area of positive triangle
    real_t area_minus;             // Area of negative triangle
    geom_point_t centroid_plus;    // Centroid of positive triangle
    geom_point_t centroid_minus;   // Centroid of negative triangle
    geom_point_t edge_vector;      // Edge vector (from minus to plus)
    
    // Support region
    geom_point_t support_center;   // Center of support region
    real_t support_radius;         // Radius of support region
} rwg_basis_t;
```

### Mathematical Definition
- **On positive triangle**: f(r) = (l / (2*A+)) * (r - r+)
- **On negative triangle**: f(r) = (l / (2*A-)) * (r- - r)
- **Elsewhere**: f(r) = 0

Where:
- l = edge length
- A+ = area of positive triangle
- A- = area of negative triangle
- r+ = vertex opposite to edge in positive triangle
- r- = vertex opposite to edge in negative triangle

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| RWG basis creation | ✅ | `rwg_basis_create()` | Fully implemented |
| RWG basis evaluation | ✅ | `rwg_basis_evaluate()` | Fully implemented |
| RWG divergence | ✅ | `rwg_basis_divergence()` | Fully implemented |
| Basis set management | ✅ | `rwg_basis_set_*()` | Fully implemented |
| Higher-order basis | ⚠️ | Interface only | Future expansion |
| Rooftop basis | ⚠️ | Interface only | For PEEC |

## File Locations

- **L2 Interface**: `src/discretization/basis/rwg_basis.h`
- **L2 Implementation**: `src/discretization/basis/rwg_basis.c`
- **Higher-order**: `src/discretization/basis/higher_order_basis.h`
- **Rooftop**: `src/discretization/basis/rooftop_basis.h`

## Usage

### Create RWG Basis Set
```c
rwg_basis_set_t* basis_set = rwg_basis_set_create(mesh);
if (!basis_set) {
    // Handle error
}
```

### Evaluate Basis Function
```c
complex_t value[3];
rwg_basis_evaluate(&basis_set->basis[i], point, value);
// value contains [fx, fy, fz]
```

### Compute Divergence
```c
complex_t div = rwg_basis_divergence(&basis_set->basis[i], point);
```

## Core Constraints

- **No physics**: Basis functions are geometric, not physical
- **No operator evaluation**: L2 defines basis, L3 evaluates operators
- **Solver-agnostic**: Basis functions work with any solver
- **Layer separation**: L2 defines discretization, L3 uses for operators
