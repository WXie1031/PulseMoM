# Direct Solver Rules

## Scope
Direct linear system solvers (LU, QR, Cholesky, SVD). These belong to **L4 Numerical Backend Layer**.

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ Implements direct factorization algorithms (LU, QR, Cholesky)
- ✅ Sees matrices, not physics
- ✅ No physics assumptions or kernel logic
- ✅ Supports both dense and sparse matrices

## Supported Methods

### 1. LU Decomposition
- **Status**: ✅ Fully implemented for dense and sparse matrices
- **Use case**: General square matrices
- **Implementation**: `src/backend/solvers/direct_solver.c` - `lu_decompose_simple()`
- **Note**: Simplified implementation without pivoting (for production, use LAPACK/MKL)

### 2. QR Decomposition
- **Status**: ⚠️ Interface defined, implementation in L4 backend
- **Use case**: Overdetermined systems, least squares

### 3. Cholesky Decomposition
- **Status**: ⚠️ Interface defined, implementation in L4 backend
- **Use case**: Symmetric positive definite matrices

### 4. SVD (Singular Value Decomposition)
- **Status**: ⚠️ Interface defined, implementation in L4 backend
- **Use case**: Rank-deficient systems, pseudo-inverse

## Matrix Type Support

### Dense Matrices
- **Status**: ✅ Fully supported
- **Storage**: Row-major `complex_t*` array
- **Factorization**: In-place LU decomposition

### Sparse Matrices (CSR Format)
- **Status**: ✅ Supported (converted to dense for factorization)
- **Storage**: CSR (Compressed Sparse Row) format
- **Implementation**: Converts CSR to dense, then performs LU decomposition
- **Note**: For production, use specialized sparse LU (SuperLU, MUMPS, etc.)

### Compressed Matrices
- **Status**: ⚠️ Interface defined, implementation in L4 backend
- **Note**: ACA, H-matrix factorization belongs to specialized L4 backend

## Implementation Status

| Method | Dense | Sparse (CSR) | Compressed |
|--------|-------|---------------|------------|
| LU | ✅ | ✅ (via conversion) | ⚠️ L4 |
| QR | ⚠️ L4 | ⚠️ L4 | ⚠️ L4 |
| Cholesky | ⚠️ L4 | ⚠️ L4 | ⚠️ L4 |
| SVD | ⚠️ L4 | ⚠️ L4 | ⚠️ L4 |

## File Locations

- **L4 Interface**: `src/backend/solvers/direct_solver.h`
- **L4 Implementation**: `src/backend/solvers/direct_solver.c`

## Usage

### Dense Matrix
```c
direct_solver_config_t config;
config.method = DIRECT_METHOD_LU;
config.use_pivoting = true;

solver_interface_t* solver = direct_solver_create(&config);
int status = direct_solver_factorize(solver, dense_matrix);
status = direct_solver_solve(solver, rhs, solution);
```

### Sparse Matrix
```c
operator_matrix_t* sparse_matrix = matrix_assembler_create_matrix(MATRIX_TYPE_SPARSE, n, n);
// ... set sparse matrix data (CSR format) ...

int status = direct_solver_factorize(solver, sparse_matrix);
// Sparse matrix is converted to dense internally for factorization
```

## Core Constraints

- **Matrix-only**: Solvers see matrices, not physics
- **No kernel logic**: No electromagnetic kernel code
- **Memory ownership**: Clear ownership of factorization data
- **Layer separation**: L4 implements algorithms, L3 defines operators

## Notes

- Current sparse matrix support converts CSR to dense for factorization
- For production use, integrate specialized sparse direct solvers (SuperLU, MUMPS, PARDISO)
- Pivoting is not yet implemented (simplified LU)
- Out-of-core factorization is interface-defined but not implemented
