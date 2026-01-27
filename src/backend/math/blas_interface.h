/********************************************************************************
 * BLAS Interface (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines BLAS/LAPACK interface.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 provides math backend, not physics.
 ********************************************************************************/

#ifndef BLAS_INTERFACE_H
#define BLAS_INTERFACE_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// BLAS Backend Types
// ============================================================================

/**
 * BLAS Backend
 */
typedef enum {
    BLAS_BACKEND_NATIVE = 1,    // Native implementation
    BLAS_BACKEND_MKL = 2,       // Intel MKL
    BLAS_BACKEND_OPENBLAS = 3,  // OpenBLAS
    BLAS_BACKEND_CUDA = 4       // NVIDIA cuBLAS
} blas_backend_t;

// ============================================================================
// BLAS Interface
// ============================================================================

/**
 * Initialize BLAS backend
 */
int blas_interface_init(blas_backend_t backend);

/**
 * Finalize BLAS backend
 */
void blas_interface_finalize(void);

/**
 * BLAS GEMV: Matrix-Vector Multiply
 * y = α*A*x + β*y
 */
int blas_gemv(
    char trans,                 // 'N' or 'T'
    int m, int n,               // Matrix dimensions
    complex_t alpha,
    const complex_t* A,         // Matrix A
    int lda,                    // Leading dimension
    const complex_t* x,         // Vector x
    int incx,
    complex_t beta,
    complex_t* y,               // Vector y
    int incy
);

/**
 * BLAS GEMM: Matrix-Matrix Multiply
 * C = α*A*B + β*C
 */
int blas_gemm(
    char transa, char transb,  // Transpose flags
    int m, int n, int k,        // Matrix dimensions
    complex_t alpha,
    const complex_t* A,
    int lda,
    const complex_t* B,
    int ldb,
    complex_t beta,
    complex_t* C,
    int ldc
);

#ifdef __cplusplus
}
#endif

#endif // BLAS_INTERFACE_H
