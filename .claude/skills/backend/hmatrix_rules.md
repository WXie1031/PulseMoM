# H-Matrix Rules

## Scope
Hierarchical matrix compression for fast algorithms. These belong to **L4 Numerical Backend Layer**.

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ Implements hierarchical matrix compression
- ✅ Provides error-controlled approximation
- ✅ Maintains operator semantics
- ✅ Provides transparent fallback to dense matrices
- ❌ Does NOT interpret physics
- ❌ Does NOT make solver decisions

## Core Constraints
- **Error-controlled approximation**: Compression respects specified tolerance
- **Transparent fallback**: Automatically falls back to dense if compression fails
- **Operator semantics preserved**: Compressed matrices produce same results as dense (within tolerance)
- **Backend-agnostic**: Works with any operator (MoM, PEEC, etc.)

## H-Matrix Structure

### Block Cluster Tree
- Hierarchical decomposition of matrix into blocks
- Admissible blocks: Well-separated (can be compressed)
- Non-admissible blocks: Near-field (must be dense)

### Compression Format
- **Low-rank blocks**: U * V^T factorization
- **Dense blocks**: Full matrix storage
- **Zero blocks**: Implicit storage

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| H-matrix structure | ✅ | `src/core/h_matrix_compression.h` | Fully defined |
| Block cluster tree | ⚠️ | Interface defined | Implementation in L4 |
| Admissibility condition | ✅ | `mom_hmatrix.c` | Implemented |
| Low-rank compression | ✅ | `mom_hmatrix.c` | ACA-based |
| Matrix-vector product | ✅ | `mom_matvec.c` | Supports H-matrix |
| Compression | ⚠️ | Framework exists | Full implementation in L4 |

## File Locations

- **L4 Interface**: `src/backend/fast_algorithms/hmatrix.h`
- **L4 Implementation**: `src/solvers/mom/mom_hmatrix.c`
- **Core Structure**: `src/core/h_matrix_compression.h`
- **Matrix-Vector Product**: `src/solvers/mom/mom_matvec.c`

## Usage

### H-Matrix Compression
```c
hmatrix_block_t* blocks = NULL;
int num_blocks = 0;
real_t tolerance = 1e-6;

// Compress matrix into H-matrix format
int status = hmatrix_compress(matrix, tolerance, &blocks, &num_blocks);
```

### H-Matrix Matrix-Vector Product
```c
mom_algorithm_t algorithm = MOM_ALGO_HMATRIX;
mom_matrix_vector_product(
    NULL,  // Dense matrix not used
    NULL,  // ACA blocks not used
    0,
    hmatrix_blocks,  // H-matrix blocks
    num_hmatrix_blocks,
    NULL,  // MLFMM tree not used
    x, y, n, algorithm, num_threads
);
```

## Core Constraints

- **Error control**: Compression respects tolerance
- **Transparent fallback**: Falls back to dense if needed
- **Operator semantics**: Results match dense (within tolerance)
- **Layer separation**: L4 implements compression, L3 defines operators
