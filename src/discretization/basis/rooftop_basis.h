/********************************************************************************
 * Rooftop Basis Functions (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines rooftop basis functions for discretization.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 *
 * Architecture Rule: L2 layer defines basis functions, not physics or operators.
 ********************************************************************************/

#ifndef ROOFTOP_BASIS_H
#define ROOFTOP_BASIS_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../mesh/mesh_engine.h"  // Using discretization/mesh (L2 layer)

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Rooftop Basis Function Definition
// ============================================================================

/**
 * Rooftop Grid
 * 
 * L2 layer defines the grid structure for rooftop basis functions
 */
typedef struct {
    real_t x_min, x_max;           // Bounding box
    real_t y_min, y_max;
    real_t z;                       // Layer z-coordinate
    int layer;                     // Layer index
    
    // Discretization
    int nx, ny;                    // Grid dimensions
    real_t dx, dy;                 // Cell sizes
} rooftop_grid_t;

/**
 * Rooftop Basis Function
 * 
 * L2 layer defines the basis function structure.
 * Physics interpretation is in L1, operator evaluation is in L3.
 */
typedef struct {
    int id;                        // Basis function index
    int cell_index;                // Cell index in grid
    int direction;                 // 0: x-direction, 1: y-direction
    
    // Geometric properties
    rooftop_grid_t* grid;          // Parent grid
    real_t cell_area;              // Cell area
    geom_point_t cell_center;      // Cell center
    
    // Support region
    geom_point_t support_min;      // Support region minimum point
    geom_point_t support_max;      // Support region maximum point
} rooftop_basis_t;

/**
 * Rooftop Basis Function Set
 */
typedef struct {
    rooftop_basis_t* basis_functions;
    int num_basis_functions;
    
    rooftop_grid_t* grids;
    int num_grids;
    
    // Connectivity
    int* cell_to_basis;            // Cell to basis functions
    int num_cells;
} rooftop_basis_set_t;

// ============================================================================
// Rooftop Basis Function Interface
// ============================================================================

/**
 * Create rooftop basis functions from grid
 * 
 * L2 layer creates basis functions from grid topology.
 */
int rooftop_basis_create(
    const rooftop_grid_t* grid,
    rooftop_basis_set_t* basis_set
);

/**
 * Create rooftop basis functions from multiple grids
 */
int rooftop_basis_create_multigrid(
    const rooftop_grid_t* grids,
    int num_grids,
    rooftop_basis_set_t* basis_set
);

/**
 * Destroy rooftop basis function set
 */
void rooftop_basis_destroy(rooftop_basis_set_t* basis_set);

/**
 * Get basis function by index
 */
const rooftop_basis_t* rooftop_basis_get(
    const rooftop_basis_set_t* basis_set,
    int index
);

/**
 * Get basis functions on cell
 */
int rooftop_basis_get_on_cell(
    const rooftop_basis_set_t* basis_set,
    int grid_id,
    int cell_index,
    int* basis_indices,
    int max_basis
);

/**
 * Validate rooftop basis function set
 */
bool rooftop_basis_validate(const rooftop_basis_set_t* basis_set);

#ifdef __cplusplus
}
#endif

#endif // ROOFTOP_BASIS_H
