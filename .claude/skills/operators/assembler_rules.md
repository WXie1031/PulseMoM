# Matrix Assembler Rules

## Scope
Operator to matrix mapping. These belong to **L3 Operators Layer**.

## Architecture Rules

### L3 Layer (Operators)
- ✅ Assembles operator matrices from kernels
- ✅ Backend-agnostic (works with any numerical backend)
- ✅ Deterministic assembly (same input → same output)
- ❌ Does NOT call solvers
- ❌ Does NOT contain physics assumptions

## Supported Assembly Operations

### 1. MoM Impedance Matrix Assembly
- **Status**: ✅ Fully implemented
- **Operator**: Z_ij = <f_i, operator(f_j)>
- **Methods**: EFIE, MFIE, CFIE
- **Implementation**: `src/operators/assembler/matrix_assembler.c` - `matrix_assembler_assemble_mom()`

### 2. PEEC Circuit Matrix Assembly
- **Status**: ✅ Fully implemented
- **Operator**: R, L, C, P matrices
- **Implementation**: `src/operators/assembler/matrix_assembler.c` - `matrix_assembler_assemble_peec()`
- **Features**:
  - Resistance matrix (R) assembly
  - Inductance matrix (L) assembly
  - Potential coefficient matrix (P) assembly
  - System matrix: Z = R + jωL - j/(ωP)

### 3. Excitation Vector Assembly
- **Status**: ✅ Fully implemented
- **Operator**: V_i = <f_i, E_inc>
- **Methods**: Plane wave, voltage source, current source
- **Implementation**: `src/operators/assembler/matrix_assembler.c` - `matrix_assembler_assemble_excitation()`

## Implementation Status

| Assembly Type | Interface | Implementation | Notes |
|---------------|-----------|----------------|-------|
| MoM impedance | ✅ | ✅ Complete | Full EFIE/MFIE/CFIE support |
| PEEC circuit | ✅ | ✅ Complete | Full R, L, C, P matrix assembly |
| Excitation vector | ✅ | ✅ Complete | Plane wave with RWG basis |

## File Locations

- **L3 Interface**: `src/operators/assembler/matrix_assembler.h`
- **L3 Implementation**: `src/operators/assembler/matrix_assembler.c`

## Usage

### MoM Matrix Assembly
```c
matrix_assembly_spec_t spec;
spec.formulation = ASSEMBLY_FORMULATION_EFIE;
spec.frequency = 1e9;
spec.use_parallel = true;

operator_matrix_t* Z = matrix_assembler_create_matrix(MATRIX_TYPE_DENSE, n, n);
matrix_assembler_assemble_mom(mesh, basis_set, &spec, Z);
```

### Excitation Vector Assembly
```c
mom_plane_wave_t excitation;
excitation.k_vector = (point3d_t){0.0, 0.0, 1.0};
excitation.polarization = (point3d_t){1.0, 0.0, 0.0};
excitation.amplitude = 1.0;

operator_vector_t* rhs = matvec_operator_create_vector(n);
matrix_assembler_assemble_excitation(mesh, basis_set, &excitation, rhs);
```

## Core Constraints

- **Backend-agnostic**: Works with any numerical backend (dense, sparse, compressed)
- **Deterministic**: Same input always produces same output
- **No solver calls**: Does not call L4 solvers
- **Layer separation**: L3 assembles operators, L4 solves systems

## Notes

- Matrix assembly is parallelized using OpenMP
- Excitation vector assembly includes proper RWG basis function integration
- PEEC assembly is interface-defined, full implementation would assemble R, L, C, P matrices
