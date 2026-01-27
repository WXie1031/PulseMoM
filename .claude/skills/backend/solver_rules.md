# Solver Backend Rules

## Scope
Linear system solvers. These belong to **L4 Numerical Backend Layer**.

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ Implements numerical algorithms for solving linear systems
- ✅ Works with abstract matrices (dense, sparse, compressed)
- ✅ Provides convergence monitoring and statistics
- ❌ Does NOT interpret physics
- ❌ Does NOT contain kernel logic
- ❌ Does NOT make solver decisions

## Core Constraints
- **Matrix-only**: Solver sees matrices, not physics
- **No kernel logic**: No MoM/PEEC-specific code
- **Backend-agnostic**: Works with any matrix type
- **Layer separation**: L4 implements algorithms, L3 defines operators

## Supported Solvers

### Direct Solvers
- **Dense LU**: ✅ Fully implemented
  - Location: `src/backend/solvers/direct_solver.c`
  - Features: LU decomposition, forward/backward substitution
- **Sparse LU**: ⚠️ Basic support (converts to dense)
  - Location: `src/backend/solvers/direct_solver.c`
  - Note: Production should use SuperLU, MUMPS, PARDISO

### Iterative Solvers
- **CG (Conjugate Gradient)**: ✅ Fully implemented
  - Location: `src/backend/solvers/iterative_solver.c`
  - For: Symmetric positive definite matrices
- **GMRES (Generalized Minimal Residual)**: ✅ Fully implemented
  - Location: `src/backend/solvers/iterative_solver.c`
  - Features: Arnoldi process, Givens rotations, restart
- **BiCGSTAB**: ✅ Fully implemented
  - Location: `src/backend/solvers/iterative_solver.c`
  - For: Non-symmetric matrices
- **TFQMR**: ✅ Implemented (uses BiCGSTAB)
  - Location: `src/backend/solvers/iterative_solver.c`
  - Note: Full TFQMR algorithm in L4

## Implementation Status

| Solver Type | Interface | Implementation | Status | Notes |
|-------------|-----------|----------------|--------|-------|
| Dense LU | ✅ | ✅ | Complete | Full implementation |
| Sparse LU | ✅ | ⚠️ | Basic | Converts to dense |
| CG | ✅ | ✅ | Complete | Full implementation |
| GMRES | ✅ | ✅ | Complete | Full Arnoldi process |
| BiCGSTAB | ✅ | ✅ | Complete | Full implementation |
| TFQMR | ✅ | ⚠️ | Simplified | Uses BiCGSTAB |

## File Locations

- **Solver Interface**: `src/backend/solvers/solver_interface.h`
- **Direct Solver**: `src/backend/solvers/direct_solver.c`
- **Iterative Solver**: `src/backend/solvers/iterative_solver.c`

## Usage

### Direct Solver
```c
solver_interface_t* solver = direct_solver_create(matrix);
int status = direct_solver_solve(solver, rhs, solution);
```

### Iterative Solver
```c
iterative_solver_config_t config;
config.method = ITERATIVE_METHOD_GMRES;
config.tolerance = 1e-6;
config.max_iterations = 1000;

solver_interface_t* solver = iterative_solver_create(&config);
solver_statistics_t stats;
int status = iterative_solver_solve(solver, matrix, rhs, solution, &stats);
```

## Core Constraints

- **Matrix-only**: Solver operates on matrices, not physics
- **No kernel logic**: No MoM/PEEC-specific code
- **Backend-agnostic**: Works with any matrix type
- **Layer separation**: L4 implements algorithms, L3 defines operators
