/********************************************************************************
 * Mesh Engine Interface
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Unified mesh generation engine for electromagnetic simulation
 ********************************************************************************/

#ifndef MESH_ENGINE_H
#define MESH_ENGINE_H

#include "../core/core_common.h"
#include "../core/core_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct mesh_engine mesh_engine_t;
typedef struct mesh_request mesh_request_t;
typedef struct mesh_result mesh_result_t;
/* mesh_t is defined in core_mesh.h */

// Use core mesh element definitions from core_mesh.h

// Mesh formats
typedef enum {
    MESH_FORMAT_STL,
    MESH_FORMAT_OBJ,
    MESH_FORMAT_OFF,
    MESH_FORMAT_PLY,
    MESH_FORMAT_GMSH,
    MESH_FORMAT_NETGEN,
    MESH_FORMAT_CGAL,
    MESH_FORMAT_TRIANGLE,
    MESH_FORMAT_TETGEN,
    MESH_FORMAT_CUSTOM
} mesh_format_t;

// Mesh strategies
typedef enum {
    MESH_STRATEGY_QUALITY,      // Optimize for quality
    MESH_STRATEGY_SPEED,        // Optimize for speed
    MESH_STRATEGY_ADAPTIVE,     // Adaptive refinement
    MESH_STRATEGY_UNIFORM       // Uniform sizing
} mesh_strategy_t;

// Mesh request structure
typedef struct mesh_request {
    const geom_geometry_t* geometry; // Input geometry
    mesh_format_t format;            // Input format
    bool mom_enabled;                // Enable MoM meshing
    bool peec_enabled;               // Enable PEEC meshing
    mesh_element_type_t preferred_type; // Preferred element type
    mesh_strategy_t strategy;        // Meshing strategy
    double target_quality;           // Target mesh quality
    double global_size;              // Global element size
    double min_size;                 // Minimum element size
    double max_size;                 // Maximum element size
    double min_quality;              // Minimum acceptable quality
    int max_opt_iterations;          // Max optimization iterations
    bool preserve_features;          // Preserve sharp features
    double frequency;                // Analysis frequency (Hz)
    double elements_per_wavelength;  // Elements per wavelength
    bool enable_adaptivity;          // Enable adaptive refinement
    int num_threads;                 // Number of threads
    bool validate_quality;           // Validate mesh quality
    bool compute_statistics;         // Compute mesh statistics
} mesh_request_t;

// Mesh result structure
typedef struct mesh_result {
    mesh_t* mesh;                   // Generated mesh
    int error_code;                 // Error code
    char error_message[256];        // Error message
    // Detailed statistics
    int num_vertices;
    int num_elements;
    int num_boundary_nodes;
    double min_quality;
    double avg_quality;
    double max_quality;
    int poor_quality_elements;
    double generation_time;         // Computation time (s)
    double memory_usage;            // Memory usage (MB)
    // Compatibility flags
    bool mom_compatible;
    bool peec_compatible;
    bool hybrid_compatible;
} mesh_result_t;

// Use core mesh structure mesh_t from core_mesh.h

// Mesh engine lifecycle
mesh_engine_t* mesh_engine_create(void);
void mesh_engine_destroy(mesh_engine_t* engine);

// Mesh generation
mesh_result_t* mesh_engine_generate(mesh_engine_t* engine, const mesh_request_t* request);
void mesh_result_destroy(mesh_result_t* result);

/* Engine-level wrappers are deprecated in favor of core_mesh.h APIs */

// Mesh I/O
int mesh_export_stl(const mesh_t* mesh, const char* filename);
int mesh_export_obj(const mesh_t* mesh, const char* filename);
int mesh_export_gmsh(const mesh_t* mesh, const char* filename);

// Utility functions
double mesh_compute_volume(const mesh_t* mesh);
double mesh_compute_surface_area(const mesh_t* mesh);
void mesh_compute_bounding_box(const mesh_t* mesh, point3d_t* min_corner, point3d_t* max_corner);

// RWG basis functions for MoM
int mesh_create_rwg_basis(const mesh_t* mesh, rwg_basis_t** basis, int* num_basis);

#ifdef __cplusplus
}
#endif

#endif // MESH_ENGINE_H
