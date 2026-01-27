/********************************************************************************
 * Solver Interface Implementation (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements the unified solver interface.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 sees matrices, not physics. No physics assumptions.
 ********************************************************************************/

#include "solver_interface.h"
#include "direct_solver.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include <stdlib.h>
#include <string.h>

// Solver interface structure
struct solver_interface {
    solver_config_t config;
    const operator_matrix_t* matrix;
    const operator_vector_t* rhs;
    operator_vector_t* solution;
    solver_statistics_t statistics;
    
    // Solver-specific data
    void* solver_data;  // Points to specific solver implementation
};

solver_interface_t* solver_interface_create(
    const solver_config_t* config) {
    
    if (!config) return NULL;
    
    solver_interface_t* solver = (solver_interface_t*)calloc(1, sizeof(solver_interface_t));
    if (!solver) return NULL;
    
    memcpy(&solver->config, config, sizeof(solver_config_t));
    
    // Initialize statistics
    memset(&solver->statistics, 0, sizeof(solver_statistics_t));
    solver->statistics.converged = false;
    
    // Create specific solver based on type
    if (config->type == SOLVER_TYPE_DENSE_LU ||
        config->type == SOLVER_TYPE_DENSE_QR ||
        config->type == SOLVER_TYPE_SPARSE_LU ||
        config->type == SOLVER_TYPE_SPARSE_CHOLESKY) {
        // Create direct solver
        direct_solver_config_t direct_config;
        direct_config.method = (config->type == SOLVER_TYPE_DENSE_LU) ? DIRECT_METHOD_LU :
                              (config->type == SOLVER_TYPE_DENSE_QR) ? DIRECT_METHOD_QR :
                              (config->type == SOLVER_TYPE_SPARSE_CHOLESKY) ? DIRECT_METHOD_CHOLESKY :
                              DIRECT_METHOD_LU;
        direct_config.backend = config->backend;
        direct_config.use_pivoting = true;
        direct_config.pivot_tolerance = 1e-12;
        direct_config.use_out_of_core = config->use_out_of_core;
        direct_config.memory_limit = config->memory_limit;
        
        solver->solver_data = direct_solver_create(&direct_config);
        if (!solver->solver_data) {
            free(solver);
            return NULL;
        }
    } else {
        // Iterative solver (to be implemented)
        solver->solver_data = NULL;
    }
    
    return solver;
}

void solver_interface_destroy(solver_interface_t* solver) {
    if (!solver) return;
    
    // Destroy specific solver
    if (solver->solver_data) {
        // For direct solver, cast and destroy
        if (solver->config.type == SOLVER_TYPE_DENSE_LU ||
            solver->config.type == SOLVER_TYPE_DENSE_QR ||
            solver->config.type == SOLVER_TYPE_SPARSE_LU ||
            solver->config.type == SOLVER_TYPE_SPARSE_CHOLESKY) {
            // Direct solver destruction handled by its own interface
            // (would need to call direct_solver_destroy if available)
        }
    }
    
    // Free solution vector if allocated
    if (solver->solution) {
        // Free solution vector (would need proper cleanup)
    }
    
    free(solver);
}

int solver_interface_set_matrix(
    solver_interface_t* solver,
    const operator_matrix_t* matrix) {
    
    if (!solver || !matrix) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    solver->matrix = matrix;
    
    // Set matrix in specific solver
    if (solver->solver_data && 
        (solver->config.type == SOLVER_TYPE_DENSE_LU ||
         solver->config.type == SOLVER_TYPE_DENSE_QR ||
         solver->config.type == SOLVER_TYPE_SPARSE_LU ||
         solver->config.type == SOLVER_TYPE_SPARSE_CHOLESKY)) {
        return direct_solver_factorize((solver_interface_t*)solver->solver_data, matrix);
    }
    
    return STATUS_SUCCESS;
}

int solver_interface_set_rhs(
    solver_interface_t* solver,
    const operator_vector_t* rhs) {
    
    if (!solver || !rhs) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    solver->rhs = rhs;
    
    return STATUS_SUCCESS;
}

int solver_interface_solve(
    solver_interface_t* solver,
    solver_result_t* result) {
    
    if (!solver || !result) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!solver->matrix || !solver->rhs) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Initialize result
    memset(result, 0, sizeof(solver_result_t));
    
    // Allocate solution vector
    result->solution = (operator_vector_t*)calloc(1, sizeof(operator_vector_t));
    if (!result->solution) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    result->solution->size = solver->rhs->size;
    result->solution->data = (complex_t*)calloc(solver->rhs->size, sizeof(complex_t));
    if (!result->solution->data) {
        free(result->solution);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Solve using specific solver
    if (solver->solver_data &&
        (solver->config.type == SOLVER_TYPE_DENSE_LU ||
         solver->config.type == SOLVER_TYPE_DENSE_QR ||
         solver->config.type == SOLVER_TYPE_SPARSE_LU ||
         solver->config.type == SOLVER_TYPE_SPARSE_CHOLESKY)) {
        
        status_t status = direct_solver_solve(
            (solver_interface_t*)solver->solver_data,
            solver->rhs,
            result->solution
        );
        
        if (status != STATUS_SUCCESS) {
            free(result->solution->data);
            free(result->solution);
            return status;
        }
        
        result->status = STATUS_SUCCESS;
        result->statistics = solver->statistics;
        result->statistics.converged = true;
        
        // Store solution in solver
        solver->solution = result->solution;
        
        return STATUS_SUCCESS;
    }
    
    // Iterative solver (to be implemented)
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

int solver_interface_get_solution(
    const solver_interface_t* solver,
    operator_vector_t* solution) {
    
    if (!solver || !solution) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!solver->solution) {
        return STATUS_ERROR_INVALID_STATE;
    }
    
    // Copy solution
    if (solution->size != solver->solution->size) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    memcpy(solution->data, solver->solution->data, 
           solver->solution->size * sizeof(complex_t));
    
    return STATUS_SUCCESS;
}

int solver_interface_get_statistics(
    const solver_interface_t* solver,
    solver_statistics_t* statistics) {
    
    if (!solver || !statistics) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    memcpy(statistics, &solver->statistics, sizeof(solver_statistics_t));
    
    return STATUS_SUCCESS;
}
