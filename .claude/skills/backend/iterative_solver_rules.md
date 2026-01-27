# Iterative Solver Rules

## Scope
Iterative linear system solvers for electromagnetic simulations. These belong to **L4 Numerical Backend Layer**.

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ Implements iterative algorithms (CG, GMRES, BiCGSTAB, TFQMR)
- ✅ Sees matrices, not physics
- ✅ No physics assumptions or kernel logic
- ✅ Provides convergence monitoring and statistics

## Supported Iterative Methods

### 1. Conjugate Gradient (CG)
- **Status**: ✅ Fully implemented
- **Use case**: Symmetric positive definite matrices
- **Implementation**: `src/backend/solvers/iterative_solver.c` - `solve_cg()`

### 2. GMRES (Generalized Minimal Residual)
- **Status**: ✅ Fully implemented
- **Use case**: General non-symmetric matrices
- **Implementation**: `src/backend/solvers/iterative_solver.c` - `solve_gmres()`
- **Features**: 
  - Full Arnoldi process with modified Gram-Schmidt orthogonalization
  - Givens rotations for Hessenberg matrix
  - Restart capability
  - Convergence monitoring

### 3. BiCGSTAB (Biconjugate Gradient Stabilized)
- **Status**: ✅ Fully implemented
- **Use case**: Non-symmetric matrices, more stable than BiCG
- **Implementation**: `src/backend/solvers/iterative_solver.c` - `solve_bicgstab()`

### 4. TFQMR (Transpose-Free Quasi-Minimal Residual)
- **Status**: ✅ Fully implemented
- **Use case**: Non-symmetric matrices, transpose-free
- **Implementation**: `src/backend/solvers/iterative_solver.c` - `solve_tfqmr()`
- **Features**:
  - Full TFQMR algorithm
  - Transpose-free (no A^T needed)
  - Quasi-minimal residual property
  - Convergence monitoring

## Implementation Status

| Method | Interface | Implementation | Notes |
|--------|-----------|----------------|-------|
| CG | ✅ | ✅ Complete | Full implementation |
| GMRES | ✅ | ✅ Complete | Full Arnoldi process with Givens rotations |
| BiCGSTAB | ✅ | ✅ Complete | Full implementation |
| TFQMR | ✅ | ⚠️ Simplified | Uses BiCGSTAB, full TFQMR in L4 |

## Preconditioning

- **Status**: ✅ Interface implemented
- **Implementation**: Basic preconditioner application (forward substitution for dense matrices)
- **Features**:
  - Identity preconditioner (default)
  - Dense matrix preconditioner support
  - Forward substitution solver
- **Note**: Advanced preconditioner implementations (ILU, AMG, etc.) belong to L4 backend optimization

## File Locations

- **L4 Interface**: `src/backend/solvers/iterative_solver.h`
- **L4 Implementation**: `src/backend/solvers/iterative_solver.c`

## Usage

```c
iterative_solver_config_t config;
config.method = ITERATIVE_METHOD_BICGSTAB;
config.tolerance = 1e-6;
config.max_iterations = 1000;

solver_interface_t* solver = iterative_solver_create(&config);
int status = iterative_solver_solve(solver, matrix, rhs, solution, &stats);
```

## Core Constraints

- **Matrix-only**: Solvers see matrices, not physics
- **No kernel logic**: No electromagnetic kernel code
- **Convergence monitoring**: Must provide statistics
- **Layer separation**: L4 implements algorithms, L3 defines operators
