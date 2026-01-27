# Kernel Rules

## Scope
Integral equation kernels for MoM and PEEC. These belong to **L3 Operators Layer**.

## Architecture Rules

### L3 Layer (Operators)
- ✅ Defines kernel **operators** (mathematical form)
- ✅ Defines operator evaluation methods
- ✅ Provides reference implementations
- ❌ Does NOT implement numerical optimizations (belongs to L4)

## Supported Kernels

### 1. MoM Kernels

#### EFIE (Electric Field Integral Equation)
- **Status**: ✅ Fully implemented
- **Operator**: Z_ij = <f_i, E_operator(f_j)>
- **Form**: E_operator = -jωμ * G * J - (1/jωε) * ∇(∇·G*J)
- **Implementation**: `src/operators/kernels/mom_kernel.c` - `mom_kernel_evaluate_efie()`

#### MFIE (Magnetic Field Integral Equation)
- **Status**: ✅ Fully implemented
- **Operator**: Z_ij = <f_i, H_operator(f_j)>
- **Form**: H_operator = ∇ × (G * J)
- **Implementation**: `src/operators/kernels/mom_kernel.c` - `mom_kernel_evaluate_mfie()`

#### CFIE (Combined Field Integral Equation)
- **Status**: ✅ Fully implemented
- **Operator**: Z_ij = α * EFIE + (1-α) * MFIE
- **Implementation**: `src/operators/kernels/mom_kernel.c` - `mom_kernel_evaluate_cfie()`

### 2. PEEC Kernels

#### Inductance Kernel
- **Status**: ✅ Fully implemented
- **Operator**: L_ij = (μ/4π) * ∫∫ (1/r) dS_i dS_j
- **Self-term**: Analytical formula for rectangular elements
- **Mutual-term**: Distance-based calculation
- **Implementation**: `src/operators/kernels/peec_kernel.c` - `peec_kernel_evaluate_inductance()`

#### Potential Coefficient Kernel
- **Status**: ✅ Fully implemented
- **Operator**: P_ij = (1/4πε) * ∫∫ (1/r) dS_i dS_j
- **Self-term**: Analytical formula with aspect ratio correction
- **Mutual-term**: Distance-based calculation
- **Implementation**: `src/operators/kernels/peec_kernel.c` - `peec_kernel_evaluate_potential_coefficient()`

## Implementation Status

| Kernel Type | Interface | Implementation | Notes |
|-------------|-----------|----------------|-------|
| MoM EFIE | ✅ | ✅ Complete | Full operator form |
| MoM MFIE | ✅ | ✅ Complete | Full operator form |
| MoM CFIE | ✅ | ✅ Complete | Combination of EFIE and MFIE |
| PEEC Inductance | ✅ | ✅ Complete | Self and mutual terms |
| PEEC Potential | ✅ | ✅ Complete | Self and mutual terms |

## File Locations

- **L3 Interface**: `src/operators/kernels/mom_kernel.h`, `peec_kernel.h`
- **L3 Implementation**: `src/operators/kernels/mom_kernel.c`, `peec_kernel.c`
- **L1 Physics**: `src/physics/mom/mom_physics.h` - Physical equation definitions

## Usage

### MoM EFIE Kernel
```c
mom_kernel_t* kernel = mom_kernel_create(MOM_FORMULATION_EFIE, frequency);
complex_t Z_ij = mom_kernel_evaluate_efie(kernel, source_tri, test_tri, source_point, test_point);
```

### PEEC Inductance Kernel
```c
peec_kernel_t* kernel = peec_kernel_create(PEEC_FORMULATION_CLASSICAL, frequency);
real_t L_ij = peec_kernel_evaluate_inductance(kernel, source_elem, test_elem);
```

## Core Constraints

- **Operator-only**: Defines mathematical operators, not numerical optimizations
- **Physics-agnostic**: Kernels are operator definitions, physics is in L1
- **Layer separation**: L3 defines operators, L4 implements optimizations
- **Accuracy first**: Reference implementations prioritize correctness

## Notes

- MoM kernels include full EFIE and MFIE operator forms
- PEEC kernels use analytical formulas for self-terms
- Mutual terms use distance-based approximations
- RWG basis function dot product and cross product calculations are optimized
- Basis function overlap is computed based on triangle geometry and distance
- Full basis function integration would require L2 layer mesh information
