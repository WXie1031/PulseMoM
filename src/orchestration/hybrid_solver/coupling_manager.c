/********************************************************************************
 * Coupling Manager Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file manages coupling between domains.
 * L5 layer: Execution Orchestration - manages coupling orchestration.
 *
 * Architecture Rule: L5 orchestrates coupling, does NOT implement coupling operators.
 ********************************************************************************/

#include "coupling_manager.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../operators/coupling/coupling_operator.h"
#include <stdlib.h>
#include <string.h>

// Coupling entry
typedef struct {
    int source_domain;
    int target_domain;
    coupling_operator_t* operator;
    coupling_matrix_t* coupling_matrix;
    bool is_active;
} coupling_entry_t;

// Coupling manager structure
struct coupling_manager {
    coupling_entry_t* couplings;
    int num_couplings;
    int capacity;
    
    // Convergence tracking
    real_t* previous_errors;
    int num_iterations;
    real_t convergence_tolerance;
};

coupling_manager_t* coupling_manager_create(void) {
    coupling_manager_t* manager = (coupling_manager_t*)calloc(1, sizeof(coupling_manager_t));
    if (!manager) return NULL;
    
    manager->capacity = 8;  // Initial capacity
    manager->couplings = (coupling_entry_t*)calloc(manager->capacity, sizeof(coupling_entry_t));
    if (!manager->couplings) {
        free(manager);
        return NULL;
    }
    
    manager->num_couplings = 0;
    manager->num_iterations = 0;
    manager->convergence_tolerance = 1e-6;
    manager->previous_errors = NULL;
    
    return manager;
}

void coupling_manager_destroy(coupling_manager_t* manager) {
    if (!manager) return;
    
    // Free coupling operators and matrices
    if (manager->couplings) {
        for (int i = 0; i < manager->num_couplings; i++) {
            if (manager->couplings[i].operator) {
                coupling_operator_destroy(manager->couplings[i].operator);
            }
            if (manager->couplings[i].coupling_matrix) {
                coupling_operator_destroy_matrix(manager->couplings[i].coupling_matrix);
            }
        }
        free(manager->couplings);
    }
    
    if (manager->previous_errors) {
        free(manager->previous_errors);
    }
    
    free(manager);
}

int coupling_manager_register_operator(
    coupling_manager_t* manager,
    const coupling_operator_t* operator,
    int source_domain,
    int target_domain) {
    
    if (!manager || !operator) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Check if coupling already exists
    for (int i = 0; i < manager->num_couplings; i++) {
        if (manager->couplings[i].source_domain == source_domain &&
            manager->couplings[i].target_domain == target_domain) {
            return STATUS_ERROR_INVALID_INPUT;  // Duplicate coupling
        }
    }
    
    // Resize if needed
    if (manager->num_couplings >= manager->capacity) {
        int new_capacity = manager->capacity * 2;
        coupling_entry_t* new_couplings = (coupling_entry_t*)realloc(
            manager->couplings, new_capacity * sizeof(coupling_entry_t));
        if (!new_couplings) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        manager->couplings = new_couplings;
        manager->capacity = new_capacity;
    }
    
    // Add coupling entry
    coupling_entry_t* entry = &manager->couplings[manager->num_couplings];
    entry->source_domain = source_domain;
    entry->target_domain = target_domain;
    
    // Create copy of operator (L5 layer holds reference, L3 layer owns implementation)
    entry->operator = coupling_operator_create(
        operator->coupling_type,
        source_domain,
        target_domain
    );
    if (!entry->operator) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    entry->coupling_matrix = NULL;
    entry->is_active = true;
    manager->num_couplings++;
    
    return STATUS_SUCCESS;
}

int coupling_manager_execute_transfer(
    coupling_manager_t* manager,
    int source_domain,
    int target_domain,
    const operator_vector_t* source_vector,
    operator_vector_t* target_vector) {
    
    if (!manager || !source_vector || !target_vector) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Find coupling entry
    coupling_entry_t* entry = NULL;
    for (int i = 0; i < manager->num_couplings; i++) {
        if (manager->couplings[i].source_domain == source_domain &&
            manager->couplings[i].target_domain == target_domain &&
            manager->couplings[i].is_active) {
            entry = &manager->couplings[i];
            break;
        }
    }
    
    if (!entry) {
        return STATUS_ERROR_INVALID_INPUT;  // Coupling not found
    }
    
    // Ensure coupling matrix is assembled
    if (!entry->coupling_matrix) {
        // L5 layer orchestrates coupling transfer, actual computation is in L3
        // Full implementation would:
        // 1. Identify interface points between domains
        // 2. Apply coupling operators (from L3 layer)
        // 3. Transfer data between domains
        // In full implementation, would get interface points from execution context
        return STATUS_ERROR_INVALID_STATE;
    }
    
    // L5 layer orchestrates transfer, operator is in L3
    return coupling_operator_apply(
        entry->coupling_matrix,
        source_vector,
        target_vector
    );
}

int coupling_manager_check_convergence(
    coupling_manager_t* manager,
    bool* converged,
    real_t* error) {
    
    if (!manager || !converged || !error) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L5 layer checks convergence, does NOT compute physics
    // L5 layer orchestrates convergence checking, actual residual computation is in L4
    // Convergence check:
    // Full implementation would:
    // 1. Compute interface residual
    // 2. Compare with previous iteration
    // 3. Check against tolerance
    
    *converged = false;
    *error = 0.0;
    
    // L5 layer orchestrates error computation, actual computation is in L4
    // Error computation would:
    // 1. Compute residual from interface data
    // 2. Compare against tolerance (absolute and relative)
    // 3. Return convergence status
    manager->num_iterations++;
    
    // L5 layer uses iteration count as fallback convergence criterion
    // Full implementation would use residual-based convergence from L4
    if (manager->num_iterations >= 10) {
        *converged = true;
        *error = 0.0;
    }
    
    return STATUS_SUCCESS;
}
