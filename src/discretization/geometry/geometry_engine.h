/********************************************************************************
 * Geometry Engine (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file handles geometry processing and discretization preparation.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 *
 * Architecture Rule: L2 layer defines discretization, not physics or solvers.
 ********************************************************************************/

#ifndef GEOMETRY_ENGINE_H
#define GEOMETRY_ENGINE_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "core_geometry.h"  // Include core_geometry.h for enum definitions

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Geometry Element Types
// ============================================================================

// Note: geom_element_type_t and geom_format_t are defined in core_geometry.h

// ============================================================================
// Geometry Engine Handle (Opaque Pointer)
// ============================================================================

/**
 * Geometry Engine Handle
 * 
 * Opaque pointer to geometry engine implementation
 * L2 layer uses opaque handles to hide implementation details
 */
typedef struct geometry_engine geometry_engine_t;

// ============================================================================
// Geometric Primitives
// ============================================================================

// Note: All geometric primitive types (geom_point_t, geom_vertex_t, geom_line_t,
// geom_triangle_t, geom_quadrilateral_t, geom_rectangle_t, geom_topology_t, geom_entity_t)
// are defined in core_geometry.h

// ============================================================================
// Geometry Engine Interface
// ============================================================================

/**
 * Create geometry engine
 */
void* geometry_engine_create(void);

/**
 * Destroy geometry engine
 */
void geometry_engine_destroy(void* engine);

/**
 * Import geometry from file
 * 
 * L2 layer handles file parsing, not physics interpretation
 */
int geometry_engine_import_file(
    void* engine,
    const char* filename,
    geom_format_t format
);

/**
 * Add geometric entity
 */
int geometry_engine_add_entity(
    void* engine,
    const geom_entity_t* entity
);

/**
 * Get bounding box
 */
int geometry_engine_get_bbox(
    void* engine,
    real_t* bbox_min,
    real_t* bbox_max
);

/**
 * Validate geometry
 * 
 * L2 layer checks geometric validity, not physics
 */
bool geometry_engine_validate(void* engine);

#ifdef __cplusplus
}
#endif

#endif // GEOMETRY_ENGINE_H
