/********************************************************************************
 * PEEC Algorithm Selector (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the algorithm selection logic for PEEC solver.
 * L5 layer: Execution Orchestration - WHEN and HOW to use solvers.
 *
 * Architecture Rule: L5 makes decisions about which algorithm to use,
 * but does NOT implement the algorithms themselves.
 ********************************************************************************/

#ifndef PEEC_ALGORITHM_SELECTOR_H
#define PEEC_ALGORITHM_SELECTOR_H

#include "../../common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Algorithm Types
// ============================================================================

typedef enum {
    PEEC_ALGO_BASIC = 0,      // Basic partial element extraction
    PEEC_ALGO_ADVANCED = 1,   // Advanced (non-Manhattan geometry)
    PEEC_ALGO_FULL_WAVE = 2,  // Full-wave (skin effect, multi-layer)
    PEEC_ALGO_HYBRID = 3      // Hybrid approach
} peec_algorithm_t;

// ============================================================================
// Problem Characteristics
// ============================================================================

/**
 * Problem characteristics for algorithm selection
 * 
 * L5 layer analyzes problem to decide which algorithm to use
 */
typedef struct {
    int num_conductors;              // Number of conductors
    int num_nodes;                   // Number of circuit nodes
    int num_elements;                 // Number of circuit elements
    double max_frequency;             // Maximum frequency (Hz)
    double min_feature_size;          // Minimum feature size (m)
    bool has_skin_effect;             // Skin effect modeling needed
    bool has_multi_layer;             // Multi-layer structure
    bool has_non_manhattan;           // Non-Manhattan geometry
    bool requires_high_accuracy;      // High accuracy requirements
} peec_problem_characteristics_t;

// ============================================================================
// Algorithm Selection Interface
// ============================================================================

/**
 * Select optimal PEEC algorithm based on problem characteristics
 * 
 * L5 layer decision: Which algorithm should be used?
 * 
 * Selection Logic:
 * 1. Basic Algorithm (< 1 MHz): Simplified partial element extraction
 * 2. Full-Wave Algorithm (skin/multi-layer effects): Complete Green's function
 * 3. Advanced Algorithm (non-Manhattan geometry): Complex geometry handling
 * 4. Hybrid Algorithm (general cases): Combines multiple approaches
 * 
 * @param problem Problem characteristics
 * @return Selected algorithm type
 */
peec_algorithm_t peec_select_algorithm(const peec_problem_characteristics_t *problem);

/**
 * Compute problem characteristics from circuit data and configuration
 * 
 * L5 layer analysis: What are the key characteristics of this problem?
 * 
 * @param num_conductors Number of conductors
 * @param num_nodes Number of circuit nodes
 * @param num_elements Number of circuit elements
 * @param frequency_stop Maximum frequency
 * @param min_feature_size Minimum feature size
 * @param enable_skin_effect Skin effect flag
 * @param geometry_type Geometry type (Manhattan vs non-Manhattan)
 * @param tolerance Convergence tolerance
 * @param characteristics Output: computed characteristics
 * @return 0 on success, negative on error
 */
int peec_compute_problem_characteristics(
    int num_conductors,
    int num_nodes,
    int num_elements,
    double frequency_stop,
    double min_feature_size,
    bool enable_skin_effect,
    int geometry_type,  // PEEC_GEOM_MANHATTAN = 0, others = non-Manhattan
    double tolerance,
    peec_problem_characteristics_t* characteristics
);

#ifdef __cplusplus
}
#endif

#endif // PEEC_ALGORITHM_SELECTOR_H
