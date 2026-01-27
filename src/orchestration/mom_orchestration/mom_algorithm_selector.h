/********************************************************************************
 * MoM Algorithm Selector (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the algorithm selection logic for MoM solver.
 * L5 layer: Execution Orchestration - WHEN and HOW to use solvers.
 *
 * Architecture Rule: L5 makes decisions about which algorithm to use,
 * but does NOT implement the algorithms themselves.
 ********************************************************************************/

#ifndef MOM_ALGORITHM_SELECTOR_H
#define MOM_ALGORITHM_SELECTOR_H

#include "../../common/types.h"
#include "../../discretization/mesh/core_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Algorithm Types (from mom_solver.h)
// ============================================================================

typedef enum {
    MOM_ALGO_BASIC = 0,      // Basic direct solver (LU)
    MOM_ALGO_ACA = 1,        // Adaptive Cross Approximation
    MOM_ALGO_MLFMM = 2,      // Multilevel Fast Multipole Method
    MOM_ALGO_HMATRIX = 3     // Hierarchical Matrix
} mom_algorithm_t;

// ============================================================================
// Problem Characteristics
// ============================================================================

/**
 * Problem characteristics for algorithm selection
 * 
 * L5 layer analyzes problem to decide which algorithm to use
 */
typedef struct {
    int num_unknowns;              // Number of unknowns
    double frequency;               // Operating frequency
    double electrical_size;         // Electrical size of problem (in wavelengths)
    bool is_wideband;               // Wideband analysis flag
    bool has_curved_surfaces;       // Curved surface modeling needed
    int num_materials;              // Number of different materials
    bool requires_high_accuracy;    // High accuracy requirements
} mom_problem_characteristics_t;

// ============================================================================
// Algorithm Selection Interface
// ============================================================================

/**
 * Select optimal MoM algorithm based on problem characteristics
 * 
 * L5 layer decision: Which algorithm should be used?
 * 
 * Selection Logic:
 * 1. Basic Direct Solver (< 1000 unknowns): Uses LU decomposition
 * 2. ACA Algorithm (1000-50000 unknowns): Uses Adaptive Cross Approximation
 * 3. MLFMM Algorithm (> 10λ electrical size): Uses Multilevel Fast Multipole
 * 4. H-Matrix Algorithm (other large problems): Uses hierarchical matrix
 * 
 * @param problem Problem characteristics
 * @return Selected algorithm type
 */
mom_algorithm_t mom_select_algorithm(const mom_problem_characteristics_t *problem);

/**
 * Compute problem characteristics from mesh and configuration
 * 
 * L5 layer analysis: What are the key characteristics of this problem?
 * 
 * @param mesh Mesh data
 * @param frequency Operating frequency
 * @param enable_wideband Wideband analysis flag
 * @param tolerance Convergence tolerance
 * @param characteristics Output: computed characteristics
 * @return 0 on success, negative on error
 */
int mom_compute_problem_characteristics(
    const mesh_t* mesh,
    double frequency,
    bool enable_wideband,
    double tolerance,
    mom_problem_characteristics_t* characteristics
);

#ifdef __cplusplus
}
#endif

#endif // MOM_ALGORITHM_SELECTOR_H
