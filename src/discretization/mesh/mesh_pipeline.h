/**
 * @file core_mesh_pipeline.h
 * @brief Unified mesh generation pipeline configuration interface
 * @details Provides a unified interface for mesh generation across MoM and PEEC solvers
 *          while keeping solver-specific implementations in their respective directories.
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_MESH_PIPELINE_H
#define CORE_MESH_PIPELINE_H

#include "core_common.h"
#include "core_mesh.h"
#include "../../discretization/geometry/core_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

// Solver type enumeration
typedef enum {
    SOLVER_MOM = 0,        // Method of Moments solver
    SOLVER_PEEC = 1,       // Partial Element Equivalent Circuit solver
    SOLVER_HYBRID = 2,     // Hybrid MoM-PEEC solver
    SOLVER_UNKNOWN = -1
} solver_type_t;

// Mesh generation algorithm options
// Note: This enum extends mesh_algorithm_t from core_mesh.h
// Pipeline-specific algorithms that map to core mesh algorithms
typedef enum {
    MESH_PIPELINE_ALGORITHM_AUTO = 0,           // Automatic selection based on geometry
    MESH_PIPELINE_ALGORITHM_DELAUNAY = 1,       // Delaunay triangulation (maps to MESH_ALGORITHM_DELAUNAY)
    MESH_PIPELINE_ALGORITHM_ADVANCING_FRONT = 2, // Advancing front method (maps to MESH_ALGORITHM_ADVANCING_FRONT)
    MESH_PIPELINE_ALGORITHM_STRUCTURED_GRID = 3, // Structured rectangular grid (PEEC)
    MESH_PIPELINE_ALGORITHM_ADAPTIVE = 4,       // Adaptive refinement
    MESH_PIPELINE_ALGORITHM_CURVATURE_BASED = 5 // Curvature-based refinement
} mesh_pipeline_algorithm_t;

// Helper function to convert between pipeline and core algorithm enums
// Using static inline to avoid macro expansion issues
static inline mesh_algorithm_t mesh_pipeline_to_core_algorithm(mesh_pipeline_algorithm_t alg) {
    switch (alg) {
        case MESH_PIPELINE_ALGORITHM_DELAUNAY:
            return MESH_ALGORITHM_DELAUNAY;
        case MESH_PIPELINE_ALGORITHM_ADVANCING_FRONT:
            return MESH_ALGORITHM_ADVANCING_FRONT;
        case MESH_PIPELINE_ALGORITHM_STRUCTURED_GRID:
            return MESH_ALGORITHM_STRUCTURED;
        default:
            return MESH_ALGORITHM_UNSTRUCTURED;
    }
}

// Mesh quality requirements
typedef struct {
    double min_quality;            // Minimum element quality (0-1)
    double max_aspect_ratio;       // Maximum aspect ratio
    double min_angle;              // Minimum angle in degrees
    double max_angle;              // Maximum angle in degrees
    bool enforce_quality;          // Enforce quality constraints
} mesh_quality_requirements_t;

// Unified mesh pipeline configuration
typedef struct {
    // Basic configuration
    mesh_type_t type;                      // MESH_TYPE_TRIANGULAR, MESH_TYPE_MANHATTAN, etc.
    solver_type_t solver;                  // SOLVER_MOM, SOLVER_PEEC, SOLVER_HYBRID
    mesh_pipeline_algorithm_t algorithm;   // Mesh generation algorithm
    
    // Sizing parameters
    double target_size;                    // Target element size
    double min_size;                       // Minimum element size
    double max_size;                       // Maximum element size
    double wavelength;                     // Wavelength for frequency-based sizing (MoM)
    double grid_resolution[3];             // Grid resolution [x, y, z] (PEEC Manhattan)
    
    // Refinement options
    bool adaptive_refinement;              // Enable adaptive refinement
    int max_refinement_levels;             // Maximum refinement levels
    double refinement_threshold;           // Error threshold for refinement
    
    // Quality requirements
    mesh_quality_requirements_t quality;    // Quality constraints
    
    // Solver-specific parameters (passed as void* to avoid coupling)
    void* solver_specific_params;          // Solver-specific parameters
    void (*solver_params_free)(void*);     // Optional cleanup function
    
    // Geometry input
    void* geometry;                        // Geometry input (surface, volume, etc.)
    
    // Output options
    bool export_mesh;                     // Export mesh to file
    const char* export_filename;           // Export filename
    bool validate_mesh;                    // Validate mesh after generation
} mesh_pipeline_config_t;

// Pipeline generation result
typedef struct {
    mesh_t* mesh;                          // Generated mesh
    int status;                            // Status code (0 = success)
    double generation_time;                // Mesh generation time in seconds
    int num_elements;                      // Number of elements generated
    int num_vertices;                      // Number of vertices generated
    mesh_quality_requirements_t quality_stats; // Actual quality statistics
    char error_message[256];               // Error message if failed
} mesh_pipeline_result_t;

/*********************************************************************
 * Unified Pipeline Interface
 *********************************************************************/

/**
 * @brief Initialize mesh pipeline system
 * @return 0 on success, negative on error
 */
int mesh_pipeline_init(void);

/**
 * @brief Cleanup mesh pipeline system
 */
void mesh_pipeline_cleanup(void);

/**
 * @brief Generate mesh using unified pipeline
 * @param config Pipeline configuration
 * @param result Output result structure (must be allocated by caller)
 * @return 0 on success, negative on error
 * 
 * @note This function dispatches to solver-specific implementations
 *       based on config->solver and config->type. The actual mesh
 *       generation is performed by solver-specific functions that
 *       implement the pipeline interface.
 */
int mesh_pipeline_generate(const mesh_pipeline_config_t* config, 
                          mesh_pipeline_result_t* result);

/**
 * @brief Destroy pipeline result and free resources
 * @param result Result structure to destroy
 */
void mesh_pipeline_result_destroy(mesh_pipeline_result_t* result);

/**
 * @brief Validate mesh pipeline configuration
 * @param config Configuration to validate
 * @return 0 if valid, negative if invalid
 */
int mesh_pipeline_validate_config(const mesh_pipeline_config_t* config);

/**
 * @brief Get recommended mesh parameters for solver and geometry
 * @param solver Solver type
 * @param geometry_type Geometry type
 * @param frequency Operating frequency (for MoM)
 * @param config Output recommended configuration
 * @return 0 on success, negative on error
 */
int mesh_pipeline_get_recommended_config(solver_type_t solver,
                                        geom_element_type_t geometry_type,
                                        double frequency,
                                        mesh_pipeline_config_t* config);

/*********************************************************************
 * Solver-Specific Pipeline Functions (implemented in solver directories)
 *********************************************************************/

/**
 * @brief MoM-specific mesh pipeline generation
 * @param config Pipeline configuration
 * @param result Output result
 * @return 0 on success, negative on error
 * 
 * @note Implemented in discretization/mesh/triangular_mesh_solver.c
 */
int mesh_pipeline_generate_mom(const mesh_pipeline_config_t* config,
                               mesh_pipeline_result_t* result);

/**
 * @brief PEEC-specific mesh pipeline generation
 * @param config Pipeline configuration
 * @param result Output result
 * @return 0 on success, negative on error
 * 
 * @note Implemented in discretization/mesh/manhattan_mesh_peec.c or triangular_mesh_peec.c
 */
int mesh_pipeline_generate_peec(const mesh_pipeline_config_t* config,
                                mesh_pipeline_result_t* result);

/*********************************************************************
 * Helper Functions
 *********************************************************************/

/**
 * @brief Convert solver type to string
 */
const char* mesh_pipeline_solver_type_to_string(solver_type_t solver);

/**
 * @brief Convert mesh type to string
 */
const char* mesh_pipeline_mesh_type_to_string(mesh_type_t type);

/**
 * @brief Convert algorithm to string
 */
const char* mesh_pipeline_algorithm_to_string(mesh_pipeline_algorithm_t algorithm);

#ifdef __cplusplus
}
#endif

#endif // CORE_MESH_PIPELINE_H
