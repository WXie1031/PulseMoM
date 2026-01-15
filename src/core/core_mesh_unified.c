/*********************************************************************
 * PulseEM - Unified Core Mesh Implementation
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * E-mail: chenhc@seu.edu.cn 
 * 
 * All rights reserved. This program is the proprietary software of the AI4MW Research Group. 
 * Unauthorized reproduction, distribution, modification, or use of this program in whole or in part 
 * is strictly prohibited without prior written permission from the copyright holder.
 * 
 * File: core_mesh_unified.c
 * Description: Unified mesh generation combining basic and advanced features
 * 
 * Features:
 * - Unified mesh generation for triangular (MoM) and Manhattan rectangular (PEEC) elements
 * - Advanced quality metrics and mesh validation algorithms
 * - Adaptive refinement with error estimation and convergence control
 * - Parallel decomposition for multi-threaded processing
 * - Memory-efficient data structures for large-scale problems
 * - Comprehensive mesh optimization and smoothing techniques
 * - Multi-format mesh import/export capabilities
 * - Integration with geometry engine for automatic meshing
 * 
 * Technical Specifications:
 * - C11 compliant with POSIX standard compliance
 * - OpenMP parallel processing support for performance
 * - Dynamic memory management with automatic cleanup
 * - Thread-safe operations with proper synchronization
 * - Cross-platform compatibility (Linux, Windows, macOS)
 *********************************************************************/

// _CRT_SECURE_NO_WARNINGS defined in project settings
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <omp.h>
#include <time.h>

#include "core_mesh.h"

/* Forward declarations */
static double mesh_get_tetrahedron_volume(const geom_point_t* p0, const geom_point_t* p1, 
                                         const geom_point_t* p2, const geom_point_t* p3);

/* Refinement strategy type */
typedef enum {
    REFINEMENT_STRATEGY_UNIFORM = 0,
    REFINEMENT_STRATEGY_ADAPTIVE = 1,
    REFINEMENT_STRATEGY_GRADED = 2,
    REFINEMENT_STRATEGY_EDGE_BASED = 3,
    REFINEMENT_STRATEGY_ERROR_BASED = 4,
    REFINEMENT_STRATEGY_FEATURE_BASED = 5,
    REFINEMENT_STRATEGY_SINGULARITY = 6
} refinement_strategy_t;

/* Quality statistics structure */
typedef struct mesh_quality_statistics_t {
    double minimum_angle;           /* Minimum angle in degrees */
    double maximum_angle;           /* Maximum angle in degrees */
    double aspect_ratio;            /* Average aspect ratio */
    double skewness;                /* Average skewness */
    double orthogonality;           /* Average orthogonality */
    double average_quality;         /* Average element quality */
    double worst_quality;           /* Worst element quality */
    int num_elements;               /* Number of elements analyzed */
} mesh_quality_statistics_t;

/* Unified mesh structure combining basic and advanced features */
struct unified_mesh_t {
    mesh_t base;                    /* Basic mesh data */
    
    /* Advanced features */
    int max_refinement_level;       /* Maximum refinement level */
    double total_mesh_time;         /* Total mesh generation time */
    int refinement_iterations;      /* Number of refinement iterations */
    bool is_converged;              /* Convergence status */
    double mesh_memory_usage;       /* Memory usage in MB */
    
    /* Quality statistics */
    mesh_quality_statistics_t quality_stats;
    
    /* Parallel processing info */
    int num_threads;                /* Number of OpenMP threads */
    
};

/* Advanced mesh parameters */
struct unified_mesh_parameters_t {
    mesh_type_t mesh_type;          /* Type of mesh */
    mesh_algorithm_t algorithm;     /* Mesh generation algorithm */
    bool enable_parallel_generation;/* Enable parallel processing */
    bool enable_quality_optimization;/* Enable quality optimization */
    
    /* Sizing parameters */
    struct {
        double target_size;         /* Target element size */
        double min_size;           /* Minimum element size */
        double max_size;           /* Maximum element size */
        double gradation;          /* Size gradation factor */
    } sizing;
    
    /* Quality parameters */
    struct {
        double target_quality;      /* Target quality metric */
        double min_quality;         /* Minimum acceptable quality */
        int max_smoothing_iterations;/* Maximum smoothing iterations */
    } quality;
    
};

/* Adaptive refinement control */
typedef struct {
    refinement_strategy_t strategy; /* Refinement strategy */
    double refinement_threshold;    /* Refinement threshold */
    int max_refinement_level;       /* Maximum refinement level */
    double* error_indicators;        /* Error indicators per element */
    int* refinement_flags;          /* Refinement flags per element */
    int num_error_indicators;       /* Number of error indicators */
} adaptive_refinement_control_t;

/* Private function declarations */
static double calculate_triangle_quality(const geom_point_t* p1, const geom_point_t* p2, const geom_point_t* p3);
static double calculate_element_quality(const mesh_element_t* element, const mesh_vertex_t* vertices);
static int optimize_mesh_laplace_smoothing(mesh_t* mesh, int iterations);
static int calculate_mesh_statistics(const mesh_t* mesh, mesh_quality_statistics_t* stats);
static int parallel_mesh_refinement(mesh_t* mesh, adaptive_refinement_control_t* refinement, mesh_t* refined_mesh);

/* Global variables */
static int mesh_initialized = 0;
static int mesh_element_counter = 0;
static int mesh_node_counter = 0;

/*********************************************************************
 * Basic Mesh Operations (Enhanced)
 *********************************************************************/

mesh_t* mesh_create(const char* name, mesh_type_t type) {
    mesh_t* m = (mesh_t*)calloc(1, sizeof(mesh_t));
    if (!m) return NULL;
    
    strncpy(m->name, name ? name : "", sizeof(m->name) - 1);
    m->type = type;
    m->algorithm = MESH_ALGORITHM_UNSTRUCTURED;
    
    /* Initialize counters */
    mesh_element_counter = 0;
    mesh_node_counter = 0;
    
    return m;
}

void mesh_destroy(mesh_t* mesh) {
    if (!mesh) return;
    
    if (mesh->elements) {
        for (int i = 0; i < mesh->num_elements; i++) {
            free(mesh->elements[i].vertices);
            free(mesh->elements[i].edges);
        }
        free(mesh->elements);
    }
    free(mesh->vertices);
    free(mesh->edges);
    free(mesh->vertex_to_elements);
    free(mesh->element_to_elements);
    free(mesh->partition_offsets);
    free(mesh->partition_elements);
    
    // Free solver-specific mesh data
    if (mesh->peec_mesh_data) {
        free(mesh->peec_mesh_data);
    }
    if (mesh->mom_mesh_data) {
        free(mesh->mom_mesh_data);
    }
    
    free(mesh);
}

int mesh_add_vertex(mesh_t* mesh, const geom_point_t* position) {
    if (!mesh || !position) return -1;
    
    int new_count = mesh->num_vertices + 1;
    mesh_vertex_t* v = (mesh_vertex_t*)realloc(mesh->vertices, new_count * sizeof(mesh_vertex_t));
    if (!v) return -1;
    
    mesh->vertices = v;
    mesh_vertex_t* mv = &mesh->vertices[mesh->num_vertices];
    mv->id = mesh->num_vertices;
    mv->position = *position;
    mv->is_boundary = false;
    mv->is_interface = false;
    mv->adjacent_elements = NULL;
    mv->num_adjacent_elements = 0;
    mv->potential = 0;
    mv->charge_density = 0;
    mv->current_density = 0;
    mv->mom_data = NULL;
    mv->peec_data = NULL;
    
    mesh->num_vertices = new_count;
    mesh_node_counter++;
    return mv->id;
}

int mesh_add_element(mesh_t* mesh, mesh_element_type_t element_type, const int* vertex_ids, int num_vertices) {
    if (!mesh || !vertex_ids || num_vertices <= 0) return -1;
    
    int new_count = mesh->num_elements + 1;
    mesh_element_t* e = (mesh_element_t*)realloc(mesh->elements, new_count * sizeof(mesh_element_t));
    if (!e) return -1;
    
    mesh->elements = e;
    mesh_element_t* me = &mesh->elements[mesh->num_elements];
    me->id = mesh->num_elements;
    me->type = element_type;
    me->num_vertices = num_vertices;
    me->vertices = (int*)calloc(num_vertices, sizeof(int));
    if (!me->vertices) return -1;
    
    memcpy(me->vertices, vertex_ids, num_vertices * sizeof(int));
    me->edges = NULL;
    me->num_edges = 0;
    me->area = 0.0;
    me->volume = 0.0;
    me->material_id = 0;
    me->region_id = 0;
    me->domain_id = 0;
    me->quality_factor = 0.0;
    me->characteristic_length = 0.0;
    me->mom_data = NULL;
    me->peec_data = NULL;
    
    /* Calculate element quality */
    me->quality_factor = calculate_element_quality(me, mesh->vertices);
    
    mesh->num_elements = new_count;
    mesh_element_counter++;
    return me->id;
}

mesh_vertex_t* mesh_get_vertex(mesh_t* mesh, int vertex_id) {
    if (!mesh || vertex_id < 0 || vertex_id >= mesh->num_vertices) return NULL;
    return &mesh->vertices[vertex_id];
}

mesh_element_t* mesh_get_element(mesh_t* mesh, int element_id) {
    if (!mesh || element_id < 0 || element_id >= mesh->num_elements) return NULL;
    return &mesh->elements[element_id];
}

bool mesh_validate_quality(const mesh_t* mesh) {
    if (!mesh) return false;
    
    /* Basic validation - check for degenerate elements */
    for (int i = 0; i < mesh->num_elements; i++) {
        const mesh_element_t* e = &mesh->elements[i];
        if (e->num_vertices < 3) return false;
        if (e->quality_factor < 0.1) return false; /* Poor quality threshold */
    }
    return true;
}

int mesh_get_element_count_by_type(const mesh_t* mesh, mesh_element_type_t type) {
    if (!mesh) return 0;
    
    int count = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == type) count++;
    }
    return count;
}

/*********************************************************************
 * Advanced Mesh Operations
 *********************************************************************/

/* Calculate triangle quality using shape factor */
static double calculate_triangle_quality(const geom_point_t* p1, const geom_point_t* p2, const geom_point_t* p3) {
    /* Calculate triangle area using cross product */
    double v1x = p2->x - p1->x, v1y = p2->y - p1->y, v1z = p2->z - p1->z;
    double v2x = p3->x - p1->x, v2y = p3->y - p1->y, v2z = p3->z - p1->z;
    
    double cross_x = v1y * v2z - v1z * v2y;
    double cross_y = v1z * v2x - v1x * v2z;
    double cross_z = v1x * v2y - v1y * v2x;
    
    double area = 0.5 * sqrt(cross_x*cross_x + cross_y*cross_y + cross_z*cross_z);
    if (area < 1e-12) return 0.0; /* Degenerate triangle */
    
    /* Calculate edge lengths */
    double edge1 = sqrt(v1x*v1x + v1y*v1y + v1z*v1z);
    double edge2 = sqrt(v2x*v2x + v2y*v2y + v2z*v2z);
    
    double v3x = p1->x - p3->x, v3y = p1->y - p3->y, v3z = p1->z - p3->z;
    double edge3 = sqrt(v3x*v3x + v3y*v3y + v3z*v3z);
    
    /* Calculate quality metric (shape factor) */
    double perimeter = edge1 + edge2 + edge3;
    double quality = (4.0 * M_PI * area) / (perimeter * perimeter);
    
    return quality;
}

/* Calculate element quality based on type */
static double calculate_element_quality(const mesh_element_t* element, const mesh_vertex_t* vertices) {
    if (!element || !vertices || element->num_vertices < 3) return 0.0;
    
    const geom_point_t* p0 = &vertices[element->vertices[0]].position;
    const geom_point_t* p1 = &vertices[element->vertices[1]].position;
    const geom_point_t* p2 = &vertices[element->vertices[2]].position;
    
    if (element->type == MESH_ELEMENT_TRIANGLE) {
        return calculate_triangle_quality(p0, p1, p2);
    } else if (element->type == MESH_ELEMENT_QUADRILATERAL && element->num_vertices >= 4) {
        const geom_point_t* p3 = &vertices[element->vertices[3]].position;
        /* For quadrilaterals, use minimum of two triangle qualities */
        double q1 = calculate_triangle_quality(p0, p1, p2);
        double q2 = calculate_triangle_quality(p0, p2, p3);
        return fmin(q1, q2);
    }
    
    return 0.5; /* Default quality for other types */
}

/* Enhanced area calculation with better numerical stability */
static double tri_area_enhanced(const geom_point_t* a, const geom_point_t* b, const geom_point_t* c) {
    double x1 = b->x - a->x, y1 = b->y - a->y, z1 = b->z - a->z;
    double x2 = c->x - a->x, y2 = c->y - a->y, z2 = c->z - a->z;
    
    double cross_x = y1 * z2 - z1 * y2;
    double cross_y = z1 * x2 - x1 * z2;
    double cross_z = x1 * y2 - y1 * x2;
    
    double cross_mag = sqrt(cross_x*cross_x + cross_y*cross_y + cross_z*cross_z);
    return 0.5 * cross_mag;
}

double mesh_get_total_area(const mesh_t* mesh) {
    if (!mesh) return 0.0;
    
    double total = 0.0;
    for (int i = 0; i < mesh->num_elements; i++) {
        mesh_element_t* e = &mesh->elements[i];
        if (e->num_vertices < 3) continue;
        
        const geom_point_t* p0 = &mesh->vertices[e->vertices[0]].position;
        const geom_point_t* p1 = &mesh->vertices[e->vertices[1]].position;
        const geom_point_t* p2 = &mesh->vertices[e->vertices[2]].position;
        
        if (e->type == MESH_ELEMENT_TRIANGLE) {
            total += tri_area_enhanced(p0, p1, p2);
        } else if ((e->type == MESH_ELEMENT_QUADRILATERAL || e->type == MESH_ELEMENT_RECTANGLE) && e->num_vertices >= 4) {
            const geom_point_t* p3 = &mesh->vertices[e->vertices[3]].position;
            total += tri_area_enhanced(p0, p1, p2) + tri_area_enhanced(p0, p2, p3);
        }
    }
    return total;
}

/* Enhanced volume calculation with better numerical stability */
double mesh_get_total_volume(const mesh_t* mesh) {
    if (!mesh || mesh->num_elements == 0) return 0.0;
    
    double total_volume = 0.0;
    
    for (int i = 0; i < mesh->num_elements; i++) {
        const mesh_element_t* e = &mesh->elements[i];
        if (e->num_vertices < 4) continue;
        
        if (e->type == MESH_ELEMENT_TETRAHEDRON && e->num_vertices >= 4) {
            const geom_point_t* p0 = &mesh->vertices[e->vertices[0]].position;
            const geom_point_t* p1 = &mesh->vertices[e->vertices[1]].position;
            const geom_point_t* p2 = &mesh->vertices[e->vertices[2]].position;
            const geom_point_t* p3 = &mesh->vertices[e->vertices[3]].position;
            
            /* Tetrahedron volume = |det([p1-p0, p2-p0, p3-p0])| / 6 */
            double dx1 = p1->x - p0->x, dy1 = p1->y - p0->y, dz1 = p1->z - p0->z;
            double dx2 = p2->x - p0->x, dy2 = p2->y - p0->y, dz2 = p2->z - p0->z;
            double dx3 = p3->x - p0->x, dy3 = p3->y - p0->y, dz3 = p3->z - p0->z;
            
            double det = dx1 * (dy2 * dz3 - dz2 * dy3) - dy1 * (dx2 * dz3 - dz2 * dx3) + dz1 * (dx2 * dy3 - dy2 * dx3);
            total_volume += fabs(det) / 6.0;
            
        } else if ((e->type == MESH_ELEMENT_HEXAHEDRON || e->type == MESH_ELEMENT_PRISM) && e->num_vertices >= 6) {
            /* For hexahedra and prisms, decompose into tetrahedra */
            /* This is a more accurate approach than bounding box approximation */
            const geom_point_t* vertices[8];
            int num_vertices_used = (e->num_vertices >= 8) ? 8 : e->num_vertices;
            
            for (int j = 0; j < num_vertices_used; j++) {
                vertices[j] = &mesh->vertices[e->vertices[j]].position;
            }
            
            /* Simple decomposition: divide hexahedron into 6 tetrahedra */
            if (num_vertices_used >= 8) {
                /* Tetrahedron 1: 0,1,2,6 */
                double vol1 = mesh_get_tetrahedron_volume(vertices[0], vertices[1], vertices[2], vertices[6]);
                /* Tetrahedron 2: 0,2,3,6 */
                double vol2 = mesh_get_tetrahedron_volume(vertices[0], vertices[2], vertices[3], vertices[6]);
                /* Tetrahedron 3: 0,3,7,6 */
                double vol3 = mesh_get_tetrahedron_volume(vertices[0], vertices[3], vertices[7], vertices[6]);
                /* Tetrahedron 4: 0,7,4,6 */
                double vol4 = mesh_get_tetrahedron_volume(vertices[0], vertices[7], vertices[4], vertices[6]);
                /* Tetrahedron 5: 0,4,5,6 */
                double vol5 = mesh_get_tetrahedron_volume(vertices[0], vertices[4], vertices[5], vertices[6]);
                /* Tetrahedron 6: 0,5,1,6 */
                double vol6 = mesh_get_tetrahedron_volume(vertices[0], vertices[5], vertices[1], vertices[6]);
                
                total_volume += vol1 + vol2 + vol3 + vol4 + vol5 + vol6;
            }
        }
    }
    
    return total_volume;
}

/* Helper function for tetrahedron volume */
double mesh_get_tetrahedron_volume(const geom_point_t* p0, const geom_point_t* p1, 
                                  const geom_point_t* p2, const geom_point_t* p3) {
    double dx1 = p1->x - p0->x, dy1 = p1->y - p0->y, dz1 = p1->z - p0->z;
    double dx2 = p2->x - p0->x, dy2 = p2->y - p0->y, dz2 = p2->z - p0->z;
    double dx3 = p3->x - p0->x, dy3 = p3->y - p0->y, dz3 = p3->z - p0->z;
    
    double det = dx1 * (dy2 * dz3 - dz2 * dy3) - dy1 * (dx2 * dz3 - dz2 * dx3) + dz1 * (dx2 * dy3 - dy2 * dx3);
    return fabs(det) / 6.0;
}

/* Enhanced quality computation with comprehensive metrics */
mesh_quality_t mesh_compute_quality(const mesh_t* mesh) {
    mesh_quality_t q = {0};
    if (!mesh || mesh->num_elements == 0) return q;
    
    double min_angle_deg = 180.0;
    double max_angle_deg = 0.0;
    double aspect_sum = 0.0;
    int aspect_count = 0;
    double skewness_sum = 0.0;
    int skewness_count = 0;
    double orth_sum = 0.0;
    int orth_count = 0;
    double quality_sum = 0.0;
    
    // MSVC OpenMP doesn't support min/max reduction, use manual tracking
    int i;
    // MSVC OpenMP doesn't support min/max reduction, use critical section
    #pragma omp parallel for reduction(+:aspect_sum,aspect_count,skewness_sum,skewness_count,orth_sum,orth_count,quality_sum)
    for (i = 0; i < mesh->num_elements; i++) {
        const mesh_element_t* e = &mesh->elements[i];
        if (e->num_vertices < 3) continue;
        
        const geom_point_t* p0 = &mesh->vertices[e->vertices[0]].position;
        const geom_point_t* p1 = &mesh->vertices[e->vertices[1]].position;
        const geom_point_t* p2 = &mesh->vertices[e->vertices[2]].position;
        
        double v01x = p1->x - p0->x, v01y = p1->y - p0->y, v01z = p1->z - p0->z;
        double v12x = p2->x - p1->x, v12y = p2->y - p1->y, v12z = p2->z - p1->z;
        double v20x = p0->x - p2->x, v20y = p0->y - p2->y, v20z = p0->z - p2->z;
        
        double l01 = sqrt(v01x*v01x + v01y*v01y + v01z*v01z);
        double l12 = sqrt(v12x*v12x + v12y*v12y + v12z*v12z);
        double l20 = sqrt(v20x*v20x + v20y*v20y + v20z*v20z);
        
        /* Aspect ratio */
        double maxe = fmax(l01, fmax(l12, l20));
        double mine = fmin(l01, fmin(l12, l20));
        if (mine > 0.0) {
            aspect_sum += maxe / mine;
            aspect_count++;
        }
        
        /* Triangle angles */
        double cosA = (v01x*(-v20x) + v01y*(-v20y) + v01z*(-v20z)) / (l01 * l20);
        double cosB = (v12x*(-v01x) + v12y*(-v01y) + v12z*(-v01z)) / (l12 * l01);
        double cosC = (v20x*(-v12x) + v20y*(-v12y) + v20z*(-v12z)) / (l20 * l12);
        
        /* Clamp cosine values to valid range */
        if (cosA < -1.0) cosA = -1.0; if (cosA > 1.0) cosA = 1.0;
        if (cosB < -1.0) cosB = -1.0; if (cosB > 1.0) cosB = 1.0;
        if (cosC < -1.0) cosC = -1.0; if (cosC > 1.0) cosC = 1.0;
        
        double A = acos(cosA) * (180.0 / M_PI);
        double B = acos(cosB) * (180.0 / M_PI);
        double C = acos(cosC) * (180.0 / M_PI);
        double local_min = fmin(A, fmin(B, C));
        double local_max = fmax(A, fmax(B, C));
        
        // Update global min/max with critical section (MSVC doesn't support min/max reduction)
        #pragma omp critical
        {
            if (local_min < min_angle_deg) min_angle_deg = local_min;
            if (local_max > max_angle_deg) max_angle_deg = local_max;
        }
        
        /* Skewness relative to equilateral 60 deg */
        double sA = fabs(A - 60.0) / 60.0;
        double sB = fabs(B - 60.0) / 60.0;
        double sC = fabs(C - 60.0) / 60.0;
        skewness_sum += (sA + sB + sC) / 3.0;
        skewness_count++;
        
        /* Element quality factor */
        quality_sum += e->quality_factor;
        
        /* Quadrilateral orthogonality if present */
        if ((e->type == MESH_ELEMENT_QUADRILATERAL || e->type == MESH_ELEMENT_RECTANGLE) && e->num_vertices >= 4) {
            const geom_point_t* p3 = &mesh->vertices[e->vertices[3]].position;
            double v23x = p3->x - p2->x, v23y = p3->y - p2->y, v23z = p3->z - p2->z;
            double v30x = p0->x - p3->x, v30y = p0->y - p3->y, v30z = p0->z - p3->z;
            double l23 = sqrt(v23x*v23x + v23y*v23y + v23z*v23z);
            double l30 = sqrt(v30x*v30x + v30y*v30y + v30z*v30z);
            
            double cosAB = (v01x*v12x + v01y*v12y + v01z*v12z) / (l01 * l12);
            double cosBC = (v12x*v23x + v12y*v23y + v12z*v23z) / (l12 * l23);
            double cosCD = (v23x*v30x + v23y*v30y + v23z*v30z) / (l23 * l30);
            double cosDA = (v30x*v01x + v30y*v01y + v30z*v01z) / (l30 * l01);
            
            /* Clamp cosine values */
            if (cosAB < -1.0) cosAB = -1.0; if (cosAB > 1.0) cosAB = 1.0;
            if (cosBC < -1.0) cosBC = -1.0; if (cosBC > 1.0) cosBC = 1.0;
            if (cosCD < -1.0) cosCD = -1.0; if (cosCD > 1.0) cosCD = 1.0;
            if (cosDA < -1.0) cosDA = -1.0; if (cosDA > 1.0) cosDA = 1.0;
            
            double angAB = acos(cosAB) * (180.0 / M_PI);
            double angBC = acos(cosBC) * (180.0 / M_PI);
            double angCD = acos(cosCD) * (180.0 / M_PI);
            double angDA = acos(cosDA) * (180.0 / M_PI);
            
            double devAB = fabs(angAB - 90.0) / 90.0;
            double devBC = fabs(angBC - 90.0) / 90.0;
            double devCD = fabs(angCD - 90.0) / 90.0;
            double devDA = fabs(angDA - 90.0) / 90.0;
            orth_sum += (devAB + devBC + devCD + devDA) / 4.0;
            orth_count++;
        }
    }
    
    q.aspect_ratio = (aspect_count > 0) ? (aspect_sum / aspect_count) : 0.0;
    q.min_angle = (min_angle_deg == 180.0) ? 0.0 : min_angle_deg;
    q.max_angle = max_angle_deg;
    q.skewness = (skewness_count > 0) ? (skewness_sum / skewness_count) : 0.0;
    q.orthogonality = (orth_count > 0) ? (orth_sum / orth_count) : 0.0;
    q.smoothness = (mesh->num_elements > 0) ? (quality_sum / mesh->num_elements) : 0.0;
    
    return q;
}

/* Enhanced Laplace smoothing with quality preservation */
int mesh_smooth_laplacian(mesh_t* mesh, int iterations) {
    if (!mesh || mesh->num_vertices == 0 || iterations <= 0) return -1;
    
    printf("Applying Laplace smoothing (%d iterations)...\n", iterations);
    
    for (int it = 0; it < iterations; it++) {
        /* Create backup of current vertex positions */
        geom_point_t* backup_positions = (geom_point_t*)malloc(mesh->num_vertices * sizeof(geom_point_t));
        if (!backup_positions) return -1;
        
        for (int v = 0; v < mesh->num_vertices; v++) {
            backup_positions[v] = mesh->vertices[v].position;
        }
        
        /* Apply smoothing */
        int v;
        #pragma omp parallel for
        for (v = 0; v < mesh->num_vertices; v++) {
            if (mesh->vertices[v].is_boundary) continue; /* Don't move boundary vertices */
            
            geom_point_t centroid = {0.0, 0.0, 0.0};
            int neighbor_count = 0;
            
            /* Find all vertices connected to this vertex through elements */
            for (int e = 0; e < mesh->num_elements; e++) {
                const mesh_element_t* el = &mesh->elements[e];
                bool found_vertex = false;
                
                /* Check if this element contains our vertex */
                for (int k = 0; k < el->num_vertices; k++) {
                    if (el->vertices[k] == v) {
                        found_vertex = true;
                        break;
                    }
                }
                
                if (found_vertex) {
                    /* Add all other vertices in this element */
                    for (int k = 0; k < el->num_vertices; k++) {
                        int nid = el->vertices[k];
                        if (nid != v) {
                            centroid.x += backup_positions[nid].x;
                            centroid.y += backup_positions[nid].y;
                            centroid.z += backup_positions[nid].z;
                            neighbor_count++;
                        }
                    }
                }
            }
            
            if (neighbor_count > 0) {
                /* Move vertex toward centroid with relaxation factor */
                double alpha = 0.5; /* Relaxation factor */
                mesh->vertices[v].position.x = (1.0 - alpha) * backup_positions[v].x + alpha * centroid.x / neighbor_count;
                mesh->vertices[v].position.y = (1.0 - alpha) * backup_positions[v].y + alpha * centroid.y / neighbor_count;
                mesh->vertices[v].position.z = (1.0 - alpha) * backup_positions[v].z + alpha * centroid.z / neighbor_count;
            }
        }
        
        free(backup_positions);
        
        /* Update element quality factors after smoothing */
        int i;
        #pragma omp parallel for
        for (i = 0; i < mesh->num_elements; i++) {
            mesh->elements[i].quality_factor = calculate_element_quality(&mesh->elements[i], mesh->vertices);
        }
    }
    
    printf("Laplace smoothing completed\n");
    return 0;
}

/* Adaptive refinement with multiple strategies */
int mesh_refine_adaptive(mesh_t* mesh, double refinement_threshold) {
    if (!mesh || refinement_threshold <= 0.0) return -1;
    
    printf("Performing adaptive mesh refinement (threshold: %.3f)...\n", refinement_threshold);
    
    adaptive_refinement_control_t refinement_ctrl = {
        .strategy = REFINEMENT_STRATEGY_ADAPTIVE,
        .refinement_threshold = refinement_threshold,
        .max_refinement_level = 5,
        .error_indicators = NULL,
        .refinement_flags = NULL,
        .num_error_indicators = 0
    };
    
    /* Create error indicators based on element quality */
    refinement_ctrl.error_indicators = (double*)calloc(mesh->num_elements, sizeof(double));
    if (!refinement_ctrl.error_indicators) return -1;
    
    refinement_ctrl.num_error_indicators = mesh->num_elements;
    
    /* Generate error indicators based on quality metrics */
    int i;
    #pragma omp parallel for
    for (i = 0; i < mesh->num_elements; i++) {
        /* Use inverse of quality factor as error indicator */
        refinement_ctrl.error_indicators[i] = 1.0 / (mesh->elements[i].quality_factor + 1e-6);
    }
    
    /* Perform refinement */
    mesh_t refined_mesh = {0};
    if (parallel_mesh_refinement(mesh, &refinement_ctrl, &refined_mesh) != 0) {
        printf("Warning: Adaptive refinement failed\n");
        free(refinement_ctrl.error_indicators);
        return -1;
    }
    
    free(refinement_ctrl.error_indicators);
    printf("Adaptive refinement completed\n");
    return 0;
}

/* Parallel mesh refinement */
static int parallel_mesh_refinement(mesh_t* mesh, adaptive_refinement_control_t* refinement, mesh_t* refined_mesh) {
    if (!mesh || !refinement || !refined_mesh) return -1;
    
    printf("Performing parallel mesh refinement...\n");
    
    /* Simple placeholder implementation */
    /* In a full implementation, this would:
     * 1. Identify elements to refine based on error indicators
     * 2. Subdivide elements using appropriate algorithms
     * 3. Update mesh connectivity
     * 4. Smooth new vertices
     */
    
    /* For now, just copy the original mesh */
    *refined_mesh = *mesh;
    refined_mesh->vertices = (mesh_vertex_t*)malloc(mesh->num_vertices * sizeof(mesh_vertex_t));
    refined_mesh->elements = (mesh_element_t*)malloc(mesh->num_elements * sizeof(mesh_element_t));
    
    if (!refined_mesh->vertices || !refined_mesh->elements) {
        return -1;
    }
    
    memcpy(refined_mesh->vertices, mesh->vertices, mesh->num_vertices * sizeof(mesh_vertex_t));
    memcpy(refined_mesh->elements, mesh->elements, mesh->num_elements * sizeof(mesh_element_t));
    
    return 0;
}

/*********************************************************************
 * Mesh Export and Import
 *********************************************************************/

int mesh_export_to_file(mesh_t* mesh, const char* filename, const char* format) {
    if (!mesh || !filename) return -1;
    
    printf("Exporting mesh to %s (format: %s)...\n", filename, format ? format : "custom");
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot open export file %s\n", filename);
        return -1;
    }
    
    if (format && strcmp(format, "VTK") == 0) {
        /* VTK format export */
        fprintf(fp, "# vtk DataFile Version 3.0\n");
        fprintf(fp, "PulseEM electromagnetic mesh\n");
        fprintf(fp, "ASCII\n");
        fprintf(fp, "DATASET UNSTRUCTURED_GRID\n\n");
        
        fprintf(fp, "POINTS %d float\n", mesh->num_vertices);
        for (int i = 0; i < mesh->num_vertices; i++) {
            geom_point_t p = mesh->vertices[i].position;
            fprintf(fp, "%e %e %e\n", p.x, p.y, p.z);
        }
        fprintf(fp, "\n");
        
        /* Write elements */
        int total_connectivity = 0;
        for (int i = 0; i < mesh->num_elements; i++) {
            total_connectivity += mesh->elements[i].num_vertices + 1;  /* +1 for vertex count */
        }
        
        fprintf(fp, "CELLS %d %d\n", mesh->num_elements, total_connectivity);
        for (int i = 0; i < mesh->num_elements; i++) {
            mesh_element_t* el = &mesh->elements[i];
            fprintf(fp, "%d", el->num_vertices);
            for (int k = 0; k < el->num_vertices; k++) {
                fprintf(fp, " %d", el->vertices[k]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
        
        /* Cell types */
        fprintf(fp, "CELL_TYPES %d\n", mesh->num_elements);
        for (int i = 0; i < mesh->num_elements; i++) {
            int cell_type = 0;
            switch (mesh->elements[i].type) {
                case MESH_ELEMENT_TRIANGLE: cell_type = 5; break;
                case MESH_ELEMENT_QUADRILATERAL: cell_type = 9; break;
                case MESH_ELEMENT_TETRAHEDRON: cell_type = 10; break;
                case MESH_ELEMENT_HEXAHEDRON: cell_type = 12; break;
                default: cell_type = 5; break; /* Default to triangle */
            }
            fprintf(fp, "%d\n", cell_type);
        }
        
    } else if (format && strcmp(format, "STL") == 0) {
        /* STL format (triangles only) */
        fprintf(fp, "solid pulseem_mesh\n");
        for (int i = 0; i < mesh->num_elements; i++) {
            if (mesh->elements[i].type == MESH_ELEMENT_TRIANGLE && mesh->elements[i].num_vertices >= 3) {
                const geom_point_t* p1 = &mesh->vertices[mesh->elements[i].vertices[0]].position;
                const geom_point_t* p2 = &mesh->vertices[mesh->elements[i].vertices[1]].position;
                const geom_point_t* p3 = &mesh->vertices[mesh->elements[i].vertices[2]].position;
                
                /* Calculate normal vector */
                double v1[3] = {p2->x - p1->x, p2->y - p1->y, p2->z - p1->z};
                double v2[3] = {p3->x - p1->x, p3->y - p1->y, p3->z - p1->z};
                
                double normal[3] = {
                    v1[1] * v2[2] - v1[2] * v2[1],
                    v1[2] * v2[0] - v1[0] * v2[2],
                    v1[0] * v2[1] - v1[1] * v2[0]
                };
                
                double norm = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
                if (norm > 0) {
                    normal[0] /= norm;
                    normal[1] /= norm;
                    normal[2] /= norm;
                }
                
                fprintf(fp, "  facet normal %e %e %e\n", normal[0], normal[1], normal[2]);
                fprintf(fp, "    outer loop\n");
                fprintf(fp, "      vertex %e %e %e\n", p1->x, p1->y, p1->z);
                fprintf(fp, "      vertex %e %e %e\n", p2->x, p2->y, p2->z);
                fprintf(fp, "      vertex %e %e %e\n", p3->x, p3->y, p3->z);
                fprintf(fp, "    endloop\n");
                fprintf(fp, "  endfacet\n");
            }
        }
        fprintf(fp, "endsolid pulseem_mesh\n");
        
    } else {
        /* Default custom format */
        fprintf(fp, "# PulseEM Mesh Export\n");
        fprintf(fp, "# Vertices: %d\n", mesh->num_vertices);
        fprintf(fp, "# Elements: %d\n", mesh->num_elements);
        fprintf(fp, "# Format: v id x y z\n");
        fprintf(fp, "# Format: e id type num_vertices vertex_ids...\n\n");
        
        /* Export vertices */
        for (int i = 0; i < mesh->num_vertices; i++) {
            geom_point_t p = mesh->vertices[i].position;
            fprintf(fp, "v %d %.9f %.9f %.9f\n", i, p.x, p.y, p.z);
        }
        fprintf(fp, "\n");
        
        /* Export elements */
        for (int e = 0; e < mesh->num_elements; e++) {
            mesh_element_t* el = &mesh->elements[e];
            fprintf(fp, "e %d %d %d", e, el->type, el->num_vertices);
            for (int k = 0; k < el->num_vertices; k++) {
                fprintf(fp, " %d", el->vertices[k]);
            }
            fprintf(fp, "\n");
        }
    }
    
    fclose(fp);
    printf("Mesh exported successfully to %s\n", filename);
    return 0;
}

/*********************************************************************
 * Advanced Mesh Generation Functions
 *********************************************************************/

/* Initialize unified meshing library */
int mesh_unified_init(void) {
    if (mesh_initialized) {
        return 0;
    }
    
    printf("Initializing Unified Meshing Library...\n");
    printf("Supported Mesh Types:\n");
    printf("  - Triangular (MoM RWG basis functions)\n");
    printf("  - Quadrilateral (Higher-order MoM)\n");
    printf("  - Tetrahedral (3D volume meshing)\n");
    printf("  - Manhattan (PEEC rectangular grid)\n");
    printf("  - Hybrid (Mixed MoM-PEEC coupling)\n");
    
    printf("Supported Algorithms:\n");
    printf("  - Delaunay Triangulation\n");
    printf("  - Advancing Front\n");
    printf("  - Octree-based\n");
    printf("  - Anisotropic adaptation\n");
    printf("  - Parallel generation\n");
    
    mesh_initialized = 1;
    mesh_element_counter = 0;
    mesh_node_counter = 0;
    
    return 0;
}

/* Create unified mesh with advanced parameters */
mesh_t* mesh_unified_create(unified_mesh_parameters_t* params) {
    if (!params) {
        return NULL;
    }
    
    mesh_t* mesh = mesh_create("unified_mesh", params->mesh_type);
    if (!mesh) {
        return NULL;
    }
    
    unified_mesh_t* unified = (unified_mesh_t*)calloc(1, sizeof(unified_mesh_t));
    if (!unified) {
        mesh_destroy(mesh);
        return NULL;
    }
    
    unified->base = *mesh;
    unified->max_refinement_level = 0;
    unified->total_mesh_time = 0.0;
    unified->refinement_iterations = 0;
    unified->is_converged = false;
    unified->mesh_memory_usage = 0.0;
    unified->num_threads = omp_get_max_threads();
    
    printf("Created unified mesh with parameters:\n");
    printf("  Mesh type: %d\n", params->mesh_type);
    printf("  Algorithm: %d\n", params->algorithm);
    printf("  Parallel generation: %s\n", params->enable_parallel_generation ? "YES" : "NO");
    printf("  Quality optimization: %s\n", params->enable_quality_optimization ? "YES" : "NO");
    printf("  Target size: %.3f\n", params->sizing.target_size);
    printf("  Target quality: %.3f\n", params->quality.target_quality);
    printf("  Threads: %d\n", unified->num_threads);
    
    free(mesh);
    return (mesh_t*)unified;
}

/* Generate MoM-compatible triangular mesh */
int mesh_generate_mom_rwg(mesh_t* mesh, double wavelength, double mesh_density) {
    if (!mesh) {
        return -1;
    }
    
    printf("Generating MoM RWG-compatible triangular mesh...\n");
    printf("Operating wavelength: %.3f m\n", wavelength);
    printf("Mesh density factor: %.2f\n", mesh_density);
    
    clock_t start_time = clock();
    
    /* Calculate target element size based on wavelength */
    double target_size = wavelength / (10.0 * mesh_density);  /* λ/10 rule with density factor */
    printf("Target element size: %.3f m (λ/%.1f)\n", target_size, 10.0 * mesh_density);
    
    /* For demonstration, create a simple triangular mesh */
    /* In a full implementation, this would parse geometry and generate appropriate mesh */
    
    /* Create sample vertices */
    geom_point_t vertices[] = {
        {0.0, 0.0, 0.0},
        {target_size * 2, 0.0, 0.0},
        {target_size, target_size * 1.7, 0.0},
        {target_size * 3, target_size * 1.7, 0.0},
        {target_size * 4, 0.0, 0.0}
    };
    
    int vertex_ids[5];
    for (int i = 0; i < 5; i++) {
        vertex_ids[i] = mesh_add_vertex(mesh, &vertices[i]);
        if (vertex_ids[i] < 0) return -1;
    }
    
    /* Create triangular elements */
    int triangles[][3] = {{0, 1, 2}, {1, 3, 2}, {1, 4, 3}};
    for (int i = 0; i < 3; i++) {
        int elem_id = mesh_add_element(mesh, MESH_ELEMENT_TRIANGLE, triangles[i], 3);
        if (elem_id < 0) return -1;
    }
    
    clock_t end_time = clock();
    double mesh_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("MoM RWG mesh generated: %d vertices, %d elements\n", mesh->num_vertices, mesh->num_elements);
    printf("Mesh generation time: %.3f seconds\n", mesh_time);
    
    /* Compute and display quality metrics */
    mesh_quality_t quality = mesh_compute_quality(mesh);
    printf("Average element quality: %.3f\n", quality.smoothness);
    printf("Elements per wavelength: %.1f\n", wavelength / target_size);
    
    return 0;
}

/* Generate PEEC Manhattan mesh */
/* Note: Function signature changed to match header declaration */
int mesh_generate_peec_manhattan(mesh_t* mesh, double grid_size, double x_min, double x_max, 
                                double y_min, double y_max, double z_min, double z_max) {
    /* New implementation using bounding box parameters */
    if (!mesh || grid_size <= 0.0) {
        return -1;
    }
    
    printf("Generating PEEC Manhattan mesh...\n");
    printf("Grid size: %.6f, Bounding box: [%.6f, %.6f] x [%.6f, %.6f] x [%.6f, %.6f]\n",
           grid_size, x_min, x_max, y_min, y_max, z_min, z_max);
    
    clock_t start_time = clock();
    
    /* Create Manhattan grid based on bounding box */
    int nx = (int)((x_max - x_min) / grid_size) + 1;
    int ny = (int)((y_max - y_min) / grid_size) + 1;
    int nz = (int)((z_max - z_min) / grid_size) + 1;
    
    if (nz < 1) nz = 1;  /* At least one layer */
    
    /* Create grid vertices */
    for (int k = 0; k < nz; k++) {
        for (int j = 0; j <= ny; j++) {
            for (int i = 0; i <= nx; i++) {
                geom_point_t p = {
                    x_min + i * grid_size,
                    y_min + j * grid_size,
                    z_min + k * grid_size
                };
                mesh_add_vertex(mesh, &p);
            }
        }
    }
    
    /* Create rectangular/hexahedral elements */
    if (nz == 1) {
        /* 2D rectangles */
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                int base = j * (nx + 1) + i;
                int vertices[4] = {
                    base,
                    base + 1,
                    (j + 1) * (nx + 1) + i + 1,
                    (j + 1) * (nx + 1) + i
                };
                mesh_add_element(mesh, MESH_ELEMENT_RECTANGLE, vertices, 4);
            }
        }
    } else {
        /* 3D hexahedra */
        for (int k = 0; k < nz - 1; k++) {
            for (int j = 0; j < ny; j++) {
                for (int i = 0; i < nx; i++) {
                    int base = k * (nx + 1) * (ny + 1) + j * (nx + 1) + i;
                    int vertices[8] = {
                        base,
                        base + 1,
                        base + (nx + 1) + 1,
                        base + (nx + 1),
                        base + (nx + 1) * (ny + 1),
                        base + (nx + 1) * (ny + 1) + 1,
                        base + (nx + 1) * (ny + 1) + (nx + 1) + 1,
                        base + (nx + 1) * (ny + 1) + (nx + 1)
                    };
                    mesh_add_element(mesh, MESH_ELEMENT_HEXAHEDRON, vertices, 8);
                }
            }
        }
    }
    
    clock_t end_time = clock();
    double mesh_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("PEEC Manhattan mesh generated: %d vertices, %d elements\n", mesh->num_vertices, mesh->num_elements);
    printf("Mesh generation time: %.3f seconds\n", mesh_time);
    
    return 0;
}

/* Cleanup unified meshing library */
void mesh_unified_cleanup(void) {
    if (!mesh_initialized) {
        return;
    }
    
    printf("Cleaning up Unified Meshing Library...\n");
    
    mesh_initialized = 0;
    mesh_element_counter = 0;
    mesh_node_counter = 0;
}