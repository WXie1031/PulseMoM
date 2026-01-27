/********************************************************************************
 * RWG Basis Functions (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines RWG (Rao-Wilton-Glisson) basis functions for discretization.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 *
 * Architecture Rule: L2 layer defines basis functions, not physics or operators.
 ********************************************************************************/

#ifndef RWG_BASIS_H
#define RWG_BASIS_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../mesh/mesh_engine.h"  // Using discretization/mesh (L2 layer)

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// RWG Basis Function Definition
// ============================================================================

/**
 * RWG Basis Function
 * 
 * L2 layer defines the basis function structure for discretization.
 * Physics interpretation is in L1, operator evaluation is in L3.
 */
typedef struct {
    int id;                        // Basis function index
    int triangle_plus;             // Positive triangle index
    int triangle_minus;            // Negative triangle index
    int edge_index;                // Common edge index
    
    // Geometric properties
    real_t edge_length;            // Edge length
    real_t area_plus;              // Area of positive triangle
    real_t area_minus;             // Area of negative triangle
    geom_point_t centroid_plus;     // Centroid of positive triangle
    geom_point_t centroid_minus;   // Centroid of negative triangle
    geom_point_t edge_vector;      // Edge vector (from minus to plus)
    
    // Support region
    geom_point_t support_center;  // Center of support region
    real_t support_radius;         // Radius of support region
} rwg_basis_t;

/**
 * RWG Basis Function Set
 */
typedef struct {
    rwg_basis_t* basis_functions;
    int num_basis_functions;
    
    // Connectivity
    int* edge_to_basis;           // Edge index to basis function index
    int* triangle_to_basis;        // Triangle to basis functions (multiple)
    int num_edges;
    int num_triangles;
} rwg_basis_set_t;

// ============================================================================
// RWG Basis Function Interface
// ============================================================================

/**
 * Create RWG basis functions from mesh
 * 
 * L2 layer creates basis functions from mesh topology.
 * Physics interpretation is in L1, evaluation is in L3.
 */
int rwg_basis_create(
    const mesh_t* mesh,
    rwg_basis_set_t* basis_set
);

/**
 * Destroy RWG basis function set
 */
void rwg_basis_destroy(rwg_basis_set_t* basis_set);

/**
 * Get basis function by index
 */
const rwg_basis_t* rwg_basis_get(
    const rwg_basis_set_t* basis_set,
    int index
);

/**
 * Get basis functions on triangle
 * 
 * L2 layer provides topological queries, not physics
 */
int rwg_basis_get_on_triangle(
    const rwg_basis_set_t* basis_set,
    int triangle_id,
    int* basis_indices,
    int max_basis
);

/**
 * Get basis function on edge
 */
int rwg_basis_get_on_edge(
    const rwg_basis_set_t* basis_set,
    int edge_id
);

/**
 * Validate basis function set
 * 
 * L2 layer checks topological consistency, not physics
 */
bool rwg_basis_validate(const rwg_basis_set_t* basis_set);

#ifdef __cplusplus
}
#endif

#endif // RWG_BASIS_H
