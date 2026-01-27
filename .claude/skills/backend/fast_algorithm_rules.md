# Fast Algorithm Rules

## Scope
Fast algorithms for matrix-vector products (FMM, MLFMM, ACA, H-matrix). These belong to **L4 Numerical Backend Layer**.

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ Implements fast algorithms for acceleration
- ✅ Reduces complexity from O(N²) to O(N log N) or O(N)
- ✅ Preserves operator semantics
- ✅ Provides error-controlled approximation
- ❌ Does NOT interpret physics
- ❌ Does NOT make solver decisions

## Core Constraints
- **Acceleration only**: Fast algorithms are performance optimizations
- **Operator semantics preserved**: Results match standard methods (within tolerance)
- **Error control**: Approximation respects specified tolerance
- **Transparent fallback**: Falls back to standard methods if needed

## Supported Fast Algorithms

### 1. ACA (Adaptive Cross Approximation)
- **Status**: ✅ Implemented
- **Location**: `src/backend/fast_algorithms/aca.h`, `src/solvers/mom/mom_aca.c`
- **Complexity**: O(N log N) for low-rank matrices
- **Features**:
  - Low-rank matrix approximation
  - Error-controlled compression
  - Iterative rank determination
  - Partial pivoting support

### 2. H-Matrix (Hierarchical Matrix)
- **Status**: ✅ Framework implemented
- **Location**: `src/core/h_matrix_compression.h`, `src/solvers/mom/mom_hmatrix.c`
- **Complexity**: O(N log N)
- **Features**:
  - Hierarchical block structure
  - Admissibility condition
  - Low-rank block compression
  - Block cluster tree

### 3. MLFMM (Multilevel Fast Multipole Method)
- **Status**: ✅ Framework implemented
- **Location**: `src/solvers/mom/mom_mlfmm.c`
- **Complexity**: O(N)
- **Features**:
  - Octree decomposition
  - Multipole expansion
  - M2M, M2L, L2L translations
  - Near-field / far-field separation

### 4. FMM (Fast Multipole Method)
- **Status**: ⚠️ Basic framework
- **Location**: `src/solvers/mom/mom_mlfmm.c`
- **Complexity**: O(N log N)
- **Note**: Single-level version of MLFMM

## Implementation Status

| Algorithm | Interface | Implementation | Status | Notes |
|-----------|-----------|----------------|--------|-------|
| ACA | ✅ | ✅ | Complete | Full implementation |
| H-Matrix | ✅ | ⚠️ | Framework | Block structure defined |
| MLFMM | ✅ | ⚠️ | Framework | Octree and translations |
| FMM | ✅ | ⚠️ | Basic | Single-level version |

## File Locations

- **ACA**: `src/backend/fast_algorithms/aca.h`, `src/solvers/mom/mom_aca.c`
- **H-Matrix**: `src/core/h_matrix_compression.h`, `src/solvers/mom/mom_hmatrix.c`
- **MLFMM**: `src/solvers/mom/mom_mlfmm.c`
- **Unified Interface**: `src/solvers/mom/mom_matvec.h`

## Usage

### Select Fast Algorithm
```c
mom_algorithm_t algorithm = MOM_ALGO_ACA;  // or HMATRIX, MLFMM

mom_matrix_vector_product(
    Z,           // Dense matrix (if algorithm is BASIC)
    aca_blocks,  // ACA blocks (if algorithm is ACA)
    num_aca_blocks,
    hmatrix_blocks,  // H-matrix blocks (if algorithm is HMATRIX)
    num_hmatrix_blocks,
    mlfmm_tree,  // MLFMM tree (if algorithm is MLFMM)
    x, y, n, algorithm, num_threads
);
```

### ACA Compression
```c
aca_params_t params;
params.tolerance = 1e-6;
params.max_rank = 100;
params.use_partial_pivoting = true;

aca_result_t result;
int status = aca_compress(matrix, &params, &result);
```

## Core Constraints

- **Acceleration only**: Performance optimization, not physics change
- **Operator semantics**: Results match standard methods
- **Error control**: Respects tolerance
- **Transparent fallback**: Falls back if compression fails
- **Layer separation**: L4 implements algorithms, L3 defines operators
