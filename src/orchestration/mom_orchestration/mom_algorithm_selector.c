/********************************************************************************
 * MoM Algorithm Selector Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements the algorithm selection logic for MoM solver.
 * L5 layer: Execution Orchestration - WHEN and HOW to use solvers.
 *
 * Extracted from: solvers/mom/mom_solver_unified.c
 ********************************************************************************/

#include "mom_algorithm_selector.h"
#include "../../common/constants.h"
#include "../../common/core_common.h"  // For HIGH_ACCURACY_TOLERANCE
#include "../../discretization/mesh/core_mesh.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define CURVATURE_THRESHOLD_DEFAULT (M_PI / 18.0)  // 10 degrees

/********************************************************************************
 * Select Optimal MoM Algorithm
 ********************************************************************************/
mom_algorithm_t mom_select_algorithm(const mom_problem_characteristics_t *problem) {
    if (!problem) return MOM_ALGO_BASIC;
    
    if (problem->num_unknowns < 1000) {
        return MOM_ALGO_BASIC;  // Direct solver for small problems
    } else if (problem->num_unknowns < 50000) {
        return MOM_ALGO_ACA;    // ACA for medium problems
    } else if (problem->electrical_size > 10.0) {
        return MOM_ALGO_MLFMM;  // MLFMM for large electrical problems
    } else {
        return MOM_ALGO_HMATRIX; // H-matrix for general large problems
    }
}

/********************************************************************************
 * Detect Curved Surfaces in Mesh
 ********************************************************************************/
static bool detect_curved_surfaces(const mesh_t* mesh) {
    if (!mesh || mesh->num_elements < 3) return false;
    
    const double CURVATURE_THRESHOLD = CURVATURE_THRESHOLD_DEFAULT;
    const int MIN_SAMPLES = 10;
    
    int num_samples = (mesh->num_elements < MIN_SAMPLES) ? mesh->num_elements : MIN_SAMPLES;
    int step = mesh->num_elements / num_samples;
    if (step < 1) step = 1;
    
    int curved_count = 0;
    int total_checked = 0;
    
    // Check normal vector variation between adjacent elements
    for (int i = 0; i < mesh->num_elements - 1 && total_checked < num_samples; i += step) {
        if (i + step >= mesh->num_elements) break;
        
        const mesh_element_t* elem1 = &mesh->elements[i];
        const mesh_element_t* elem2 = &mesh->elements[i + step];
        
        // Only check surface elements
        if (elem1->type != MESH_ELEMENT_TRIANGLE && 
            elem1->type != MESH_ELEMENT_QUADRILATERAL &&
            elem1->type != MESH_ELEMENT_RECTANGLE) {
            continue;
        }
        
        if (elem2->type != MESH_ELEMENT_TRIANGLE && 
            elem2->type != MESH_ELEMENT_QUADRILATERAL &&
            elem2->type != MESH_ELEMENT_RECTANGLE) {
            continue;
        }
        
        // Compute dot product of normals
        double dot = elem1->normal.x * elem2->normal.x +
                    elem1->normal.y * elem2->normal.y +
                    elem1->normal.z * elem2->normal.z;
        
        // Normalize dot product
        double angle = acos(fmax(-1.0, fmin(1.0, dot)));
        
        // If angle between normals is significant, surface is curved
        if (angle > CURVATURE_THRESHOLD) {
            curved_count++;
        }
        
        total_checked++;
    }
    
    // If more than 30% of checked pairs show curvature, consider surface curved
    return (total_checked > 0 && (double)curved_count / total_checked > 0.3);
}

/********************************************************************************
 * Count Different Materials
 ********************************************************************************/
static int count_different_materials(const mesh_t* mesh) {
    if (!mesh || mesh->num_elements == 0) return 1;
    
    // Simple approach: count unique material IDs
    int max_material_id = 0;
    bool* material_present = NULL;
    
    // Find maximum material ID
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].material_id > max_material_id) {
            max_material_id = mesh->elements[i].material_id;
        }
    }
    
    if (max_material_id < 0) return 1;
    
    // Allocate and initialize material presence array
    material_present = (bool*)calloc(max_material_id + 1, sizeof(bool));
    if (!material_present) return 1;
    
    // Mark present materials
    for (int i = 0; i < mesh->num_elements; i++) {
        int mat_id = mesh->elements[i].material_id;
        if (mat_id >= 0 && mat_id <= max_material_id) {
            material_present[mat_id] = true;
        }
    }
    
    // Count unique materials
    int count = 0;
    for (int i = 0; i <= max_material_id; i++) {
        if (material_present[i]) count++;
    }
    
    free(material_present);
    
    return (count > 0) ? count : 1;
}

/********************************************************************************
 * Compute Problem Characteristics
 ********************************************************************************/
int mom_compute_problem_characteristics(
    const mesh_t* mesh,
    double frequency,
    bool enable_wideband,
    double tolerance,
    mom_problem_characteristics_t* characteristics) {
    
    if (!mesh || !characteristics) return -1;
    
    // Initialize
    memset(characteristics, 0, sizeof(mom_problem_characteristics_t));
    
    // Count basis functions (simplified: assume one per element)
    characteristics->num_unknowns = mesh->num_elements;
    characteristics->frequency = frequency;
    characteristics->is_wideband = enable_wideband;
    
    // Compute electrical size
    double max_dimension_sq = 0.0;
    const mesh_vertex_t* vertices = mesh->vertices;
    const int num_vertices = mesh->num_vertices;
    
    for (int i = 0; i < num_vertices; i++) {
        const geom_point_t* pos = &vertices[i].position;
        const double x = pos->x;
        const double y = pos->y;
        const double z = pos->z;
        const double r_sq = x*x + y*y + z*z;
        if (r_sq > max_dimension_sq) {
            max_dimension_sq = r_sq;
        }
    }
    const double max_dimension = sqrt(max_dimension_sq);
    
    const double wavelength = C0 / frequency;
    characteristics->electrical_size = 2.0 * max_dimension / wavelength;
    
    // Detect curved surfaces
    characteristics->has_curved_surfaces = detect_curved_surfaces(mesh);
    
    // Count different materials
    characteristics->num_materials = count_different_materials(mesh);
    
    // High accuracy requirement
    characteristics->requires_high_accuracy = (tolerance < HIGH_ACCURACY_TOLERANCE);
    
    return 0;
}
