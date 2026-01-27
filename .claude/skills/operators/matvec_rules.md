# Matrix-Vector Product Rules

## Scope
Matrix-vector product operators for electromagnetic simulations. These belong to **L3 Operators Layer**.

## Architecture Rules

### L3 Layer (Operators)
- ✅ Defines matrix-vector product **operators** (mathematical form)
- ✅ Defines interface for different matrix types (dense, sparse, compressed)
- ✅ Provides reference implementations
- ❌ Does NOT implement complex numerical optimizations (belongs to L4)

### L4 Layer (Backend)
- ✅ Implements numerical optimizations:
  - GPU acceleration
  - BLAS/LAPACK integration
  - Sparse matrix optimizations
  - Compressed matrix operations (ACA, H-matrix)

## Supported Operations

### 1. Matrix-Vector Product: y = A * x
- **Status**: ✅ Fully implemented for dense and sparse matrices
- **Dense**: ✅ Complete implementation
- **Sparse (CSR)**: ✅ Complete implementation
- **Compressed**: ⚠️ Interface defined, implementation in L4

### 2. Transpose Matrix-Vector Product: y = A^T * x
- **Status**: ✅ Fully implemented for dense and sparse matrices
- **Dense**: ✅ Complete implementation
- **Sparse (CSR)**: ✅ Complete implementation

### 3. Hermitian Matrix-Vector Product: y = A^H * x
- **Status**: ✅ Fully implemented for dense and sparse matrices
- **Dense**: ✅ Complete implementation
- **Sparse (CSR)**: ✅ Complete implementation

## Matrix Types

### Dense Matrix
- **Storage**: `complex_t* dense` - Row-major storage [num_rows * num_cols]
- **Status**: ✅ Fully supported

### Sparse Matrix (CSR Format)
- **Storage**: CSR (Compressed Sparse Row) format
  - `int* row_ptr` - Row pointer array (size: num_rows + 1)
  - `int* col_idx` - Column indices (size: nnz)
  - `complex_t* values` - Non-zero values (size: nnz)
- **Status**: ✅ Fully supported for all operations

### Compressed Matrix
- **Storage**: Defined in L4 backend (ACA, H-matrix, etc.)
- **Status**: ⚠️ Interface defined, implementation in L4

## Implementation Status

| Operation | Dense | Sparse (CSR) | Compressed |
|-----------|-------|---------------|------------|
| y = A * x | ✅ | ✅ | ⚠️ L4 |
| y = A^T * x | ✅ | ✅ | ⚠️ L4 |
| y = A^H * x | ✅ | ✅ | ⚠️ L4 |

## File Locations

- **L3 Interface**: `src/operators/matvec/matvec_operator.h`
- **L3 Implementation**: `src/operators/matvec/matvec_operator.c`
- **L4 Optimizations**: `src/backend/` (GPU, BLAS, etc.)

## Usage

### Dense Matrix
```c
operator_matrix_t* A = matrix_assembler_create_matrix(MATRIX_TYPE_DENSE, m, n);
// ... fill matrix ...
operator_vector_t* x = matvec_operator_create_vector(n);
operator_vector_t* y = matvec_operator_create_vector(m);

matvec_operator_apply(A, x, y);  // y = A * x
```

### Sparse Matrix
```c
// Sparse matrix structure (CSR format)
sparse_matrix_csr_t sparse;
sparse.num_rows = m;
sparse.num_cols = n;
sparse.nnz = nnz;
sparse.row_ptr = row_ptr;
sparse.col_idx = col_idx;
sparse.values = values;

operator_matrix_t* A = matrix_assembler_create_matrix(MATRIX_TYPE_SPARSE, m, n);
A->data.sparse = &sparse;

matvec_operator_apply(A, x, y);  // y = A * x (sparse)
```

## Core Constraints

- **Operator-only**: Defines mathematical operators, not numerical optimizations
- **Layer separation**: L3 defines operators, L4 implements optimizations
- **Matrix-agnostic**: Works with any matrix type (dense, sparse, compressed)
