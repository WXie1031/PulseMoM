/*****************************************************************************************
 * Core Solver Implementation - Unified linear solver interface
 * 
 * Supports dense LU, sparse LU, iterative methods, and circuit solvers
 * Includes preconditioning and eigenvalue solving capabilities
 * Provides GPU acceleration and parallel processing
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "core_common.h"
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#endif

#include "core_solver.h"
#include "h_matrix_compression.h"

// Define solver types for electromagnetic kernels compatibility
#ifdef BUILDING_ELECTROMAGNETIC_KERNELS_DLL
#define SOLVER_DENSE_LU      SOLVER_TYPE_DENSE_LU
#define SOLVER_SPARSE_LU     SOLVER_TYPE_SPARSE_LU
#define SOLVER_GMRES         SOLVER_TYPE_ITERATIVE_GMRES
#define SOLVER_CG            SOLVER_TYPE_ITERATIVE_CG
#define SOLVER_BICGSTAB      SOLVER_TYPE_ITERATIVE_BICGSTAB
#endif

// Rest of the original file content would go here...