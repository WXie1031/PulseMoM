# Coupling Operator Rules

## Scope
MoM–PEEC–MTL operator coupling. These belong to **L3 Operators Layer**.

## Architecture Rules

### L3 Layer (Operators)
- ✅ Defines coupling **operators** (mathematical form)
- ✅ Explicit coupling operators (no implicit data sharing)
- ✅ Backend-agnostic (works with any numerical backend)
- ❌ Does NOT orchestrate execution (belongs to L5)
- ❌ Does NOT define physics (belongs to L1)

### L5 Layer (Orchestration)
- ✅ Orchestrates coupling execution
- ✅ Manages data flow between domains
- ✅ Decides when to apply coupling operators

## Supported Coupling Types

### 1. Electric Field Coupling
- **Status**: ✅ Fully implemented
- **Operator**: Capacitive coupling, decreases as 1/r²
- **Use case**: MoM–PEEC electric field coupling
- **Implementation**: `src/operators/coupling/coupling_operator.c`
- **L1 Definition**: `HYBRID_COUPLING_ELECTRIC_FIELD`

### 2. Magnetic Field Coupling
- **Status**: ✅ Fully implemented
- **Operator**: Inductive coupling, decreases as 1/r³
- **Use case**: MoM–PEEC magnetic field coupling
- **Implementation**: `src/operators/coupling/coupling_operator.c`
- **L1 Definition**: `HYBRID_COUPLING_MAGNETIC_FIELD`

### 3. Current Density / Voltage Potential / Power Flow / Mixed Coupling
- **Status**: ✅ Fully implemented
- **Operator**: Green's function form G(r) = exp(-jk*r) / (4*π*r)
- **Use case**: Full-wave coupling between domains
- **Implementation**: `src/operators/coupling/coupling_operator.c`
- **L1 Definitions**: 
  - `HYBRID_COUPLING_CURRENT_DENSITY`
  - `HYBRID_COUPLING_VOLTAGE_POTENTIAL`
  - `HYBRID_COUPLING_POWER_FLOW`
  - `HYBRID_COUPLING_MIXED`

## Implementation Status

| Coupling Type | Interface | Implementation | Notes |
|---------------|-----------|----------------|-------|
| Electric Field | ✅ | ✅ Complete | 1/r² distance dependence |
| Magnetic Field | ✅ | ✅ Complete | 1/r³ distance dependence |
| Current/Voltage/Power/Mixed | ✅ | ✅ Complete | Green's function form |

## File Locations

- **L3 Interface**: `src/operators/coupling/coupling_operator.h`
- **L3 Implementation**: `src/operators/coupling/coupling_operator.c`
- **L1 Physics**: `src/physics/hybrid/hybrid_physics_boundary.h` - Physical coupling definitions

## Usage

### Create Coupling Operator
```c
coupling_operator_t* op = coupling_operator_create(
    HYBRID_COUPLING_ELECTRIC_FIELD,  // From L1 physics layer
    DOMAIN_MOM,
    DOMAIN_PEEC
);
op->coupling_strength = 1.0;
op->frequency = 1e9;
```

### Assemble Coupling Matrix
```c
coupling_matrix_t* matrix = coupling_operator_create_matrix(source_size, target_size);
coupling_operator_assemble_matrix(op, interface_points, num_points, matrix);
```

### Apply Coupling Operator
```c
operator_vector_t* target = matvec_operator_create_vector(target_size);
coupling_operator_apply(matrix, source_vector, target);
```

## Core Constraints

- **Explicit operators**: All coupling must be explicit, no implicit data sharing
- **No orchestration**: L3 defines operators, L5 orchestrates execution
- **No physics**: Coupling operators are mathematical forms, physics is in L1
- **Backend-agnostic**: Works with any numerical backend (dense, sparse, compressed)

## Notes

- Coupling operators are distance-dependent for field coupling types
- Full-wave coupling (current/voltage/power/mixed) uses frequency-dependent Green's function
- Coupling types are defined in L1 physics layer (`hybrid_physics_boundary.h`)
- L3 layer implements operator forms based on L1 physics definitions
- Coupling strength is a parameter that can be adjusted based on L1 physics
