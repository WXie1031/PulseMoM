# Material Properties Rules

## Scope
Material properties and dielectric constants for electromagnetic simulation. These belong to **L1 Physics Layer** and **L3 Operators Layer**.

## Architecture Rules

### L1 Layer (Physics)
- ✅ Defines material properties in physical terms
- ✅ Defines frequency-dependent material models
- ✅ Provides material property calculations
- ❌ Does NOT implement numerical evaluation

### L3 Layer (Operators)
- ✅ Uses material properties in operator evaluation
- ✅ Integrates material properties into kernels
- ✅ Provides material-aware Green's functions
- ❌ Does NOT define physics

## Supported Material Properties

### 1. Basic Properties
- **Relative Permittivity (ε_r)**: Dielectric constant
- **Relative Permeability (μ_r)**: Magnetic permeability
- **Conductivity (σ)**: Electrical conductivity [S/m]
- **Loss Tangent (tan δ)**: Dielectric loss

### 2. Frequency-Dependent Properties
- **Complex Permittivity**: ε = ε₀ * ε_r * (1 - j*tan δ)
- **Complex Permeability**: μ = μ₀ * μ_r (for non-magnetic materials)
- **Effective Wavenumber**: k = ω * √(ε * μ)
- **Wave Impedance**: η = √(μ / ε)

## Material Library

### Supported Materials
- **Free Space / Air**: ε_r = 1.0, μ_r = 1.0
- **FR4**: ε_r = 4.4, tan δ = 0.02
- **Rogers RO4003**: ε_r = 3.38, tan δ = 0.0027
- **Rogers RO4350**: ε_r = 3.48, tan δ = 0.0037
- **Taconic TLY**: ε_r = 2.2, tan δ = 0.0009
- **Silicon**: ε_r = 11.7
- **Gallium Arsenide**: ε_r = 12.9
- **Alumina**: ε_r = 9.8, tan δ = 0.0001
- **Teflon (PTFE)**: ε_r = 2.1, tan δ = 0.0002

## Implementation Status

| Feature | L1 Layer | L3 Layer | Status |
|---------|----------|----------|--------|
| Material property definition | ✅ | - | Complete |
| Frequency-dependent calculation | ✅ | - | Complete |
| MoM kernel integration | - | ✅ | Complete |
| PEEC kernel integration | - | ✅ | Complete |
| Green's function integration | - | ✅ | Complete |
| Material library | ✅ | - | Complete |

## File Locations

- **L1 Interface**: `src/physics/mom/mom_physics.h`, `src/physics/peec/peec_physics.h`
- **L1 Implementation**: `src/physics/mom/mom_physics.c`, `src/physics/peec/peec_physics.c`
- **L3 Interface**: `src/operators/kernels/mom_kernel.h`, `src/operators/kernels/peec_kernel.h`
- **L3 Implementation**: `src/operators/kernels/mom_kernel.c`, `src/operators/kernels/peec_kernel.c`
- **Material Library**: `src/materials/material_library.h`, `src/materials/material_library.c`

## Usage

### Set Material in MoM Kernel
```c
mom_kernel_t* kernel = mom_kernel_create(MOM_FORMULATION_EFIE, frequency);

mom_material_t material;
material_library_get_mom_material(MATERIAL_FR4, &material);
mom_kernel_set_material(kernel, &material);
```

### Set Material in PEEC Kernel
```c
peec_kernel_t* kernel = peec_kernel_create(PEEC_FORMULATION_CLASSICAL, frequency);

peec_material_t material;
material_library_get_peec_material(MATERIAL_FR4, &material);
peec_kernel_set_material(kernel, &material);
```

### Compute Material Properties at Frequency
```c
complex_t eps, mu;
real_t conductivity;
material_library_get_at_frequency(MATERIAL_FR4, frequency, &eps, &mu, &conductivity);
```

## Core Constraints

- **L1 defines physics**: Material properties are physical definitions
- **L3 uses properties**: Operators use material properties but don't define them
- **Frequency-dependent**: Material properties can vary with frequency
- **Layer separation**: L1 defines, L3 uses

## Notes

- Material properties affect wavenumber and impedance calculations
- Lossy materials are handled through complex permittivity
- Frequency-dependent materials can use Debye/Lorentz models (in core layer)
- Layered media support is available for multi-layer structures
