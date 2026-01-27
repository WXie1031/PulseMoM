/********************************************************************************
 * GPU Linear Algebra (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines GPU linear algebra operations.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: GPU code must not assume MoM/PEEC/MTL semantics.
 * GPU kernels operate on abstract operators or matrices.
 ********************************************************************************/

#ifndef GPU_LINEAR_ALGEBRA_H
#define GPU_LINEAR_ALGEBRA_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cuComplex.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GPU Memory Management
// ============================================================================

/**
 * GPU Memory Handle
 * 
 * L4 layer manages GPU memory, not physics data
 */
typedef struct {
    void* device_ptr;       // Device memory pointer
    size_t size;            // Memory size in bytes
    int device_id;          // GPU device ID
} gpu_memory_t;

/**
 * Allocate GPU memory
 */
int gpu_memory_allocate(
    gpu_memory_t* mem,
    size_t size,
    int device_id
);

/**
 * Free GPU memory
 */
void gpu_memory_free(gpu_memory_t* mem);

/**
 * Copy data to GPU
 */
int gpu_memory_copy_to_device(
    const void* host_data,
    gpu_memory_t* device_mem,
    size_t size
);

/**
 * Copy data from GPU
 */
int gpu_memory_copy_from_device(
    const gpu_memory_t* device_mem,
    void* host_data,
    size_t size
);

// ============================================================================
// GPU Linear Algebra Operations
// ============================================================================

/**
 * GPU Matrix-Vector Multiplication
 * 
 * L4 layer provides GPU matvec, no physics assumptions
 */
int gpu_matrix_vector_multiply(
    const complex_t* A,         // Matrix A (host)
    const complex_t* x,         // Vector x (host)
    complex_t* y,               // Result y = A*x (host)
    int m, int n,               // Matrix dimensions
    int device_id
);

/**
 * GPU Matrix-Matrix Multiplication
 */
int gpu_matrix_matrix_multiply(
    const complex_t* A,
    const complex_t* B,
    complex_t* C,
    int m, int n, int k,        // Matrix dimensions
    int device_id
);

/**
 * GPU BLAS Interface
 * 
 * L4 layer provides GPU BLAS operations
 */
typedef struct {
    void* cublas_handle;        // cuBLAS handle
    int device_id;
} gpu_blas_handle_t;

/**
 * Initialize GPU BLAS
 */
int gpu_blas_init(gpu_blas_handle_t* handle, int device_id);

/**
 * Destroy GPU BLAS
 */
void gpu_blas_destroy(gpu_blas_handle_t* handle);

/**
 * GPU BLAS matrix-vector multiply
 */
int gpu_blas_gemv(
    gpu_blas_handle_t* handle,
    const complex_t* A,
    const complex_t* x,
    complex_t* y,
    int m, int n
);

#ifdef __cplusplus
}
#endif

#endif // GPU_LINEAR_ALGEBRA_H
