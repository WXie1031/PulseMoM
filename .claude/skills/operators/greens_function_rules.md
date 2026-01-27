# Green's Function Rules

## Scope
Green's function operators for electromagnetic simulations. These belong to **L3 Operators Layer**.

## Supported Green's Function Types

### 1. Free-Space Green's Function
- **Function**: `greens_function_free_space(r, k)`
- **Operator**: G(r) = exp(-jk*r) / (4*π*r)
- **Gradient**: `greens_function_gradient_free_space(r, k, r_vec, gradient)`
- **Operator**: ∇G(r) = -jk * (r_vec/r) * G(r)
- **Status**: ✅ Fully implemented

### 2. Layered Media Green's Function
- **Function**: `greens_function_layered_media(rho, z, z_prime, k0, n_layers, layers)`
- **Operator**: Uses Sommerfeld integral representation
- **Status**: ✅ Interface defined, improved implementation (uses effective medium approximation)
- **Features**:
  - Determines observation and source layers based on z-coordinates
  - Uses average of layer properties for effective permittivity/permeability
  - Handles boundary cases with numerical tolerance
  - Clamps layer indices to valid range
- **Note**: Full numerical evaluation (Sommerfeld/DCIM) belongs to L4 backend

### 3. Periodic Green's Function
- **Function**: `greens_function_periodic(r, k, periodicity, n_harmonics)`
- **Operator**: Periodic extension using Floquet harmonics
- **Status**: ✅ Interface defined, basic implementation (sums nearest periodic images)
- **Features**:
  - Sums contributions from nearest periodic images
  - Supports 1D periodicity (can be extended to 2D/3D)
  - Currently assumes k_bloch = 0 (normal incidence)
  - Uses free-space Green's function for each image
- **Note**: Full implementation (2D/3D periodicity, Bloch wavevector, Ewald summation) belongs to L4 backend

## Green's Function Kernel Types

The following kernel types are defined for singular integration:

```c
typedef enum {
    KERNEL_G = 1,                    // Green's function G
    KERNEL_GRAD_G = 2,              // Gradient of Green's function ∇G
    KERNEL_G_R_R_PRIME = 3,         // G/r² for specific integrals
    KERNEL_DOUBLE_GRAD_G = 4        // Double gradient ∇∇G
} greens_kernel_type_t;
```

## Architecture Rules

### L3 Layer (Operators)
- ✅ Defines Green's function **operators** (mathematical form)
- ✅ Defines interface for different Green's function types
- ✅ Provides reference implementations for free-space
- ❌ Does NOT implement complex numerical methods (belongs to L4)

### L4 Layer (Backend)
- ✅ Implements numerical evaluation methods:
  - Sommerfeld integral evaluation
  - DCIM (Discrete Complex Image Method)
  - Floquet harmonic summation
  - Adaptive quadrature for singular integrals

## Implementation Status

| Green's Function Type | Interface | Implementation | Notes |
|----------------------|-----------|----------------|-------|
| Free-space | ✅ | ✅ Complete | Full implementation |
| Free-space gradient | ✅ | ✅ Complete | Full implementation |
| Layered media | ✅ | ⚠️ Simplified | Uses free-space approximation |
| Periodic | ✅ | ⚠️ Simplified | Uses free-space approximation |

## File Locations

- **L3 Interface**: `src/operators/kernels/greens_function.h`
- **L3 Implementation**: `src/operators/kernels/greens_function.c`
- **L4 Numerical Methods**: `src/backend/` (if implemented)

## Usage

### Free-Space Green's Function
```c
complex_t G = greens_function_free_space(r, k);
```

### Layered Media
```c
complex_t G = greens_function_layered_media(rho, z, z_prime, k0, n_layers, layers);
```

### Periodic
```c
complex_t G = greens_function_periodic(r, k, periodicity, n_harmonics);
```

## Core Constraints

- **Physics-only**: Green's functions define physical operators
- **Numerically stable**: Reference implementations must be stable
- **Layer separation**: L3 defines operators, L4 implements numerical methods
