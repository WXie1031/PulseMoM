/********************************************************************************
 * GPU Kernels (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines GPU kernel interfaces.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: GPU kernels operate on abstract operators, not physics.
 * NO physics-specific branching in GPU kernels.
 ********************************************************************************/

#ifndef GPU_KERNELS_H
#define GPU_KERNELS_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cuComplex.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GPU Kernel Launch Configuration
// ============================================================================

/**
 * GPU Kernel Launch Parameters
 */
typedef struct {
    int grid_dim_x, grid_dim_y, grid_dim_z;
    int block_dim_x, block_dim_y, block_dim_z;
    size_t shared_memory_size;
    int device_id;
} gpu_launch_config_t;

// ============================================================================
// GPU Kernel Interfaces
// ============================================================================

/**
 * GPU Matrix-Vector Multiply Kernel
 * 
 * L4 layer: Generic matvec kernel, no physics assumptions
 */
int gpu_kernel_matrix_vector_multiply(
    const complex_t* A,         // Matrix (device)
    const complex_t* x,         // Vector (device)
    complex_t* y,               // Result (device)
    int m, int n,               // Matrix dimensions
    const gpu_launch_config_t* config
);

/**
 * GPU Matrix Assembly Kernel
 * 
 * L4 layer: Generic assembly kernel, receives operator from L3
 */
int gpu_kernel_matrix_assembly(
    const real_t* coordinates,  // Coordinates (device)
    const real_t* parameters,   // Operator parameters (device)
    complex_t* matrix,          // Output matrix (device)
    int n_rows, int n_cols,
    const gpu_launch_config_t* config
);

/**
 * GPU Iterative Solver Kernel (GMRES)
 * 
 * L4 layer: Generic iterative solver kernel
 */
int gpu_kernel_gmres_iteration(
    const complex_t* A,         // Matrix (device)
    const complex_t* b,         // RHS vector (device)
    complex_t* x,               // Solution vector (device)
    int n,                      // Matrix size
    int iteration,
    real_t tolerance,
    real_t* residual_norm,      // Output residual
    bool* converged,            // Output convergence flag
    const gpu_launch_config_t* config
);

#ifdef __cplusplus
}
#endif

#endif // GPU_KERNELS_H
