/********************************************************************************
 * PEEC Algorithm Selector Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements the algorithm selection logic for PEEC solver.
 * L5 layer: Execution Orchestration - WHEN and HOW to use solvers.
 *
 * Extracted from: solvers/peec/peec_solver_unified.c
 ********************************************************************************/

#include "peec_algorithm_selector.h"
#include <string.h>
#include <math.h>

// Geometry type constants (from peec_solver_module.h)
#define PEEC_GEOM_MANHATTAN 0

/********************************************************************************
 * Select Optimal PEEC Algorithm
 ********************************************************************************/
peec_algorithm_t peec_select_algorithm(const peec_problem_characteristics_t *problem) {
    if (!problem) return PEEC_ALGO_BASIC;
    
    if (problem->max_frequency < 1e6) {
        return PEEC_ALGO_BASIC;  // Basic for low frequency
    } else if (problem->has_skin_effect || problem->has_multi_layer) {
        return PEEC_ALGO_FULL_WAVE;  // Full-wave for complex effects
    } else if (problem->has_non_manhattan) {
        return PEEC_ALGO_ADVANCED;  // Advanced for complex geometry
    } else {
        return PEEC_ALGO_HYBRID;  // Hybrid approach
    }
}

/********************************************************************************
 * Compute Problem Characteristics
 ********************************************************************************/
int peec_compute_problem_characteristics(
    int num_conductors,
    int num_nodes,
    int num_elements,
    double frequency_stop,
    double min_feature_size,
    bool enable_skin_effect,
    int geometry_type,
    double tolerance,
    peec_problem_characteristics_t* characteristics) {
    
    if (!characteristics) return -1;
    
    // Initialize
    memset(characteristics, 0, sizeof(peec_problem_characteristics_t));
    
    // Set basic counts
    characteristics->num_conductors = num_conductors;
    characteristics->num_nodes = num_nodes;
    characteristics->num_elements = num_elements;
    characteristics->max_frequency = frequency_stop;
    
    // Set minimum feature size
    characteristics->min_feature_size = min_feature_size;
    if (characteristics->min_feature_size <= 0.0) {
        characteristics->min_feature_size = 1e-3;  // 1mm default
    }
    
    // Set effect flags
    characteristics->has_skin_effect = enable_skin_effect;
    characteristics->has_multi_layer = (num_conductors > 1);
    characteristics->has_non_manhattan = (geometry_type != PEEC_GEOM_MANHATTAN);
    
    // High accuracy requirement
    characteristics->requires_high_accuracy = (tolerance < 1e-6);
    
    return 0;
}
