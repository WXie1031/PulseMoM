/********************************************************************************
 * Circuit Coupling Orchestration (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the orchestration logic for circuit-electromagnetic co-simulation.
 * L5 layer: Execution Orchestration - WHEN and HOW to couple circuit and EM solvers.
 *
 * Extracted from: physics/peec/circuit_coupling_simulation.c
 * 
 * Architecture Rule: L5 orchestrates the coupling flow, but does NOT define
 * the physical equations (which remain in L1 physics layer).
 ********************************************************************************/

#ifndef CIRCUIT_COUPLING_ORCHESTRATION_H
#define CIRCUIT_COUPLING_ORCHESTRATION_H

#include "../../physics/peec/peec_circuit_coupling.h"  // L1: Physical definitions
#include "../../solvers/mom/mom_solver.h"
#include "../../backend/solvers/sparse_direct_solver.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Circuit Coupling Orchestrator
// ============================================================================

/**
 * Circuit Coupling Orchestrator
 * 
 * L5 layer: Orchestrates the coupling flow between circuit and EM solvers.
 * This includes:
 * - MNA matrix building (numerical implementation)
 * - Nonlinear solver coordination (numerical implementation)
 * - Time stepping logic (orchestration)
 * - Coupling flow control (orchestration)
 */
typedef struct CircuitCouplingOrchestrator {
    // Circuit data (from L1 physics definitions)
    void* circuit_data;  // Circuit structure from physics layer
    
    // MNA matrix (numerical implementation)
    void* mna_matrix;    // MNAMatrix* (internal structure)
    
    // EM coupling data
    void* em_model;      // PCBEMModel* (from physics layer)
    
    // Orchestration state
    double frequency;
    bool is_nonlinear;
    double convergence_tolerance;
    int max_iterations;
} CircuitCouplingOrchestrator;

// ============================================================================
// Orchestration Functions
// ============================================================================

/**
 * Create circuit coupling orchestrator
 * 
 * @return Orchestrator instance, or NULL on error
 */
CircuitCouplingOrchestrator* circuit_coupling_orchestrator_create(void);

/**
 * Destroy circuit coupling orchestrator
 * 
 * @param orchestrator Orchestrator to destroy
 */
void circuit_coupling_orchestrator_destroy(CircuitCouplingOrchestrator* orchestrator);

/**
 * Build MNA matrix (numerical implementation)
 * 
 * L5 layer: Numerical implementation of MNA matrix building.
 * The physical equations are defined in L1 physics layer.
 * 
 * @param orchestrator Orchestrator instance
 * @return 0 on success, negative on error
 */
int circuit_coupling_build_mna_matrix(CircuitCouplingOrchestrator* orchestrator);

/**
 * Solve nonlinear circuit (numerical implementation)
 * 
 * L5 layer: Numerical implementation of nonlinear solver.
 * Uses Newton-Raphson or homotopy continuation methods.
 * 
 * @param orchestrator Orchestrator instance
 * @return 0 on success, negative on error
 */
int circuit_coupling_solve_nonlinear(CircuitCouplingOrchestrator* orchestrator);

/**
 * Perform time-domain coupling simulation (orchestration)
 * 
 * L5 layer: Orchestrates the time-stepping and coupling flow.
 * 
 * @param orchestrator Orchestrator instance
 * @param time_start Start time
 * @param time_stop Stop time
 * @param time_step Time step
 * @return 0 on success, negative on error
 */
int circuit_coupling_solve_time_domain(
    CircuitCouplingOrchestrator* orchestrator,
    double time_start,
    double time_stop,
    double time_step
);

/**
 * Extract EM coupling (orchestration)
 * 
 * L5 layer: Orchestrates the extraction of EM coupling parameters
 * from the EM solver and injection into the circuit solver.
 * 
 * @param orchestrator Orchestrator instance
 * @param em_model EM model (from physics layer)
 * @return 0 on success, negative on error
 */
int circuit_coupling_extract_em_coupling(
    CircuitCouplingOrchestrator* orchestrator,
    void* em_model
);

#ifdef __cplusplus
}
#endif

#endif // CIRCUIT_COUPLING_ORCHESTRATION_H
