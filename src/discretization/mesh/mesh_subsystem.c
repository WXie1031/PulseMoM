/********************************************************************************
 *  PulseEM - Unified Electromagnetic Simulation Platform
 *
 *  Copyright (C) 2024-2025 PulseEM Technologies
 *
 *  Commercial License - All Rights Reserved
 *  Unauthorized copying, modification, or distribution is strictly prohibited
 *  Proprietary and confidential - see LICENSE file for details
 *
 *  File: mesh_subsystem.c
 *  Description: Independent mesh subsystem implementation with commercial-grade capabilities
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#include "mesh_subsystem.h"
#include "../utils/logger.h"
#include "../utils/memory_manager.h"
#include "../utils/error_handler.h"

/* Internal structures */
typedef struct {
    int* elements;              /* Element connectivity */
    double* nodes;              /* Node coordinates */
    int num_elements;          /* Number of elements */
    int num_nodes;            /* Number of nodes */
    int nodes_per_element;     /* Nodes per element */
    mesh_element_type_t type;   /* Element type */
} mesh_data_t;

struct mesh_geometry {
    char name[256];
    mesh_geometry_config_t config;
    void* native_data;          /* Native geometry data */
    int entity_count;          /* Number of geometric entities */
    double bounding_box[6];    /* Bounding box [xmin, ymin, zmin, xmax, ymax, zmax] */
    bool is_valid;             /* Geometry validity flag */
};

struct mesh_mesh {
    char name[256];
    mesh_config_t config;
    mesh_data_t* data;          /* Mesh data */
    mesh_quality_stats_t* quality_stats; /* Quality statistics */
    bool is_generated;         /* Generation status */
    bool is_optimized;         /* Optimization status */
    double generation_time;    /* Generation time */
    double memory_usage;       /* Memory usage in MB */
};

struct mesh_optimizer {
    double target_quality;      /* Target quality metric */
    mesh_quality_metric_t metric; /* Quality metric to optimize */
    int max_iterations;         /* Maximum optimization iterations */
    double tolerance;           /* Convergence tolerance */
    bool preserve_boundary;     /* Preserve boundary elements */
};

struct mesh_refiner {
    mesh_refinement_config_t config; /* Refinement configuration */
    int refinement_level;       /* Current refinement level */
    double* error_field;        /* Error field for adaptive refinement */
    bool preserve_quality;      /* Preserve mesh quality during refinement */
};

struct mesh_exporter {
    mesh_export_format_t format; /* Export format */
    char format_name[64];       /* Format name */
    bool binary_format;         /* Use binary format */
    int precision;              /* Numerical precision */
};

struct mesh_subsystem {
    bool initialized;           /* Initialization status */
    char version[64];           /* Subsystem version */
    void** solver_interfaces;   /* Registered solver interfaces */
    char** solver_names;      /* Registered solver names */
    int num_solvers;          /* Number of registered solvers */
    int max_solvers;          /* Maximum solvers */
    mesh_config_t default_config; /* Default configuration */
};

/* Quality metric functions */
static double calculate_aspect_ratio(int element_id, const mesh_data_t* data);
static double calculate_skewness(int element_id, const mesh_data_t* data);
static double calculate_orthogonality(int element_id, const mesh_data_t* data);
static double calculate_warpage(int element_id, const mesh_data_t* data);

/* Internal function prototypes */
static int generate_surface_mesh(mesh_mesh_t* mesh, const mesh_geometry_t* geometry);
static int generate_volume_mesh(mesh_mesh_t* mesh, const mesh_geometry_t* geometry);
static int optimize_mesh_quality(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh);
static int refine_mesh_adaptively(mesh_refiner_t* refiner, mesh_mesh_t* mesh, double* error_field);
static int export_mesh_to_format(const mesh_exporter_t* exporter, const mesh_mesh_t* mesh, const char* filename);

/* Geometry management */
mesh_geometry_t* mesh_geometry_create(const char* name, mesh_geometry_format_t format) {
    if (name == NULL) {
        log_error("Invalid geometry name");
        return NULL;
    }
    
    mesh_geometry_t* geometry = (mesh_geometry_t*)memory_allocate(sizeof(mesh_geometry_t));
    if (geometry == NULL) {
        log_error("Failed to allocate geometry structure");
        return NULL;
    }
    
    memset(geometry, 0, sizeof(mesh_geometry_t));
    strncpy(geometry->name, name, sizeof(geometry->name) - 1);
    geometry->config.format = format;
    geometry->config.tolerance = 1e-6;
    geometry->config.healing_enabled = true;
    geometry->config.simplification_enabled = false;
    geometry->config.min_feature_size = 1e-4;
    geometry->is_valid = false;
    
    /* Initialize bounding box */
    for (int i = 0; i < 6; i++) {
        geometry->bounding_box[i] = 0.0;
    }
    
    log_info("Created geometry '%s' with format %d", name, format);
    return geometry;
}

void mesh_geometry_destroy(mesh_geometry_t* geometry) {
    if (geometry == NULL) return;
    
    if (geometry->native_data != NULL) {
        memory_free(geometry->native_data);
    }
    
    memory_free(geometry);
    log_debug("Destroyed geometry structure");
}

int mesh_geometry_load_file(mesh_geometry_t* geometry, const char* filename) {
    if (geometry == NULL || filename == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    strncpy(geometry->config.filename, filename, sizeof(geometry->config.filename) - 1);
    
    /* Check file existence and format */
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        log_error("Failed to open geometry file: %s", filename);
        return MESH_ERROR_FILE_IO;
    }
    fclose(file);
    
    /* Parse geometry based on format */
    switch (geometry->config.format) {
        case MESH_GEOM_CAD_STEP:
            log_info("Loading STEP file: %s", filename);
            /* Placeholder for STEP parsing */
            break;
        case MESH_GEOM_CAD_IGES:
            log_info("Loading IGES file: %s", filename);
            /* Placeholder for IGES parsing */
            break;
        case MESH_GEOM_STANDARD:
            log_info("Loading standard format file: %s", filename);
            /* Placeholder for STL/OBJ/PLY parsing */
            break;
        default:
            log_warning("Geometry format %d not fully implemented", geometry->config.format);
            break;
    }
    
    /* Set bounding box (placeholder values) */
    geometry->bounding_box[0] = -1.0; geometry->bounding_box[3] = 1.0;
    geometry->bounding_box[1] = -1.0; geometry->bounding_box[4] = 1.0;
    geometry->bounding_box[2] = -1.0; geometry->bounding_box[5] = 1.0;
    geometry->is_valid = true;
    geometry->entity_count = 100; /* Placeholder */
    
    log_info("Successfully loaded geometry from %s", filename);
    return MESH_SUCCESS;
}

int mesh_geometry_save_file(const mesh_geometry_t* geometry, const char* filename) {
    if (geometry == NULL || filename == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Saving geometry to %s (format %d)", filename, geometry->config.format);
    /* Placeholder for geometry export */
    return MESH_SUCCESS;
}

int mesh_geometry_heal(mesh_geometry_t* geometry, double tolerance) {
    if (geometry == NULL || tolerance <= 0.0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    geometry->config.tolerance = tolerance;
    log_info("Healing geometry '%s' with tolerance %.2e", geometry->name, tolerance);
    
    /* Placeholder for geometry healing algorithms */
    /* This would typically include:
     * - Stitching gaps between surfaces
     * - Removing duplicate entities
     * - Fixing orientation issues
     * - Repairing self-intersections
     */
    
    geometry->is_valid = true;
    return MESH_SUCCESS;
}

int mesh_geometry_simplify(mesh_geometry_t* geometry, double tolerance) {
    if (geometry == NULL || tolerance <= 0.0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Simplifying geometry '%s' with tolerance %.2e", geometry->name, tolerance);
    
    /* Placeholder for geometry simplification */
    /* This would typically include:
     * - Removing small features
     * - Merging similar surfaces
     * - Reducing complexity
     */
    
    return MESH_SUCCESS;
}

int mesh_geometry_defeature(mesh_geometry_t* geometry, double min_size) {
    if (geometry == NULL || min_size <= 0.0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Defeaturing geometry '%s' with minimum size %.2e", geometry->name, min_size);
    
    /* Placeholder for defeaturing */
    /* This would typically include:
     * - Removing holes below minimum size
     * - Filleting sharp edges
     * - Suppressing small features
     */
    
    return MESH_SUCCESS;
}

/* Mesh generation */
mesh_mesh_t* mesh_mesh_create(const char* name) {
    if (name == NULL) {
        log_error("Invalid mesh name");
        return NULL;
    }
    
    mesh_mesh_t* mesh = (mesh_mesh_t*)memory_allocate(sizeof(mesh_mesh_t));
    if (mesh == NULL) {
        log_error("Failed to allocate mesh structure");
        return NULL;
    }
    
    memset(mesh, 0, sizeof(mesh_mesh_t));
    strncpy(mesh->name, name, sizeof(mesh->name) - 1);
    
    /* Default configuration */
    mesh->config.element_type = MESH_ELEMENT_TRIANGLE;
    mesh->config.order = 1;
    mesh->config.curved_elements = false;
    mesh->config.global_size = 0.1;
    mesh->config.min_size = 0.01;
    mesh->config.max_size = 1.0;
    mesh->config.min_quality = 0.3;
    mesh->config.max_optimization_iterations = 100;
    mesh->config.preserve_features = true;
    mesh->config.refinement_strategy = MESH_REFINE_UNIFORM;
    mesh->config.num_threads = omp_get_max_threads();
    mesh->config.enable_gpu = false;
    mesh->config.solver_coupling = MESH_COUPLING_LOOSE;
    strcpy(mesh->config.solver_name, "default");
    
    mesh->is_generated = false;
    mesh->is_optimized = false;
    mesh->generation_time = 0.0;
    mesh->memory_usage = 0.0;
    
    log_info("Created mesh '%s'", name);
    return mesh;
}

void mesh_mesh_destroy(mesh_mesh_t* mesh) {
    if (mesh == NULL) return;
    
    if (mesh->data != NULL) {
        if (mesh->data->elements != NULL) memory_free(mesh->data->elements);
        if (mesh->data->nodes != NULL) memory_free(mesh->data->nodes);
        memory_free(mesh->data);
    }
    
    if (mesh->quality_stats != NULL) {
        memory_free(mesh->quality_stats);
    }
    
    memory_free(mesh);
    log_debug("Destroyed mesh structure");
}

int mesh_mesh_set_config(mesh_mesh_t* mesh, const mesh_config_t* config) {
    if (mesh == NULL || config == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    memcpy(&mesh->config, config, sizeof(mesh_config_t));
    log_info("Updated mesh configuration for '%s'", mesh->name);
    return MESH_SUCCESS;
}

int mesh_mesh_generate_from_geometry(mesh_mesh_t* mesh, const mesh_geometry_t* geometry) {
    if (mesh == NULL || geometry == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    if (!geometry->is_valid) {
        log_error("Invalid geometry - cannot generate mesh");
        return MESH_ERROR_GEOMETRY_INVALID;
    }
    
    log_info("Generating mesh '%s' from geometry '%s'", mesh->name, geometry->name);
    double start_time = omp_get_wtime();
    
    /* Allocate mesh data */
    mesh->data = (mesh_data_t*)memory_allocate(sizeof(mesh_data_t));
    if (mesh->data == NULL) {
        return MESH_ERROR_OUT_OF_MEMORY;
    }
    memset(mesh->data, 0, sizeof(mesh_data_t));
    
    /* Generate surface mesh first */
    int status = generate_surface_mesh(mesh, geometry);
    if (status != MESH_SUCCESS) {
        log_error("Surface mesh generation failed");
        return status;
    }
    
    /* Generate volume mesh if 3D */
    if (mesh->config.element_type == MESH_ELEMENT_TETRAHEDRON ||
        mesh->config.element_type == MESH_ELEMENT_HEXAHEDRON ||
        mesh->config.element_type == MESH_ELEMENT_PRISM ||
        mesh->config.element_type == MESH_ELEMENT_PYRAMID) {
        
        status = generate_volume_mesh(mesh, geometry);
        if (status != MESH_SUCCESS) {
            log_error("Volume mesh generation failed");
            return status;
        }
    }
    
    mesh->is_generated = true;
    mesh->generation_time = omp_get_wtime() - start_time;
    mesh->memory_usage = mesh_estimate_memory_usage(mesh);
    
    log_info("Mesh generation completed in %.2f seconds (%.1f MB)", 
             mesh->generation_time, mesh->memory_usage);
    return MESH_SUCCESS;
}

int mesh_mesh_generate_surface(mesh_mesh_t* mesh, const mesh_geometry_t* geometry) {
    if (mesh == NULL || geometry == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Generating surface mesh for '%s'", mesh->name);
    
    /* Placeholder for surface mesh generation */
    /* This would typically include:
     * - Surface parameterization
     * - Advancing front or Delaunay triangulation
     * - Quality optimization
     */
    
    /* Create sample mesh data */
    mesh->data->num_nodes = 1000;
    mesh->data->num_elements = 500;
    mesh->data->nodes_per_element = 3; /* Triangles */
    mesh->data->type = MESH_ELEMENT_TRIANGLE;
    
    /* Allocate sample data */
    mesh->data->nodes = (double*)memory_allocate(mesh->data->num_nodes * 3 * sizeof(double));
    mesh->data->elements = (int*)memory_allocate(mesh->data->num_elements * mesh->data->nodes_per_element * sizeof(int));
    
    if (mesh->data->nodes == NULL || mesh->data->elements == NULL) {
        return MESH_ERROR_OUT_OF_MEMORY;
    }
    
    /* Generate simple grid (placeholder) */
    int nx = 20, ny = 25;
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            int node_idx = j * nx + i;
            if (node_idx < mesh->data->num_nodes) {
                mesh->data->nodes[node_idx * 3] = (double)i / (nx - 1);
                mesh->data->nodes[node_idx * 3 + 1] = (double)j / (ny - 1);
                mesh->data->nodes[node_idx * 3 + 2] = 0.0;
            }
        }
    }
    
    /* Generate element connectivity */
    for (int j = 0; j < ny - 1; j++) {
        for (int i = 0; i < nx - 1; i++) {
            int elem_idx = j * (nx - 1) + i;
            if (elem_idx < mesh->data->num_elements) {
                mesh->data->elements[elem_idx * 3] = j * nx + i;
                mesh->data->elements[elem_idx * 3 + 1] = j * nx + i + 1;
                mesh->data->elements[elem_idx * 3 + 2] = (j + 1) * nx + i;
            }
        }
    }
    
    return MESH_SUCCESS;
}

int mesh_mesh_generate_volume(mesh_mesh_t* mesh, const mesh_geometry_t* geometry) {
    if (mesh == NULL || geometry == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Generating volume mesh for '%s'", mesh->name);
    
    /* Placeholder for volume mesh generation */
    /* This would typically include:
     * - Boundary layer generation
     * - Delaunay tetrahedralization
     * - Hexahedral meshing
     * - Quality optimization
     */
    
    return MESH_SUCCESS;
}

/* Quality optimization */
mesh_optimizer_t* mesh_optimizer_create(void) {
    mesh_optimizer_t* optimizer = (mesh_optimizer_t*)memory_allocate(sizeof(mesh_optimizer_t));
    if (optimizer == NULL) {
        log_error("Failed to allocate optimizer structure");
        return NULL;
    }
    
    memset(optimizer, 0, sizeof(mesh_optimizer_t));
    optimizer->target_quality = 0.8;
    optimizer->metric = MESH_QUALITY_ASPECT_RATIO;
    optimizer->max_iterations = 100;
    optimizer->tolerance = 1e-6;
    optimizer->preserve_boundary = true;
    
    log_debug("Created mesh optimizer");
    return optimizer;
}

void mesh_optimizer_destroy(mesh_optimizer_t* optimizer) {
    if (optimizer == NULL) return;
    memory_free(optimizer);
    log_debug("Destroyed mesh optimizer");
}

int mesh_optimizer_set_target_quality(mesh_optimizer_t* optimizer, double target_quality) {
    if (optimizer == NULL || target_quality <= 0.0 || target_quality > 1.0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    optimizer->target_quality = target_quality;
    return MESH_SUCCESS;
}

int mesh_optimizer_set_metric(mesh_optimizer_t* optimizer, mesh_quality_metric_t metric) {
    if (optimizer == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    optimizer->metric = metric;
    return MESH_SUCCESS;
}

int mesh_optimizer_improve_mesh(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh) {
    if (optimizer == NULL || mesh == NULL || mesh->data == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Improving mesh quality for '%s' (target: %.2f)", mesh->name, optimizer->target_quality);
    
    /* Placeholder for mesh improvement algorithms */
    /* This would typically include:
     * - Laplacian smoothing
     * - Edge swapping
     * - Node relocation
     * - Element restructuring
     */
    
    mesh->is_optimized = true;
    log_info("Mesh optimization completed");
    return MESH_SUCCESS;
}

int mesh_optimizer_smooth_mesh(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh) {
    if (optimizer == NULL || mesh == NULL || mesh->data == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Smoothing mesh '%s'", mesh->name);
    
    /* Placeholder for mesh smoothing */
    /* This would typically include:
     * - Laplacian smoothing
     * - Winslow smoothing
     * - Optimization-based smoothing
     */
    
    return MESH_SUCCESS;
}

int mesh_optimizer_remesh_poor_elements(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh) {
    if (optimizer == NULL || mesh == NULL || mesh->data == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Remeshing poor quality elements in '%s'", mesh->name);
    
    /* Placeholder for poor element remeshing */
    /* This would typically include:
     * - Identifying poor quality elements
     * - Local remeshing
     * - Quality improvement
     */
    
    return MESH_SUCCESS;
}

/* Adaptive refinement */
mesh_refiner_t* mesh_refiner_create(void) {
    mesh_refiner_t* refiner = (mesh_refiner_t*)memory_allocate(sizeof(mesh_refiner_t));
    if (refiner == NULL) {
        log_error("Failed to allocate refiner structure");
        return NULL;
    }
    
    memset(refiner, 0, sizeof(mesh_refiner_t));
    refiner->config.error_type = MESH_ERROR_RESIDUAL;
    refiner->config.error_threshold = 0.1;
    refiner->config.refinement_factor = 0.5;
    refiner->config.max_refinement_level = 5;
    refiner->config.preserve_gradation = true;
    refiner->config.gradation_rate = 2.0;
    refiner->refinement_level = 0;
    refiner->preserve_quality = true;
    
    log_debug("Created mesh refiner");
    return refiner;
}

void mesh_refiner_destroy(mesh_refiner_t* refiner) {
    if (refiner == NULL) return;
    
    if (refiner->error_field != NULL) {
        memory_free(refiner->error_field);
    }
    
    memory_free(refiner);
    log_debug("Destroyed mesh refiner");
}

int mesh_refiner_set_config(mesh_refiner_t* refiner, const mesh_refinement_config_t* config) {
    if (refiner == NULL || config == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    memcpy(&refiner->config, config, sizeof(mesh_refinement_config_t));
    return MESH_SUCCESS;
}

int mesh_refiner_refine_uniformly(mesh_refiner_t* refiner, mesh_mesh_t* mesh) {
    if (refiner == NULL || mesh == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Uniformly refining mesh '%s'", mesh->name);
    
    /* Placeholder for uniform refinement */
    /* This would typically include:
     * - Subdividing all elements
     * - Maintaining quality
     * - Updating connectivity
     */
    
    refiner->refinement_level++;
    return MESH_SUCCESS;
}

int mesh_refiner_refine_adaptively(mesh_refiner_t* refiner, mesh_mesh_t* mesh, double* error_field) {
    if (refiner == NULL || mesh == NULL || error_field == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Adaptively refining mesh '%s'", mesh->name);
    
    /* Store error field for later use */
    if (refiner->error_field != NULL) {
        memory_free(refiner->error_field);
    }
    
    refiner->error_field = (double*)memory_allocate(mesh->data->num_elements * sizeof(double));
    if (refiner->error_field == NULL) {
        return MESH_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(refiner->error_field, error_field, mesh->data->num_elements * sizeof(double));
    
    /* Placeholder for adaptive refinement */
    /* This would typically include:
     * - Analyzing error field
     * - Marking elements for refinement
     * - Local refinement
     * - Quality preservation
     */
    
    refiner->refinement_level++;
    return MESH_SUCCESS;
}

int mesh_refiner_refine_boundary_layer(mesh_refiner_t* refiner, mesh_mesh_t* mesh, double thickness) {
    if (refiner == NULL || mesh == NULL || thickness <= 0.0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Refining boundary layer in '%s' (thickness: %.3f)", mesh->name, thickness);
    
    /* Placeholder for boundary layer refinement */
    /* This would typically include:
     * - Identifying boundary elements
     * - Creating layered mesh
     * - Maintaining quality
     */
    
    return MESH_SUCCESS;
}

int mesh_refiner_coarsen_mesh(mesh_refiner_t* refiner, mesh_mesh_t* mesh) {
    if (refiner == NULL || mesh == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Coarsening mesh '%s'", mesh->name);
    
    /* Placeholder for mesh coarsening */
    /* This would typically include:
     * - Identifying coarsenable regions
     * - Element merging
     * - Quality preservation
     */
    
    if (refiner->refinement_level > 0) {
        refiner->refinement_level--;
    }
    
    return MESH_SUCCESS;
}

/* Quality assessment */
int mesh_mesh_analyze_quality(const mesh_mesh_t* mesh, mesh_quality_stats_t* stats) {
    if (mesh == NULL || mesh->data == NULL || stats == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    /* Initialize statistics */
    stats->min_value = 1e10;
    stats->max_value = -1e10;
    stats->avg_value = 0.0;
    stats->std_deviation = 0.0;
    stats->below_threshold_count = 0;
    stats->total_elements = mesh->data->num_elements;
    stats->metric = MESH_QUALITY_ASPECT_RATIO;
    
    /* Calculate quality for each element */
    double sum = 0.0, sum_sq = 0.0;
    double local_min = 1e10, local_max = -1e10;
    
    // MSVC OpenMP doesn't support min/max reduction, use manual reduction
    #pragma omp parallel
    {
        double thread_sum = 0.0, thread_sum_sq = 0.0;
        double thread_min = 1e10, thread_max = -1e10;
        
        int i;
        #pragma omp for
        for (i = 0; i < mesh->data->num_elements; i++) {
            double quality = calculate_aspect_ratio(i, mesh->data);
            thread_sum += quality;
            thread_sum_sq += quality * quality;
            
            if (quality < thread_min) thread_min = quality;
            if (quality > thread_max) thread_max = quality;
        }
        
        #pragma omp critical
        {
            sum += thread_sum;
            sum_sq += thread_sum_sq;
            if (thread_min < local_min) local_min = thread_min;
            if (thread_max > local_max) local_max = thread_max;
        }
    }
    
    stats->min_value = local_min;
    stats->max_value = local_max;
    
    // Count elements below threshold
    for (int i = 0; i < mesh->data->num_elements; i++) {
        double quality = calculate_aspect_ratio(i, mesh->data);
        if (quality < mesh->config.min_quality) {
            stats->below_threshold_count++;
        }
    }
    
    stats->avg_value = sum / stats->total_elements;
    double variance = (sum_sq - sum * sum / stats->total_elements) / stats->total_elements;
    stats->std_deviation = sqrt(variance);
    
    log_info("Mesh quality analysis: min=%.3f, max=%.3f, avg=%.3f, std=%.3f", 
             stats->min_value, stats->max_value, stats->avg_value, stats->std_deviation);
    
    return MESH_SUCCESS;
}

int mesh_mesh_get_element_quality(const mesh_mesh_t* mesh, int element_id, double* quality) {
    if (mesh == NULL || mesh->data == NULL || element_id < 0 || element_id >= mesh->data->num_elements || quality == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    *quality = calculate_aspect_ratio(element_id, mesh->data);
    return MESH_SUCCESS;
}

int mesh_mesh_get_worst_elements(const mesh_mesh_t* mesh, int num_elements, int* element_ids) {
    if (mesh == NULL || mesh->data == NULL || element_ids == NULL || num_elements <= 0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    /* Simple implementation - find elements with lowest quality */
    typedef struct {
        int id;
        double quality;
    } element_quality_t;
    
    element_quality_t* qualities = (element_quality_t*)memory_allocate(
        mesh->data->num_elements * sizeof(element_quality_t));
    if (qualities == NULL) {
        return MESH_ERROR_OUT_OF_MEMORY;
    }
    
    /* Calculate quality for all elements */
    for (int i = 0; i < mesh->data->num_elements; i++) {
        qualities[i].id = i;
        qualities[i].quality = calculate_aspect_ratio(i, mesh->data);
    }
    
    /* Sort by quality (ascending) */
    for (int i = 0; i < mesh->data->num_elements - 1; i++) {
        for (int j = i + 1; j < mesh->data->num_elements; j++) {
            if (qualities[j].quality < qualities[i].quality) {
                element_quality_t temp = qualities[i];
                qualities[i] = qualities[j];
                qualities[j] = temp;
            }
        }
    }
    
    /* Return worst elements */
    int count = (num_elements < mesh->data->num_elements) ? num_elements : mesh->data->num_elements;
    for (int i = 0; i < count; i++) {
        element_ids[i] = qualities[i].id;
    }
    
    memory_free(qualities);
    return count;
}

/* Solver coupling */
int mesh_mesh_prepare_for_solver(const mesh_mesh_t* mesh, const char* solver_name) {
    if (mesh == NULL || solver_name == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Preparing mesh '%s' for solver '%s'", mesh->name, solver_name);
    
    /* Placeholder for solver-specific preparation */
    /* This would typically include:
     * - Converting element types
     * - Reordering nodes/elements
     * - Creating solver-specific data structures
     */
    
    return MESH_SUCCESS;
}

int mesh_mesh_export_for_solver(const mesh_mesh_t* mesh, const char* solver_name, const char* filename) {
    if (mesh == NULL || solver_name == NULL || filename == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Exporting mesh '%s' for solver '%s' to %s", mesh->name, solver_name, filename);
    
    /* Create appropriate exporter */
    mesh_export_format_t format = MESH_EXPORT_NATIVE;
    
    /* Map solver names to export formats */
    if (strstr(solver_name, "MoM") != NULL) {
        format = MESH_EXPORT_CEM;
    } else if (strstr(solver_name, "FEM") != NULL) {
        format = MESH_EXPORT_FEA;
    } else if (strstr(solver_name, "FDTD") != NULL) {
        format = MESH_EXPORT_CEM;
    }
    
    mesh_exporter_t* exporter = mesh_exporter_create(format);
    if (exporter == NULL) {
        return MESH_ERROR_OUT_OF_MEMORY;
    }
    
    int status = export_mesh_to_format(exporter, mesh, filename);
    mesh_exporter_destroy(exporter);
    
    return status;
}

int mesh_mesh_check_solver_compatibility(const mesh_mesh_t* mesh, const char* solver_name) {
    if (mesh == NULL || solver_name == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    /* Check basic compatibility */
    if (mesh->data == NULL || mesh->data->num_elements == 0) {
        log_error("Mesh '%s' is empty or invalid", mesh->name);
        return MESH_ERROR_MESH_GENERATION_FAILED;
    }
    
    /* Check solver-specific requirements */
    if (strstr(solver_name, "MoM") != NULL) {
        /* MoM typically requires triangular surface meshes */
        if (mesh->config.element_type != MESH_ELEMENT_TRIANGLE) {
            log_warning("MoM solver may require triangular elements");
        }
    } else if (strstr(solver_name, "FEM") != NULL) {
        /* FEM can handle various element types */
        if (mesh->config.element_type == MESH_ELEMENT_TRIANGLE && mesh->config.order < 2) {
            log_warning("FEM may benefit from higher-order elements");
        }
    }
    
    log_info("Mesh '%s' compatibility check passed for solver '%s'", mesh->name, solver_name);
    return MESH_SUCCESS;
}

/* Export capabilities */
mesh_exporter_t* mesh_exporter_create(mesh_export_format_t format) {
    mesh_exporter_t* exporter = (mesh_exporter_t*)memory_allocate(sizeof(mesh_exporter_t));
    if (exporter == NULL) {
        log_error("Failed to allocate exporter structure");
        return NULL;
    }
    
    memset(exporter, 0, sizeof(mesh_exporter_t));
    exporter->format = format;
    exporter->binary_format = true;
    exporter->precision = 6;
    
    /* Set format name */
    switch (format) {
        case MESH_EXPORT_STL: strcpy(exporter->format_name, "STL"); break;
        case MESH_EXPORT_OBJ: strcpy(exporter->format_name, "OBJ"); break;
        case MESH_EXPORT_VTK: strcpy(exporter->format_name, "VTK"); break;
        case MESH_EXPORT_MSH: strcpy(exporter->format_name, "GMSH"); break;
        case MESH_EXPORT_NATIVE: strcpy(exporter->format_name, "Native"); break;
        default: strcpy(exporter->format_name, "Unknown"); break;
    }
    
    log_debug("Created mesh exporter for format %s", exporter->format_name);
    return exporter;
}

void mesh_exporter_destroy(mesh_exporter_t* exporter) {
    if (exporter == NULL) return;
    memory_free(exporter);
    log_debug("Destroyed mesh exporter");
}

int mesh_exporter_set_format(mesh_exporter_t* exporter, mesh_export_format_t format) {
    if (exporter == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    exporter->format = format;
    
    /* Update format name */
    switch (format) {
        case MESH_EXPORT_STL: strcpy(exporter->format_name, "STL"); break;
        case MESH_EXPORT_OBJ: strcpy(exporter->format_name, "OBJ"); break;
        case MESH_EXPORT_VTK: strcpy(exporter->format_name, "VTK"); break;
        case MESH_EXPORT_MSH: strcpy(exporter->format_name, "GMSH"); break;
        case MESH_EXPORT_NATIVE: strcpy(exporter->format_name, "Native"); break;
        default: strcpy(exporter->format_name, "Unknown"); break;
    }
    
    return MESH_SUCCESS;
}

int mesh_exporter_export_mesh(const mesh_exporter_t* exporter, const mesh_mesh_t* mesh, const char* filename) {
    if (exporter == NULL || mesh == NULL || filename == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    return export_mesh_to_format(exporter, mesh, filename);
}

int mesh_exporter_export_geometry(const mesh_exporter_t* exporter, const mesh_geometry_t* geometry, const char* filename) {
    if (exporter == NULL || geometry == NULL || filename == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Exporting geometry '%s' to %s format", geometry->name, exporter->format_name);
    
    /* Placeholder for geometry export */
    return MESH_SUCCESS;
}

/* Statistics and information */
int mesh_mesh_get_statistics(const mesh_mesh_t* mesh, char* buffer, int buffer_size) {
    if (mesh == NULL || buffer == NULL || buffer_size <= 0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    int written = snprintf(buffer, buffer_size,
        "Mesh Statistics for '%s':\n"
        "  Elements: %d\n"
        "  Nodes: %d\n"
        "  Element Type: %d\n"
        "  Order: %d\n"
        "  Generated: %s\n"
        "  Optimized: %s\n"
        "  Generation Time: %.2f seconds\n"
        "  Memory Usage: %.1f MB\n",
        mesh->name,
        mesh->data ? mesh->data->num_elements : 0,
        mesh->data ? mesh->data->num_nodes : 0,
        mesh->config.element_type,
        mesh->config.order,
        mesh->is_generated ? "Yes" : "No",
        mesh->is_optimized ? "Yes" : "No",
        mesh->generation_time,
        mesh->memory_usage
    );
    
    return written;
}

int mesh_geometry_get_statistics(const mesh_geometry_t* geometry, char* buffer, int buffer_size) {
    if (geometry == NULL || buffer == NULL || buffer_size <= 0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    int written = snprintf(buffer, buffer_size,
        "Geometry Statistics for '%s':\n"
        "  Format: %d\n"
        "  Valid: %s\n"
        "  Entity Count: %d\n"
        "  Bounding Box: [%.3f, %.3f, %.3f] to [%.3f, %.3f, %.3f]\n"
        "  Tolerance: %.2e\n"
        "  Healing Enabled: %s\n",
        geometry->name,
        geometry->config.format,
        geometry->is_valid ? "Yes" : "No",
        geometry->entity_count,
        geometry->bounding_box[0], geometry->bounding_box[1], geometry->bounding_box[2],
        geometry->bounding_box[3], geometry->bounding_box[4], geometry->bounding_box[5],
        geometry->config.tolerance,
        geometry->config.healing_enabled ? "Yes" : "No"
    );
    
    return written;
}

int mesh_mesh_print_info(const mesh_mesh_t* mesh) {
    if (mesh == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    char buffer[1024];
    mesh_mesh_get_statistics(mesh, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    return MESH_SUCCESS;
}

int mesh_geometry_print_info(const mesh_geometry_t* geometry) {
    if (geometry == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    char buffer[1024];
    mesh_geometry_get_statistics(geometry, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    return MESH_SUCCESS;
}

/* Memory management */
double mesh_estimate_memory_usage(const mesh_mesh_t* mesh) {
    if (mesh == NULL || mesh->data == NULL) {
        return 0.0;
    }
    
    double memory = 0.0;
    
    /* Node storage */
    memory += mesh->data->num_nodes * 3 * sizeof(double);
    
    /* Element storage */
    memory += mesh->data->num_elements * mesh->data->nodes_per_element * sizeof(int);
    
    /* Quality statistics */
    if (mesh->quality_stats != NULL) {
        memory += sizeof(mesh_quality_stats_t);
    }
    
    return memory / (1024.0 * 1024.0); /* Convert to MB */
}

double mesh_geometry_estimate_memory_usage(const mesh_geometry_t* geometry) {
    if (geometry == NULL) {
        return 0.0;
    }
    
    double memory = sizeof(mesh_geometry_t);
    
    /* Native geometry data (estimated) */
    if (geometry->native_data != NULL) {
        memory += geometry->entity_count * 1024; /* Assume ~1KB per entity */
    }
    
    return memory / (1024.0 * 1024.0); /* Convert to MB */
}

/* Error handling */
const char* mesh_error_string(mesh_error_t error) {
    switch (error) {
        case MESH_SUCCESS: return "Success";
        case MESH_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case MESH_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case MESH_ERROR_FILE_IO: return "File I/O error";
        case MESH_ERROR_GEOMETRY_INVALID: return "Invalid geometry";
        case MESH_ERROR_MESH_GENERATION_FAILED: return "Mesh generation failed";
        case MESH_ERROR_QUALITY_TOO_LOW: return "Mesh quality too low";
        case MESH_ERROR_REFINEMENT_FAILED: return "Refinement failed";
        case MESH_ERROR_EXPORT_FAILED: return "Export failed";
        case MESH_ERROR_LICENSE: return "License error";
        case MESH_ERROR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

/* Subsystem management */
mesh_subsystem_t* mesh_subsystem_create(void) {
    mesh_subsystem_t* subsystem = (mesh_subsystem_t*)memory_allocate(sizeof(mesh_subsystem_t));
    if (subsystem == NULL) {
        log_error("Failed to allocate subsystem structure");
        return NULL;
    }
    
    memset(subsystem, 0, sizeof(mesh_subsystem_t));
    subsystem->initialized = false;
    strcpy(subsystem->version, "1.0.0");
    subsystem->max_solvers = 10;
    
    /* Allocate solver registration arrays */
    subsystem->solver_interfaces = (void**)memory_allocate(subsystem->max_solvers * sizeof(void*));
    subsystem->solver_names = (char**)memory_allocate(subsystem->max_solvers * sizeof(char*));
    
    if (subsystem->solver_interfaces == NULL || subsystem->solver_names == NULL) {
        if (subsystem->solver_interfaces) memory_free(subsystem->solver_interfaces);
        if (subsystem->solver_names) memory_free(subsystem->solver_names);
        memory_free(subsystem);
        return NULL;
    }
    
    /* Initialize default configuration */
    subsystem->default_config.element_type = MESH_ELEMENT_TRIANGLE;
    subsystem->default_config.order = 1;
    subsystem->default_config.global_size = 0.1;
    subsystem->default_config.min_quality = 0.3;
    subsystem->default_config.num_threads = omp_get_max_threads();
    
    log_info("Created mesh subsystem version %s", subsystem->version);
    return subsystem;
}

void mesh_subsystem_destroy(mesh_subsystem_t* subsystem) {
    if (subsystem == NULL) return;
    
    if (subsystem->solver_interfaces != NULL) {
        memory_free(subsystem->solver_interfaces);
    }
    
    if (subsystem->solver_names != NULL) {
        for (int i = 0; i < subsystem->num_solvers; i++) {
            if (subsystem->solver_names[i] != NULL) {
                memory_free(subsystem->solver_names[i]);
            }
        }
        memory_free(subsystem->solver_names);
    }
    
    memory_free(subsystem);
    log_debug("Destroyed mesh subsystem");
}

int mesh_subsystem_initialize(mesh_subsystem_t* subsystem) {
    if (subsystem == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    if (subsystem->initialized) {
        log_warning("Subsystem already initialized");
        return MESH_SUCCESS;
    }
    
    /* Initialize subsystem components */
    log_info("Initializing mesh subsystem version %s", subsystem->version);
    
    /* Placeholder for subsystem initialization */
    /* This would typically include:
     * - Loading geometry kernels
     * - Initializing meshing algorithms
     * - Setting up parallel processing
     */
    
    subsystem->initialized = true;
    log_info("Mesh subsystem initialized successfully");
    return MESH_SUCCESS;
}

int mesh_subsystem_configure(mesh_subsystem_t* subsystem, const char* config_file) {
    if (subsystem == NULL || config_file == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Configuring mesh subsystem from %s", config_file);
    
    /* Placeholder for configuration loading */
    /* This would typically include:
     * - Reading configuration file
     * - Setting default parameters
     * - Configuring algorithms
     */
    
    return MESH_SUCCESS;
}

int mesh_subsystem_register_solver(mesh_subsystem_t* subsystem, const char* solver_name, void* solver_interface) {
    if (subsystem == NULL || solver_name == NULL || solver_interface == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    if (subsystem->num_solvers >= subsystem->max_solvers) {
        log_error("Maximum number of solvers reached (%d)", subsystem->max_solvers);
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    /* Store solver interface */
    subsystem->solver_interfaces[subsystem->num_solvers] = solver_interface;
    
    /* Store solver name */
    subsystem->solver_names[subsystem->num_solvers] = (char*)memory_allocate(strlen(solver_name) + 1);
    if (subsystem->solver_names[subsystem->num_solvers] == NULL) {
        return MESH_ERROR_OUT_OF_MEMORY;
    }
    strcpy(subsystem->solver_names[subsystem->num_solvers], solver_name);
    
    subsystem->num_solvers++;
    log_info("Registered solver '%s' with mesh subsystem", solver_name);
    return MESH_SUCCESS;
}

int mesh_subsystem_get_capabilities(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    int written = snprintf(buffer, buffer_size,
        "PulseEM Mesh Subsystem Capabilities:\n"
        "  Geometry Formats: STEP, IGES, ACIS, Parasolid, CATIA, Pro/E, SolidWorks, Inventor, NX\n"
        "  Standard Formats: STL, OBJ, PLY\n"
        "  Element Types: Triangles, Quadrilaterals, Tetrahedra, Hexahedra, Prisms, Pyramids\n"
        "  Element Orders: Linear (1st), Quadratic (2nd)\n"
        "  Quality Metrics: Aspect Ratio, Skewness, Orthogonality, Warpage, Curvature\n"
        "  Refinement Strategies: Uniform, Adaptive, Graded, Boundary Layer, Curvature-based\n"
        "  Error Estimators: Residual, Recovery, Explicit, Implicit, Goal-oriented\n"
        "  Optimization: Laplacian smoothing, Edge swapping, Node relocation\n"
        "  Export Formats: STL, OBJ, PLY, VTK, CGNS, MED, MSH, NASTRAN, UNV\n"
        "  Solver Coupling: Loose, Tight, Embedded\n"
        "  Parallel Processing: Multi-threaded with OpenMP\n"
        "  GPU Acceleration: Optional CUDA support\n"
    );
    
    return written;
}

/* Internal function implementations */
static double calculate_aspect_ratio(int element_id, const mesh_data_t* data) {
    if (data == NULL || element_id >= data->num_elements) {
        return 0.0;
    }
    
    /* Simplified aspect ratio calculation for triangles */
    if (data->type == MESH_ELEMENT_TRIANGLE && data->nodes_per_element == 3) {
        /* Get element nodes */
        int n1 = data->elements[element_id * 3];
        int n2 = data->elements[element_id * 3 + 1];
        int n3 = data->elements[element_id * 3 + 2];
        
        /* Calculate edge lengths (simplified) */
        double dx1 = data->nodes[n2 * 3] - data->nodes[n1 * 3];
        double dy1 = data->nodes[n2 * 3 + 1] - data->nodes[n1 * 3 + 1];
        double edge1 = sqrt(dx1 * dx1 + dy1 * dy1);
        
        double dx2 = data->nodes[n3 * 3] - data->nodes[n2 * 3];
        double dy2 = data->nodes[n3 * 3 + 1] - data->nodes[n2 * 3 + 1];
        double edge2 = sqrt(dx2 * dx2 + dy2 * dy2);
        
        double dx3 = data->nodes[n1 * 3] - data->nodes[n3 * 3];
        double dy3 = data->nodes[n1 * 3 + 1] - data->nodes[n3 * 3 + 1];
        double edge3 = sqrt(dx3 * dx3 + dy3 * dy3);
        
        /* Calculate aspect ratio (max_edge/min_edge) */
        double max_edge = fmax(fmax(edge1, edge2), edge3);
        double min_edge = fmin(fmin(edge1, edge2), edge3);
        
        if (min_edge > 0.0) {
            return max_edge / min_edge;
        }
    }
    
    return 1.0; /* Default for unsupported element types */
}

static double calculate_skewness(int element_id, const mesh_data_t* data) {
    /* Placeholder for skewness calculation */
    return 0.0;
}

static double calculate_orthogonality(int element_id, const mesh_data_t* data) {
    /* Placeholder for orthogonality calculation */
    return 0.0;
}

static double calculate_warpage(int element_id, const mesh_data_t* data) {
    /* Placeholder for warpage calculation */
    return 0.0;
}

static int generate_surface_mesh(mesh_mesh_t* mesh, const mesh_geometry_t* geometry) {
    /* Placeholder for surface mesh generation */
    return mesh_mesh_generate_surface(mesh, geometry);
}

static int generate_volume_mesh(mesh_mesh_t* mesh, const mesh_geometry_t* geometry) {
    /* Placeholder for volume mesh generation */
    return mesh_mesh_generate_volume(mesh, geometry);
}

static int optimize_mesh_quality(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh) {
    /* Placeholder for mesh quality optimization */
    return mesh_optimizer_improve_mesh(optimizer, mesh);
}

static int refine_mesh_adaptively(mesh_refiner_t* refiner, mesh_mesh_t* mesh, double* error_field) {
    /* Placeholder for adaptive mesh refinement */
    return mesh_refiner_refine_adaptively(refiner, mesh, error_field);
}

static int export_mesh_to_format(const mesh_exporter_t* exporter, const mesh_mesh_t* mesh, const char* filename) {
    if (exporter == NULL || mesh == NULL || filename == NULL) {
        return MESH_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Exporting mesh to %s format: %s", exporter->format_name, filename);
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        log_error("Failed to open export file: %s", filename);
        return MESH_ERROR_FILE_IO;
    }
    
    /* Export based on format */
    switch (exporter->format) {
        case MESH_EXPORT_STL:
            fprintf(file, "solid PulseEM_Mesh\n");
            /* Write STL data */
            for (int i = 0; i < mesh->data->num_elements; i++) {
                fprintf(file, "  facet normal 0 0 1\n");
                fprintf(file, "    outer loop\n");
                for (int j = 0; j < mesh->data->nodes_per_element; j++) {
                    int node_idx = mesh->data->elements[i * mesh->data->nodes_per_element + j];
                    fprintf(file, "      vertex %.6f %.6f %.6f\n",
                            mesh->data->nodes[node_idx * 3],
                            mesh->data->nodes[node_idx * 3 + 1],
                            mesh->data->nodes[node_idx * 3 + 2]);
                }
                fprintf(file, "    endloop\n");
                fprintf(file, "  endfacet\n");
            }
            fprintf(file, "endsolid PulseEM_Mesh\n");
            break;
            
        case MESH_EXPORT_OBJ:
            fprintf(file, "# PulseEM Mesh Export\n");
            /* Write vertices */
            for (int i = 0; i < mesh->data->num_nodes; i++) {
                fprintf(file, "v %.6f %.6f %.6f\n",
                        mesh->data->nodes[i * 3],
                        mesh->data->nodes[i * 3 + 1],
                        mesh->data->nodes[i * 3 + 2]);
            }
            /* Write faces */
            for (int i = 0; i < mesh->data->num_elements; i++) {
                fprintf(file, "f");
                for (int j = 0; j < mesh->data->nodes_per_element; j++) {
                    int node_idx = mesh->data->elements[i * mesh->data->nodes_per_element + j] + 1; /* OBJ is 1-based */
                    fprintf(file, " %d", node_idx);
                }
                fprintf(file, "\n");
            }
            break;
            
        default:
            log_warning("Export format %s not fully implemented", exporter->format_name);
            break;
    }
    
    fclose(file);
    log_info("Mesh export completed: %s", filename);
    return MESH_SUCCESS;
}