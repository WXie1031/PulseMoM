/********************************************************************************
 * Coupling Manager (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file manages coupling between domains.
 * L5 layer: Execution Orchestration - manages coupling orchestration.
 *
 * Architecture Rule: L5 orchestrates coupling, does NOT implement coupling operators.
 ********************************************************************************/

#ifndef COUPLING_MANAGER_H
#define COUPLING_MANAGER_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../physics/hybrid/hybrid_physics_boundary.h"
#include "../../operators/coupling/coupling_operator.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Coupling Manager
 * 
 * L5 layer manages coupling orchestration
 */
typedef struct coupling_manager coupling_manager_t;

/**
 * Create coupling manager
 */
coupling_manager_t* coupling_manager_create(void);

/**
 * Destroy coupling manager
 */
void coupling_manager_destroy(coupling_manager_t* manager);

/**
 * Register coupling operator
 * 
 * L5 layer registers operators from L3, does NOT implement them
 */
int coupling_manager_register_operator(
    coupling_manager_t* manager,
    const coupling_operator_t* operator,
    int source_domain,
    int target_domain
);

/**
 * Execute coupling transfer
 * 
 * L5 layer orchestrates transfer, operator is in L3
 */
int coupling_manager_execute_transfer(
    coupling_manager_t* manager,
    int source_domain,
    int target_domain,
    const operator_vector_t* source_vector,
    operator_vector_t* target_vector
);

/**
 * Check coupling convergence
 */
int coupling_manager_check_convergence(
    coupling_manager_t* manager,
    bool* converged,
    real_t* error
);

#ifdef __cplusplus
}
#endif

#endif // COUPLING_MANAGER_H
