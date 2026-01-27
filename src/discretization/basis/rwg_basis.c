/********************************************************************************
 * RWG Basis Functions Implementation (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements RWG basis functions for discretization.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 ********************************************************************************/

#include "rwg_basis.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper function: Compute triangle area
static real_t compute_triangle_area(
    const mesh_element_t* element,
    const mesh_vertex_t* vertices) {
    
    if (!element || !vertices || element->num_vertices < 3) return 0.0;
    
    int v1_idx = element->vertices[0];
    int v2_idx = element->vertices[1];
    int v3_idx = element->vertices[2];
    
    const geom_point_t* v1 = &vertices[v1_idx].position;
    const geom_point_t* v2 = &vertices[v2_idx].position;
    const geom_point_t* v3 = &vertices[v3_idx].position;
    
    // Compute cross product
    real_t v1x = v2->x - v1->x;
    real_t v1y = v2->y - v1->y;
    real_t v1z = v2->z - v1->z;
    
    real_t v2x = v3->x - v1->x;
    real_t v2y = v3->y - v1->y;
    real_t v2z = v3->z - v1->z;
    
    real_t cross_x = v1y * v2z - v1z * v2y;
    real_t cross_y = v1z * v2x - v1x * v2z;
    real_t cross_z = v1x * v2y - v1y * v2x;
    
    return 0.5 * sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
}

int rwg_basis_create(
    const mesh_t* mesh,
    rwg_basis_set_t* basis_set) {
    
    if (!mesh || !basis_set) return STATUS_ERROR_INVALID_INPUT;
    
    // L2 layer creates basis functions from mesh topology
    // Count edges first
    int num_edges = mesh->num_edges;
    
    // Allocate basis function set
    basis_set->basis_functions = (rwg_basis_t*)calloc(num_edges, sizeof(rwg_basis_t));
    if (!basis_set->basis_functions) return STATUS_ERROR_MEMORY_ALLOCATION;
    
    basis_set->num_basis_functions = 0;
    basis_set->num_edges = num_edges;
    basis_set->num_triangles = mesh->num_elements;
    
    // Allocate connectivity arrays
    basis_set->edge_to_basis = (int*)malloc(num_edges * sizeof(int));
    basis_set->triangle_to_basis = (int*)malloc(mesh->num_elements * 3 * sizeof(int));
    if (!basis_set->edge_to_basis || !basis_set->triangle_to_basis) {
        rwg_basis_destroy(basis_set);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize edge_to_basis to -1 (no basis function)
    for (int i = 0; i < num_edges; i++) {
        basis_set->edge_to_basis[i] = -1;
    }
    
    // Create RWG basis functions for interior edges
    for (int edge_idx = 0; edge_idx < num_edges; edge_idx++) {
        const mesh_edge_t* edge = &mesh->edges[edge_idx];
        
        // Find triangles sharing this edge
        int tri_plus = -1, tri_minus = -1;
        for (int elem_idx = 0; elem_idx < mesh->num_elements; elem_idx++) {
            const mesh_element_t* elem = &mesh->elements[elem_idx];
            if (elem->type != MESH_ELEMENT_TRIANGLE) continue;
            
            // Check if this triangle contains this edge
            bool has_edge = false;
            for (int e = 0; e < elem->num_edges; e++) {
                if (elem->edges[e] == edge_idx) {
                    has_edge = true;
                    break;
                }
            }
            
            if (has_edge) {
                if (tri_plus < 0) {
                    tri_plus = elem_idx;
                } else {
                    tri_minus = elem_idx;
                    break;
                }
            }
        }
        
        // Only create basis function for interior edges (shared by two triangles)
        if (tri_plus >= 0 && tri_minus >= 0) {
            rwg_basis_t* bf = &basis_set->basis_functions[basis_set->num_basis_functions];
            
            bf->id = basis_set->num_basis_functions;
            bf->triangle_plus = tri_plus;
            bf->triangle_minus = tri_minus;
            bf->edge_index = edge_idx;
            bf->edge_length = edge->length;
            
            // Compute areas
            bf->area_plus = compute_triangle_area(&mesh->elements[tri_plus], mesh->vertices);
            bf->area_minus = compute_triangle_area(&mesh->elements[tri_minus], mesh->vertices);
            
            // Compute centroids
            bf->centroid_plus = mesh->elements[tri_plus].centroid;
            bf->centroid_minus = mesh->elements[tri_minus].centroid;
            
            // Compute edge vector
            const geom_point_t* v1 = &mesh->vertices[edge->vertex1_id].position;
            const geom_point_t* v2 = &mesh->vertices[edge->vertex2_id].position;
            bf->edge_vector.x = v2->x - v1->x;
            bf->edge_vector.y = v2->y - v1->y;
            bf->edge_vector.z = v2->z - v1->z;
            
            // Compute support region
            bf->support_center.x = (bf->centroid_plus.x + bf->centroid_minus.x) / 2.0;
            bf->support_center.y = (bf->centroid_plus.y + bf->centroid_minus.y) / 2.0;
            bf->support_center.z = (bf->centroid_plus.z + bf->centroid_minus.z) / 2.0;
            bf->support_radius = sqrt(bf->area_plus + bf->area_minus) / M_PI;
            
            // Update connectivity
            basis_set->edge_to_basis[edge_idx] = bf->id;
            
            basis_set->num_basis_functions++;
        }
    }
    
    return STATUS_SUCCESS;
}

void rwg_basis_destroy(rwg_basis_set_t* basis_set) {
    if (!basis_set) return;
    
    if (basis_set->basis_functions) {
        free(basis_set->basis_functions);
        basis_set->basis_functions = NULL;
    }
    
    if (basis_set->edge_to_basis) {
        free(basis_set->edge_to_basis);
        basis_set->edge_to_basis = NULL;
    }
    
    if (basis_set->triangle_to_basis) {
        free(basis_set->triangle_to_basis);
        basis_set->triangle_to_basis = NULL;
    }
    
    basis_set->num_basis_functions = 0;
}

const rwg_basis_t* rwg_basis_get(
    const rwg_basis_set_t* basis_set,
    int index) {
    
    if (!basis_set || index < 0 || index >= basis_set->num_basis_functions) {
        return NULL;
    }
    
    return &basis_set->basis_functions[index];
}

int rwg_basis_get_on_triangle(
    const rwg_basis_set_t* basis_set,
    int triangle_id,
    int* basis_indices,
    int max_basis) {
    
    if (!basis_set || !basis_indices || max_basis <= 0) {
        return 0;
    }
    
    // L2 layer provides topological queries, not physics
    int count = 0;
    for (int i = 0; i < basis_set->num_basis_functions && count < max_basis; i++) {
        const rwg_basis_t* bf = &basis_set->basis_functions[i];
        if (bf->triangle_plus == triangle_id || bf->triangle_minus == triangle_id) {
            basis_indices[count++] = i;
        }
    }
    
    return count;
}

int rwg_basis_get_on_edge(
    const rwg_basis_set_t* basis_set,
    int edge_id) {
    
    if (!basis_set || edge_id < 0 || edge_id >= basis_set->num_edges) {
        return -1;
    }
    
    return basis_set->edge_to_basis[edge_id];
}

bool rwg_basis_validate(const rwg_basis_set_t* basis_set) {
    if (!basis_set) return false;
    
    // L2 layer checks topological consistency, not physics
    for (int i = 0; i < basis_set->num_basis_functions; i++) {
        const rwg_basis_t* bf = &basis_set->basis_functions[i];
        
        if (bf->triangle_plus < 0 || bf->triangle_minus < 0) return false;
        if (bf->edge_index < 0) return false;
        if (bf->edge_length <= 0.0) return false;
        if (bf->area_plus <= 0.0 || bf->area_minus <= 0.0) return false;
    }
    
    return true;
}
