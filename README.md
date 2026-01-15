# PulseMoM - Commercial-Grade PEEC-MoM Unified Framework

A comprehensive electromagnetic simulation suite combining Method of Moments (MoM) and Partial Element Equivalent Circuit (PEEC) solvers in a unified framework.

**📚 [完整文档索引](docs/MASTER_DOCUMENTATION_INDEX.md)** | **👋 [从这里开始](docs/START_HERE.md)** | **📖 [如何使用文档](docs/HOW_TO_USE_DOCUMENTATION.md)** | **🚀 [使用指南](docs/USAGE_GUIDE.md)**

## Overview

PulseMoM is a commercial-grade electromagnetic simulation framework that provides:

- **Unified Architecture**: Single repository with multi-package structure
- **Method of Moments (MoM)**: Full-wave electromagnetic analysis with triangular meshes
- **Partial Element Equivalent Circuit (PEEC)**: Circuit-based electromagnetic analysis with Manhattan geometry
- **Hybrid Solving**: Combined MoM-PEEC simulations with domain decomposition
- **Advanced Features**: Wideband analysis, model order reduction, GPU acceleration
- **Industry Formats**: Support for GDSII, Gerber, DXF, OASIS, and other PCB/IC formats

## Architecture

```
PulseMoM/
├── src/
│   ├── core/                    # Unified core framework
│   │   ├── core_geometry.h      # Shared geometry engine
│   │   ├── core_mesh.h          # Unified mesh generation
│   │   ├── core_kernels.h       # Physical kernels and Green functions
│   │   ├── core_assembler.h     # Matrix and network assembly
│   │   ├── core_solver.h        # Linear solvers and preconditioners
│   │   └── core_wideband.h      # Wideband analysis and model reduction
│   └── solvers/
│       ├── mom/                 # Method of Moments solver
│       │   ├── mom_solver.h     # MoM-specific implementations
│       │   ├── mom_basis.c      # Basis functions and testing
│       │   └── mom_farfield.c   # Far-field computation
│       └── peec/                # PEEC solver
│           ├── peec_solver.h    # PEEC-specific implementations
│           ├── peec_elements.c  # Circuit element extraction
│           └── peec_spice.c     # SPICE export
├── apps/
│   ├── mom_cli.c                # MoM command-line interface
│   ├── peec_cli.c               # PEEC command-line interface
│   └── hybrid_cli.c             # Hybrid solver interface
├── examples/                    # Example applications
├── tests/                       # Comprehensive test suite
└── third_party/               # External dependencies
```

## Features

### Core Framework
- **Unified Geometry Engine**: Supports both triangular (MoM) and Manhattan rectangular (PEEC) geometries
- **Shared Mesh Generation**: Adaptive meshing with quality metrics
- **Physical Kernels**: Green functions, influence coefficients, fast algorithms (ACA/MLFMM)
- **Linear Solvers**: Dense LU, sparse LU, iterative methods, GPU acceleration
- **Wideband Analysis**: Vector fitting, model order reduction, passivity enforcement

### Method of Moments (MoM)
- **Triangular Meshes**: RWG basis functions, mesh refinement
- **Fast Algorithms**: Adaptive Cross Approximation (ACA), Multilevel Fast Multipole Method (MLFMM)
- **Excitation Types**: Plane wave, voltage source, current source, waveguide port
- **Output Data**: Surface currents, far-field patterns, radar cross section, S-parameters
- **Advanced Features**: Dielectric modeling, periodic structures, rough surfaces

### PEEC Solver
- **Manhattan Geometry**: Rectangular conductors, via structures, layered media
- **Circuit Elements**: Partial inductances, resistances, capacitances
- **Skin/Proximity Effects**: Frequency-dependent resistance and inductance
- **SPICE Export**: Netlist generation, circuit simulation interface
- **Wideband Models**: Rational function approximation, state-space models

### Hybrid Solving
- **Domain Decomposition**: Geometric, frequency, and hybrid decomposition strategies
- **Coupling Analysis**: MoM-PEEC interaction through Schur complement
- **Adaptive Algorithms**: Automatic region assignment and coupling threshold
- **Unified Interface**: Single framework for mixed-domain problems

## Building

### Requirements
- CMake 3.16+
- C11 compiler (GCC, Clang, MSVC)
- Optional: OpenMP, BLAS/LAPACK, CUDA, MPI

### Build Instructions
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build Options
```bash
cmake .. -DENABLE_OPENMP=ON \n         -DENABLE_BLAS=ON \n         -DENABLE_CUDA=OFF \n         -DENABLE_MPI=OFF \n         -DBUILD_TESTS=ON \n         -DBUILD_EXAMPLES=ON
```

## Usage

### MoM Solver
```bash
# Single frequency analysis
mom_cli -i antenna.gds -o results.dat --freq 1e9

# Frequency sweep
mom_cli -i pcb.gds --freq-start 1e6 --freq-stop 10e9 --freq-points 100

# Far-field computation
mom_cli -i scatterer.gds --freq 5e9 --rcs --pattern

# With fast algorithms
mom_cli -i large_structure.gds --freq 1e9 --aca --accuracy 1e-6 -t 8
```

### PEEC Solver
```bash
# Circuit analysis with SPICE export
peec_cli -i circuit.gds --freq 1e9 --spice netlist.sp

# With skin and proximity effects
peec_cli -i interconnect.gds --skin-depth --proximity-effect --freq 1e9

# Wideband model order reduction
peec_cli -i package.gds --wideband --freq-start 1e6 --freq-stop 10e9

# Manhattan geometry processing
peec_cli -i layout.gds --manhattan --accuracy 1e-8 -t 16
```

### Hybrid Solver
```bash
# Geometric decomposition
hybrid_cli -i complex_package.gds --decomposition geometric --mom-regions 4 --peec-regions 8

# Adaptive decomposition with coupling
hybrid_cli -i mixed_circuit.gds --adaptive --coupling --coupling-threshold 0.05

# Wideband hybrid analysis
hybrid_cli -i rf_system.gds --wideband --freq-start 1e6 --freq-stop 10e9
```

## Input Formats

### Supported File Formats
- **GDSII**: Stream format for IC layout data
- **Gerber**: PCB fabrication data (RS-274X)
- **DXF**: AutoCAD drawing exchange format
- **OASIS**: Open Artwork System Interchange Standard
- **ASCII**: Custom text-based geometry description
- **IPC-2581**: Generic PCB assembly and fabrication data

### Material Properties
```xml
<materials>
    <material name="copper" conductivity="5.8e7" permeability="1.0"/>
    <material name="fr4" permittivity="4.4" loss_tangent="0.02"/>
    <material name="air" permittivity="1.0" permeability="1.0"/>
</materials>
```

## Performance

### Benchmark Results
- **MoM with ACA**: O(N log N) complexity, 1M unknowns on 16 cores
- **PEEC Solver**: O(N) complexity, 10M circuit elements on 8 cores
- **Hybrid Solver**: Adaptive decomposition, 100K-1M regions
- **GPU Acceleration**: 10-50x speedup for large problems
- **Memory Usage**: Optimized for 64GB+ systems with streaming

### Parallel Scaling
- **OpenMP**: Shared-memory parallelization (up to 64 cores)
- **MPI**: Distributed computing for cluster environments
- **CUDA**: GPU acceleration for matrix operations
- **Hybrid**: Combined CPU-GPU computing

## Validation

### Test Cases
- **Analytical Solutions**: Sphere, dipole, patch antenna
- **Measurement Data**: PCB structures, antenna prototypes
- **Commercial Software**: HFSS, CST, FEKO comparison
- **Industry Benchmarks**: IEEE EM simulation benchmarks

### Accuracy Metrics
- **MoM**: < 0.1% error for RCS, < 1% for antenna parameters
- **PEEC**: < 0.5% error for S-parameters, < 2% for coupling
- **Hybrid**: < 1% error compared to full-wave solutions
- **Convergence**: Monotonic convergence with mesh refinement

## Applications

### Antenna Design
- **Wire Antennas**: Dipoles, loops, helices, Yagi arrays
- **Patch Antennas**: Microstrip, cavity-backed, conformal
- **Array Antennas**: Phased arrays, reflectarrays, metasurfaces
- **Feed Networks**: Corporate feeds, Butler matrices

### PCB/Package Analysis
- **Signal Integrity**: Crosstalk, reflection, impedance analysis
- **Power Integrity**: Power delivery network, decoupling optimization
- **EMI/EMC**: Radiated emissions, susceptibility analysis
- **High-Speed**: DDR, PCIe, SerDes channel modeling

### RF/Microwave Circuits
- **Filters**: Microstrip, waveguide, cavity filters
- **Couplers**: Directional, hybrid, rat-race couplers
- **Power Dividers**: Wilkinson, Gysel, traveling-wave
- **Matching Networks**: L-section, π-network, stub matching

### Metamaterials
- **Artificial Media**: Negative index, zero-index materials
- **Metasurfaces**: Reflectarrays, transmitarrays, absorbers
- **Frequency Selective Surfaces**: Bandpass, bandstop, multiband
- **Electromagnetic Bandgap**: EBG structures, AMC surfaces

## API Documentation

### Core Geometry API
```c
// Initialize geometry
geom_geometry_t geometry;
geom_geometry_init(&geometry);

// Load from file
geom_load_gdsii(&geometry, "design.gds");

// Query entities
int num_entities = geometry.num_entities;
geom_entity_t* entity = &geometry.entities[0];
```

### MoM Solver API
```c
// Create solver
mom_solver_t* solver = mom_solver_create();

// Configure solver
solver->config.frequency = 1e9;
solver->config.basis_order = 1;
solver->config.aca_threshold = 1e-6;

// Solve problem
mom_result_t* result = mom_solve_frequency(solver, frequency);
```

### PEEC Solver API
```c
// Create solver
peec_solver_t* solver = peec_solver_create();

// Configure solver
solver->config.skin_effect = 1;
solver->config.proximity_effect = 1;
solver->config.manhattan_geometry = 1;

// Extract circuit
peec_result_t* result = peec_solve_frequency(solver, frequency);
peec_export_spice(solver, "circuit.sp", frequency);
```

## Contributing

### Development Setup
```bash
git clone https://github.com/yourusername/PulseMoM.git
cd PulseMoM
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Code Style
- **C Standard**: C11 with GNU extensions
- **Naming Convention**: snake_case for functions, UPPER_CASE for macros
- **Documentation**: Doxygen comments for public APIs
- **Testing**: Comprehensive unit tests with >90% coverage

### Testing
```bash
# Run all tests
make test

# Run specific test suite
./tests/test_geometry
./tests/test_mom_solver
./tests/test_peec_solver

# Performance benchmarks
./tests/benchmark_mom
./tests/benchmark_peec
```

## License

This project is licensed under the Commercial License - see the LICENSE file for details.

## Support

For technical support and commercial licensing:
- Email: support@pulse-mom.com
- Documentation: https://docs.pulse-mom.com
- Issues: https://github.com/yourusername/PulseMoM/issues

## Acknowledgments

- **Academic Partners**: University electromagnetic research groups
- **Industry Collaborators**: PCB manufacturers, IC design companies
- **Open Source**: Contributions from the electromagnetic simulation community
- **Funding**: Government and private research grants

---

**PulseMoM** - Advanced electromagnetic simulation for the next generation of electronic systems.