/********************************************************************************
 * Mesh Engine Implementation (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements mesh generation and validation.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 *
 * Architecture Rule: L2 layer defines discretization, not physics or solvers.
 ********************************************************************************/

#include "mesh_engine.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Internal mesh engine structure
typedef struct {
    mesh_t* current_mesh;
    mesh_params_t default_params;
} mesh_engine_internal_t;

void* mesh_engine_create(void) {
    mesh_engine_internal_t* engine = (mesh_engine_internal_t*)calloc(1, sizeof(mesh_engine_internal_t));
    if (!engine) return NULL;
    
    // Initialize default parameters
    engine->default_params.target_edge_length = 0.01;  // 1 cm default
    engine->default_params.min_edge_length = 0.001;   // 1 mm minimum
    engine->default_params.max_edge_length = 0.1;     // 10 cm maximum
    engine->default_params.mesh_density = 10.0;       // 10 elements per wavelength
    engine->default_params.curvature_tolerance = 0.1;
    engine->default_params.algorithm = MESH_ALGORITHM_DELAUNAY;
    engine->default_params.use_adaptive_refinement = false;
    engine->default_params.max_refinement_levels = 3;
    engine->default_params.min_angle = 30.0;          // 30 degrees
    engine->default_params.max_aspect_ratio = 3.0;
    
    return engine;
}

void mesh_engine_destroy(void* engine) {
    if (!engine) return;
    
    mesh_engine_internal_t* e = (mesh_engine_internal_t*)engine;
    
    // Free current mesh if exists
    if (e->current_mesh) {
        // Free mesh data
        if (e->current_mesh->vertices) {
            for (int i = 0; i < e->current_mesh->num_vertices; i++) {
                if (e->current_mesh->vertices[i].adjacent_elements) {
                    free(e->current_mesh->vertices[i].adjacent_elements);
                }
            }
            free(e->current_mesh->vertices);
        }
        
        if (e->current_mesh->edges) {
            free(e->current_mesh->edges);
        }
        
        if (e->current_mesh->elements) {
            for (int i = 0; i < e->current_mesh->num_elements; i++) {
                if (e->current_mesh->elements[i].vertices) {
                    free(e->current_mesh->elements[i].vertices);
                }
                if (e->current_mesh->elements[i].edges) {
                    free(e->current_mesh->elements[i].edges);
                }
            }
            free(e->current_mesh->elements);
        }
        
        // Free topology arrays
        if (e->current_mesh->vertex_to_elements) {
            for (int i = 0; i < e->current_mesh->num_vertices; i++) {
                if (e->current_mesh->vertex_to_elements[i]) {
                    free(e->current_mesh->vertex_to_elements[i]);
                }
            }
            free(e->current_mesh->vertex_to_elements);
        }
        
        free(e->current_mesh);
    }
    
    free(e);
}

// Helper: Compute triangle area
static real_t compute_triangle_area(
    const geom_point_t* v1,
    const geom_point_t* v2,
    const geom_point_t* v3) {
    
    real_t v1v2[3] = {v2->x - v1->x, v2->y - v1->y, v2->z - v1->z};
    real_t v1v3[3] = {v3->x - v1->x, v3->y - v1->y, v3->z - v1->z};
    
    // Cross product
    real_t cross[3] = {
        v1v2[1] * v1v3[2] - v1v2[2] * v1v3[1],
        v1v2[2] * v1v3[0] - v1v2[0] * v1v3[2],
        v1v2[0] * v1v3[1] - v1v2[1] * v1v3[0]
    };
    
    // Area = 0.5 * |cross|
    return 0.5 * sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
}

// Helper: Compute element centroid
static void compute_element_centroid(
    const mesh_element_t* element,
    const mesh_vertex_t* vertices,
    geom_point_t* centroid) {
    
    centroid->x = 0.0;
    centroid->y = 0.0;
    centroid->z = 0.0;
    
    for (int i = 0; i < element->num_vertices; i++) {
        int vid = element->vertices[i];
        centroid->x += vertices[vid].position.x;
        centroid->y += vertices[vid].position.y;
        centroid->z += vertices[vid].position.z;
    }
    
    real_t inv_n = 1.0 / element->num_vertices;
    centroid->x *= inv_n;
    centroid->y *= inv_n;
    centroid->z *= inv_n;
}

int mesh_engine_generate(
    void* engine,
    const void* geometry,
    const mesh_params_t* params,
    mesh_t* mesh) {
    
    if (!engine || !geometry || !params || !mesh) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    mesh_engine_internal_t* e = (mesh_engine_internal_t*)engine;
    // geometry is an opaque pointer, cast is safe for L2 layer
    const void* geom = geometry;
    
    // Initialize mesh
    memset(mesh, 0, sizeof(mesh_t));
    mesh->type = MESH_TYPE_TRIANGULAR;  // Default
    mesh->algorithm = params->algorithm;
    
    // Simplified mesh generation
    // Full implementation would:
    // 1. Extract geometry entities from geometry engine
    // 2. Generate nodes based on algorithm
    // 3. Generate elements (triangles, quads, etc.)
    // 4. Build topology
    // 5. Compute quality metrics
    
    // For now, create a minimal mesh structure
    mesh->num_vertices = 0;
    mesh->num_edges = 0;
    mesh->num_elements = 0;
    mesh->vertices = NULL;
    mesh->edges = NULL;
    mesh->elements = NULL;
    
    // Initialize bounding box
    mesh->min_bound.x = mesh->min_bound.y = mesh->min_bound.z = 1e10;
    mesh->max_bound.x = mesh->max_bound.y = mesh->max_bound.z = -1e10;
    
    // Store current mesh
    e->current_mesh = mesh;
    
    return STATUS_SUCCESS;
}

bool mesh_engine_validate(
    const mesh_t* mesh,
    mesh_quality_t* quality) {
    
    if (!mesh || !quality) return false;
    
    // Initialize quality metrics
    quality->min_angle = 180.0;
    quality->max_angle = 0.0;
    quality->aspect_ratio = 0.0;
    quality->skewness = 0.0;
    quality->orthogonality = 0.0;
    quality->smoothness = 0.0;
    
    if (mesh->num_elements == 0) return true;
    
    // Validate each element
    real_t min_angle = 180.0;
    real_t max_angle = 0.0;
    real_t max_aspect = 0.0;
    
    for (int i = 0; i < mesh->num_elements; i++) {
        const mesh_element_t* elem = &mesh->elements[i];
        
        if (elem->type == MESH_ELEMENT_TRIANGLE && elem->num_vertices == 3) {
            // Compute angles for triangle
            const mesh_vertex_t* v0 = &mesh->vertices[elem->vertices[0]];
            const mesh_vertex_t* v1 = &mesh->vertices[elem->vertices[1]];
            const mesh_vertex_t* v2 = &mesh->vertices[elem->vertices[2]];
            
            // Compute edge lengths
            real_t e0 = sqrt(
                (v1->position.x - v0->position.x) * (v1->position.x - v0->position.x) +
                (v1->position.y - v0->position.y) * (v1->position.y - v0->position.y) +
                (v1->position.z - v0->position.z) * (v1->position.z - v0->position.z)
            );
            real_t e1 = sqrt(
                (v2->position.x - v1->position.x) * (v2->position.x - v1->position.x) +
                (v2->position.y - v1->position.y) * (v2->position.y - v1->position.y) +
                (v2->position.z - v1->position.z) * (v2->position.z - v1->position.z)
            );
            real_t e2 = sqrt(
                (v0->position.x - v2->position.x) * (v0->position.x - v2->position.x) +
                (v0->position.y - v2->position.y) * (v0->position.y - v2->position.y) +
                (v0->position.z - v2->position.z) * (v0->position.z - v2->position.z)
            );
            
            // Compute angles using law of cosines
            real_t angle0 = acos((e1*e1 + e2*e2 - e0*e0) / (2.0 * e1 * e2)) * 180.0 / M_PI;
            real_t angle1 = acos((e0*e0 + e2*e2 - e1*e1) / (2.0 * e0 * e2)) * 180.0 / M_PI;
            real_t angle2 = acos((e0*e0 + e1*e1 - e2*e2) / (2.0 * e0 * e1)) * 180.0 / M_PI;
            
            if (angle0 < min_angle) min_angle = angle0;
            if (angle1 < min_angle) min_angle = angle1;
            if (angle2 < min_angle) min_angle = angle2;
            
            if (angle0 > max_angle) max_angle = angle0;
            if (angle1 > max_angle) max_angle = angle1;
            if (angle2 > max_angle) max_angle = angle2;
            
            // Aspect ratio
            real_t min_edge = (e0 < e1) ? ((e0 < e2) ? e0 : e2) : ((e1 < e2) ? e1 : e2);
            real_t max_edge = (e0 > e1) ? ((e0 > e2) ? e0 : e2) : ((e1 > e2) ? e1 : e2);
            real_t aspect = max_edge / min_edge;
            if (aspect > max_aspect) max_aspect = aspect;
        }
    }
    
    quality->min_angle = min_angle;
    quality->max_angle = max_angle;
    quality->aspect_ratio = max_aspect;
    
    return (min_angle > 0.0 && max_angle < 180.0);
}

int mesh_engine_refine(
    mesh_t* mesh,
    const mesh_params_t* params) {
    
    if (!mesh || !params) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L2 layer performs geometric refinement, not physics-based refinement
    // Simplified implementation
    // Full implementation would:
    // 1. Identify elements that need refinement
    // 2. Subdivide elements
    // 3. Update topology
    // 4. Validate quality
    
    return STATUS_SUCCESS;
}

int mesh_engine_get_statistics(
    const mesh_t* mesh,
    int* num_vertices,
    int* num_edges,
    int* num_elements,
    mesh_quality_t* quality) {
    
    if (!mesh) return STATUS_ERROR_INVALID_INPUT;
    
    if (num_vertices) *num_vertices = mesh->num_vertices;
    if (num_edges) *num_edges = mesh->num_edges;
    if (num_elements) *num_elements = mesh->num_elements;
    
    if (quality) {
        mesh_engine_validate(mesh, quality);
    }
    
    return STATUS_SUCCESS;
}
