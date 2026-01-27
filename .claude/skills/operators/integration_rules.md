# Integration Rules

## Scope
Numerical integration for electromagnetic operators. These belong to **L3 Operators Layer**.

## Architecture Rules

### L3 Layer (Operators)
- ✅ Defines integration **methods** (mathematical form)
- ✅ Defines singular integration transformations (Duffy, polar, analytic)
- ✅ Provides reference implementations
- ❌ Does NOT implement solver-specific optimizations (belongs to L4)

### L4 Layer (Backend)
- ✅ Implements numerical optimizations:
  - Adaptive quadrature
  - High-order quadrature rules
  - GPU-accelerated integration
  - Parallel integration

## Supported Integration Methods

### 1. Regular Integration
- **Status**: ✅ Fully implemented
- **Methods**: 
  - Gaussian quadrature (1D, triangle, quadrilateral, hexahedron)
  - Standard quadrature rules
- **Implementation**: `src/operators/integration/integration_utils.c`

### 2. Singular Integration

#### Duffy Transformation
- **Status**: ✅ Fully implemented
- **Use case**: Removes 1/r singularity in triangle integrals
- **Implementation**: `src/operators/integration/singular_integration.c` - `singular_integration_duffy_transform()`
- **Method**: Maps singular triangle to regular domain using (u, v) -> (u, u*v)

#### Polar Coordinate Transformation
- **Status**: ✅ Fully implemented (improved with barycentric coordinate mapping)
- **Use case**: Alternative method for singular integrals
- **Implementation**: `src/operators/integration/singular_integration.c` - `singular_integration_triangle()` with `SINGULAR_METHOD_POLAR`
- **Method**: Maps triangle to polar coordinates centered at observation point
- **Features**:
  - Uses local coordinate system in triangle plane
  - Barycentric coordinate validation to ensure points are inside triangle
  - Proper 3D coordinate mapping from polar coordinates
  - Handles triangle orientation and shape correctly

#### Analytic Integration
- **Status**: ✅ Fully implemented
- **Use case**: Exact integration for specific kernel types
- **Implementation**: `src/operators/integration/singular_integration.c` - `singular_integration_triangle()` with `SINGULAR_METHOD_ANALYTIC`
- **Method**: Uses closed-form expressions for simple cases

#### Adaptive Quadrature
- **Status**: ⚠️ Interface defined, implementation in L4
- **Use case**: Automatic error control

## Singularity Types

### 1. Weak Singularity (1/r)
- **Detection**: ✅ Implemented
- **Treatment**: Duffy transformation or polar coordinates

### 2. Strong Singularity (1/r²)
- **Detection**: ✅ Implemented
- **Treatment**: Duffy transformation with higher-order quadrature

### 3. Hypersingular (1/r³)
- **Detection**: ✅ Implemented
- **Treatment**: Analytic integration or specialized methods

## Implementation Status

| Method | Interface | Implementation | Notes |
|--------|-----------|----------------|-------|
| Regular integration | ✅ | ✅ Complete | Gaussian quadrature (order 7) |
| Duffy transformation | ✅ | ✅ Complete | Full implementation |
| Polar transformation | ✅ | ✅ Complete | Full implementation |
| Analytic integration | ✅ | ✅ Complete | Full implementation |
| Adaptive quadrature | ✅ | ⚠️ L4 | Interface defined |

## File Locations

- **L3 Interface**: `src/operators/integration/singular_integration.h`
- **L3 Implementation**: `src/operators/integration/singular_integration.c`
- **L3 Utilities**: `src/operators/integration/integration_utils.c`
- **L4 Optimizations**: `src/backend/` (adaptive, GPU, etc.)

## Usage

### Regular Integration
```c
real_t points[8][2];
real_t weights[8];
integration_gauss_quadrature_triangle(7, points, weights);
```

### Singular Integration with Duffy Transformation
```c
complex_t result = singular_integration_triangle(
    triangle_vertices, obs_point, k,
    KERNEL_G, SINGULARITY_STRONG, SINGULAR_METHOD_DUFFY
);
```

## Core Constraints

- **Accuracy first**: Integration methods prioritize accuracy
- **Optimizations optional**: Numerical optimizations are optional and belong to L4
- **No solver assumptions**: Integration methods are solver-agnostic
- **Layer separation**: L3 defines methods, L4 implements optimizations
