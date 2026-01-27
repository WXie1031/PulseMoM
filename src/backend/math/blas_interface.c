/********************************************************************************
 * BLAS Interface Implementation (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements BLAS/LAPACK interface.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 provides math backend, not physics.
 ********************************************************************************/

#include "blas_interface.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// OpenBLAS support
#ifdef ENABLE_OPENBLAS
// Include stdint.h first to avoid type redefinition conflicts with MSVC
#include <stdint.h>
// Define HAVE_LAPACK_CONFIG_H to use lapacke_config.h which properly handles MSVC
#ifndef HAVE_LAPACK_CONFIG_H
#define HAVE_LAPACK_CONFIG_H
#endif
// Define LAPACK_COMPLEX_STRUCTURE for MSVC compatibility (if not using C++ complex)
#if defined(_MSC_VER) && !defined(__INTEL_CLANG_COMPILER) && !defined(LAPACK_COMPLEX_CPP)
#ifndef LAPACK_COMPLEX_STRUCTURE
#define LAPACK_COMPLEX_STRUCTURE
#endif
#endif
#include <cblas.h>
// Only include lapacke.h if LAPACK functions are actually needed
// For now, we only use BLAS functions, so comment out lapacke.h to avoid conflicts
// #include <lapacke.h>
#endif

// HDF5 support (for future use)
#ifdef ENABLE_HDF5
#include <hdf5.h>
#endif

// BLAS backend state
static blas_backend_t current_backend = BLAS_BACKEND_NATIVE;
static bool is_initialized = false;

int blas_interface_init(blas_backend_t backend) {
    current_backend = backend;
    is_initialized = true;
    
    // Initialize backend-specific resources
    switch (backend) {
        case BLAS_BACKEND_MKL:
            // MKL initialization (if available)
            #ifdef ENABLE_MKL
            // mkl_set_num_threads(...);
            #endif
            break;
            
        case BLAS_BACKEND_OPENBLAS:
            // OpenBLAS initialization (if available)
            #ifdef ENABLE_OPENBLAS
            // Set number of threads for OpenBLAS
            // Use OpenMP thread count if available, otherwise use 1
            #ifdef _OPENMP
            int num_threads = omp_get_max_threads();
            openblas_set_num_threads(num_threads);
            #else
            openblas_set_num_threads(1);
            #endif
            #endif
            break;
            
        case BLAS_BACKEND_CUDA:
            // cuBLAS initialization (if available)
            #ifdef ENABLE_CUDA
            // cublasCreate(...);
            #endif
            break;
            
        case BLAS_BACKEND_NATIVE:
        default:
            // Native implementation - no initialization needed
            break;
    }
    
    return STATUS_SUCCESS;
}

void blas_interface_finalize(void) {
    // Finalize backend-specific resources
    switch (current_backend) {
        case BLAS_BACKEND_CUDA:
            #ifdef ENABLE_CUDA
            // cublasDestroy(...);
            #endif
            break;
            
        default:
            // No cleanup needed
            break;
    }
    
    is_initialized = false;
}

// Native GEMV implementation
static int native_gemv(
    char trans,
    int m, int n,
    complex_t alpha,
    const complex_t* A,
    int lda,
    const complex_t* x,
    int incx,
    complex_t beta,
    complex_t* y,
    int incy) {
    
    if (!A || !x || !y) return STATUS_ERROR_INVALID_INPUT;
    
    bool transpose = (trans == 'T' || trans == 't');
    
    // Scale y by beta
    if (beta.re != 1.0 || beta.im != 0.0) {
        for (int i = 0; i < m; i++) {
            int y_idx = i * incy;
            #if defined(_MSC_VER)
            complex_t prod;
            prod.re = beta.re * y[y_idx].re - beta.im * y[y_idx].im;
            prod.im = beta.re * y[y_idx].im + beta.im * y[y_idx].re;
            y[y_idx] = prod;
            #else
            y[y_idx] = beta * y[y_idx];
            #endif
        }
    }
    
    // Compute y = alpha * A * x + beta * y
    if (transpose) {
        // y = alpha * A^T * x
        int i;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif
        for (i = 0; i < n; i++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            for (int j = 0; j < m; j++) {
                int A_idx = j * lda + i;
                int x_idx = j * incx;
                
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = A[A_idx].re * x[x_idx].re - A[A_idx].im * x[x_idx].im;
                prod.im = A[A_idx].re * x[x_idx].im + A[A_idx].im * x[x_idx].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                sum += A[A_idx] * x[x_idx];
                #endif
            }
            
            int y_idx = i * incy;
            #if defined(_MSC_VER)
            complex_t alpha_sum;
            alpha_sum.re = alpha.re * sum.re - alpha.im * sum.im;
            alpha_sum.im = alpha.re * sum.im + alpha.im * sum.re;
            y[y_idx].re += alpha_sum.re;
            y[y_idx].im += alpha_sum.im;
            #else
            y[y_idx] += alpha * sum;
            #endif
        }
    } else {
        // y = alpha * A * x
        int i;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif
        for (i = 0; i < m; i++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            for (int j = 0; j < n; j++) {
                int A_idx = i * lda + j;
                int x_idx = j * incx;
                
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = A[A_idx].re * x[x_idx].re - A[A_idx].im * x[x_idx].im;
                prod.im = A[A_idx].re * x[x_idx].im + A[A_idx].im * x[x_idx].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                sum += A[A_idx] * x[x_idx];
                #endif
            }
            
            int y_idx = i * incy;
            #if defined(_MSC_VER)
            complex_t alpha_sum;
            alpha_sum.re = alpha.re * sum.re - alpha.im * sum.im;
            alpha_sum.im = alpha.re * sum.im + alpha.im * sum.re;
            y[y_idx].re += alpha_sum.re;
            y[y_idx].im += alpha_sum.im;
            #else
            y[y_idx] += alpha * sum;
            #endif
        }
    }
    
    return STATUS_SUCCESS;
}

int blas_gemv(
    char trans,
    int m, int n,
    complex_t alpha,
    const complex_t* A,
    int lda,
    const complex_t* x,
    int incx,
    complex_t beta,
    complex_t* y,
    int incy) {
    
    if (!is_initialized) {
        blas_interface_init(BLAS_BACKEND_NATIVE);
    }
    
    // Route to appropriate backend
    switch (current_backend) {
        case BLAS_BACKEND_MKL:
            #ifdef ENABLE_MKL
            // Call MKL cblas_zgemv
            // return mkl_gemv(...);
            #endif
            // Fall through to native
            break;
            
        case BLAS_BACKEND_OPENBLAS:
            #ifdef ENABLE_OPENBLAS
            // Call OpenBLAS cblas_zgemv
            // Note: OpenBLAS uses double complex, we need to convert
            // For now, fall through to native implementation
            // Full implementation would:
            // 1. Convert complex_t to double complex
            // 2. Call cblas_zgemv with appropriate parameters
            // 3. Convert result back to complex_t
            // This requires careful handling of memory layout and types
            #endif
            // Fall through to native
            break;
            
        case BLAS_BACKEND_CUDA:
            #ifdef ENABLE_CUDA
            // Call cuBLAS cublasZgemv
            // return cublas_gemv(...);
            #endif
            // Fall through to native
            break;
            
        case BLAS_BACKEND_NATIVE:
        default:
            return native_gemv(trans, m, n, alpha, A, lda, x, incx, beta, y, incy);
    }
    
    // Fallback to native
    return native_gemv(trans, m, n, alpha, A, lda, x, incx, beta, y, incy);
}

// Native GEMM implementation
static int native_gemm(
    char transa, char transb,
    int m, int n, int k,
    complex_t alpha,
    const complex_t* A,
    int lda,
    const complex_t* B,
    int ldb,
    complex_t beta,
    complex_t* C,
    int ldc) {
    
    if (!A || !B || !C) return STATUS_ERROR_INVALID_INPUT;
    
    bool transpose_a = (transa == 'T' || transa == 't');
    bool transpose_b = (transb == 'T' || transb == 't');
    
    // Scale C by beta
    if (beta.re != 1.0 || beta.im != 0.0) {
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                int C_idx = i * ldc + j;
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = beta.re * C[C_idx].re - beta.im * C[C_idx].im;
                prod.im = beta.re * C[C_idx].im + beta.im * C[C_idx].re;
                C[C_idx] = prod;
                #else
                C[C_idx] = beta * C[C_idx];
                #endif
            }
        }
    }
    
    // Compute C = alpha * A * B + beta * C
    int i;
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            for (int l = 0; l < k; l++) {
                int A_idx = transpose_a ? (l * lda + i) : (i * lda + l);
                int B_idx = transpose_b ? (j * ldb + l) : (l * ldb + j);
                
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = A[A_idx].re * B[B_idx].re - A[A_idx].im * B[B_idx].im;
                prod.im = A[A_idx].re * B[B_idx].im + A[A_idx].im * B[B_idx].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                sum += A[A_idx] * B[B_idx];
                #endif
            }
            
            int C_idx = i * ldc + j;
            #if defined(_MSC_VER)
            complex_t alpha_sum;
            alpha_sum.re = alpha.re * sum.re - alpha.im * sum.im;
            alpha_sum.im = alpha.re * sum.im + alpha.im * sum.re;
            C[C_idx].re += alpha_sum.re;
            C[C_idx].im += alpha_sum.im;
            #else
            C[C_idx] += alpha * sum;
            #endif
        }
    }
    
    return STATUS_SUCCESS;
}

int blas_gemm(
    char transa, char transb,
    int m, int n, int k,
    complex_t alpha,
    const complex_t* A,
    int lda,
    const complex_t* B,
    int ldb,
    complex_t beta,
    complex_t* C,
    int ldc) {
    
    if (!is_initialized) {
        blas_interface_init(BLAS_BACKEND_NATIVE);
    }
    
    // Route to appropriate backend
    switch (current_backend) {
        case BLAS_BACKEND_MKL:
            #ifdef ENABLE_MKL
            // Call MKL cblas_zgemm
            // return mkl_gemm(...);
            #endif
            break;
            
        case BLAS_BACKEND_OPENBLAS:
            #ifdef ENABLE_OPENBLAS
            // Call OpenBLAS cblas_zgemm
            // return openblas_gemm(...);
            #endif
            break;
            
        case BLAS_BACKEND_CUDA:
            #ifdef ENABLE_CUDA
            // Call cuBLAS cublasZgemm
            // return cublas_gemm(...);
            #endif
            break;
            
        case BLAS_BACKEND_NATIVE:
        default:
            return native_gemm(transa, transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
    }
    
    // Fallback to native
    return native_gemm(transa, transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
}
