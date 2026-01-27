/********************************************************************************
 * Mesh Engine (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file handles mesh generation and validation.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 *
 * Architecture Rule: L2 layer defines discretization, not physics or solvers.
 ********************************************************************************/

#ifndef MESH_ENGINE_H
#define MESH_ENGINE_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../geometry/geometry_engine.h"
#include "core_mesh.h"  // Include core_mesh.h for mesh_element_type_t

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Mesh Element Types
// ============================================================================

// Note: All mesh types are defined in core_mesh.h (included above):
// - mesh_element_type_t
// - mesh_type_t
// - mesh_algorithm_t
// - mesh_quality_t
// - mesh_vertex_t
// - mesh_edge_t
// - mesh_element_t
// - mesh_t

// ============================================================================
// Mesh Generation Parameters
// ============================================================================

/**
 * Mesh Generation Parameters
 * 
 * L2 layer defines discretization parameters, not physics
 */
typedef struct {
    real_t target_edge_length;    // Target edge length (m)
    real_t min_edge_length;       // Minimum edge length (m)
    real_t max_edge_length;       // Maximum edge length (m)
    real_t mesh_density;          // Elements per wavelength (for frequency-based)
    real_t curvature_tolerance;   // Curvature-based refinement tolerance
    
    // Algorithm-specific parameters
    mesh_algorithm_t algorithm;
    bool use_adaptive_refinement;
    int max_refinement_levels;
    
    // Quality constraints
    real_t min_angle;              // Minimum angle constraint
    real_t max_aspect_ratio;       // Maximum aspect ratio
} mesh_params_t;

// ============================================================================
// Mesh Engine Interface
// ============================================================================

/**
 * Create mesh engine
 */
void* mesh_engine_create(void);

/**
 * Destroy mesh engine
 */
void mesh_engine_destroy(void* engine);

/**
 * Generate mesh from geometry
 * 
 * L2 layer performs discretization, not physics calculation
 */
int mesh_engine_generate(
    void* engine,
    const void* geometry,      // Geometry engine
    const mesh_params_t* params,
    mesh_t* mesh
);

/**
 * Validate mesh quality
 * 
 * L2 layer checks geometric quality, not physics accuracy
 */
bool mesh_engine_validate(
    const mesh_t* mesh,
    mesh_quality_t* quality
);

/**
 * Refine mesh
 * 
 * L2 layer performs geometric refinement, not physics-based refinement
 */
int mesh_engine_refine(
    mesh_t* mesh,
    const mesh_params_t* params
);

/**
 * Get mesh statistics
 */
int mesh_engine_get_statistics(
    const mesh_t* mesh,
    int* num_vertices,
    int* num_edges,
    int* num_elements,
    mesh_quality_t* quality
);

#ifdef __cplusplus
}
#endif

#endif // MESH_ENGINE_H
