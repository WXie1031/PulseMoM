/**
 * @file electromagnetic_kernels_solver_compat.h
 * @brief Compatibility definitions for electromagnetic kernels solver
 * @details Provides solver type definitions for electromagnetic kernels library
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef ELECTROMAGNETIC_KERNELS_SOLVER_COMPAT_H
#define ELECTROMAGNETIC_KERNELS_SOLVER_COMPAT_H

#ifdef BUILDING_ELECTROMAGNETIC_KERNELS_DLL

// Define solver types for electromagnetic kernels library compatibility
#define SOLVER_DENSE_LU      SOLVER_TYPE_DENSE_LU
#define SOLVER_SPARSE_LU     SOLVER_TYPE_SPARSE_LU
#define SOLVER_GMRES         SOLVER_TYPE_ITERATIVE_GMRES
#define SOLVER_CG            SOLVER_TYPE_ITERATIVE_CG
#define SOLVER_BICGSTAB      SOLVER_TYPE_ITERATIVE_BICGSTAB

// Simplified solver functions for electromagnetic kernels library
static inline int solve_dense_lu_electromagnetic_kernels(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    printf("Electromagnetic kernels: Dense LU solver placeholder\n");
    // Simple copy for testing
    memcpy(solution, rhs, solver->matrix_size * sizeof(complex_t));
    return 0;
}

static inline int solve_sparse_lu_electromagnetic_kernels(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    printf("Electromagnetic kernels: Sparse LU solver placeholder\n");
    memcpy(solution, rhs, solver->matrix_size * sizeof(complex_t));
    return 0;
}

static inline int solve_gmres_electromagnetic_kernels(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    printf("Electromagnetic kernels: GMRES solver placeholder\n");
    memcpy(solution, rhs, solver->matrix_size * sizeof(complex_t));
    return 0;
}

static inline int solve_cg_electromagnetic_kernels(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    printf("Electromagnetic kernels: CG solver placeholder\n");
    memcpy(solution, rhs, solver->matrix_size * sizeof(complex_t));
    return 0;
}

static inline int solve_bicgstab_electromagnetic_kernels(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    printf("Electromagnetic kernels: BiCGSTAB solver placeholder\n");
    memcpy(solution, rhs, solver->matrix_size * sizeof(complex_t));
    return 0;
}

#endif // BUILDING_ELECTROMAGNETIC_KERNELS_DLL

#endif // ELECTROMAGNETIC_KERNELS_SOLVER_COMPAT_H