/**
 * @file core_mesh.h
 * @brief Unified mesh engine for MoM and PEEC solvers
 * @details Supports:
 *   - Triangular meshes: Used by both MoM and PEEC solvers
 *   - Manhattan rectangular meshes: Used by PEEC solver
 *   - Hybrid meshes: Mixed element types for MoM-PEEC coupling
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_MESH_H
#define CORE_MESH_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/core_common.h"
#include "../geometry/core_geometry.h"
// Note: geom_geometry_t is defined in core_geometry.h

#ifdef __cplusplus
extern "C" {
#endif

// Mesh element types
typedef enum {
    MESH_ELEMENT_VERTEX,
    MESH_ELEMENT_EDGE,
    MESH_ELEMENT_TRIANGLE,
    MESH_ELEMENT_QUADRILATERAL,
    MESH_ELEMENT_RECTANGLE,     // PEEC Manhattan
    MESH_ELEMENT_POLYGON,        // General polygon
    MESH_ELEMENT_TETRAHEDRON,
    MESH_ELEMENT_HEXAHEDRON,
    MESH_ELEMENT_PRISM,
    MESH_ELEMENT_PYRAMID
} mesh_element_type_t;

typedef enum {
    MESH_TYPE_TRIANGULAR,       // MoM surface mesh
    MESH_TYPE_MANHATTAN,        // PEEC rectangular grid
    MESH_TYPE_TETRAHEDRAL,      // Volume mesh
    MESH_TYPE_HYBRID            // Mixed element types
} mesh_type_t;

typedef enum {
    MESH_ALGORITHM_DELAUNAY,
    MESH_ALGORITHM_ADVANCING_FRONT,
    MESH_ALGORITHM_OCTREE,
    MESH_ALGORITHM_MANHATTAN_GRID,  // PEEC specific
    MESH_ALGORITHM_STRUCTURED,
    MESH_ALGORITHM_UNSTRUCTURED
} mesh_algorithm_t;

// Mesh quality metrics
typedef struct {
    double min_angle;           // Minimum angle in degrees
    double max_angle;           // Maximum angle in degrees
    double aspect_ratio;        // Longest/shortest edge ratio
    double skewness;           // Deviation from ideal shape
    double orthogonality;      // Angle between edges
    double smoothness;         // Gradient of element sizes
} mesh_quality_t;

// Element connectivity
typedef struct {
    int id;
    mesh_element_type_t type;
    int* vertices;            // Vertex indices
    int* edges;                // Edge indices (if applicable)
    int num_vertices;
    int num_edges;
    
    // Geometric properties
    double area;
    double volume;
    geom_point_t centroid;
    geom_point_t normal;      // For surface elements
    
    // Physical properties
    int material_id;
    int region_id;
    int domain_id;             // MoM or PEEC domain
    
    // Quality metrics
    double quality_factor;
    double characteristic_length;
    
    // Solver-specific data
    void* mom_data;           // MoM basis function data
    void* peec_data;          // PEEC partial element data
} mesh_element_t;

// Vertex definition
typedef struct {
    int id;
    geom_point_t position;
    bool is_boundary;
    bool is_interface;        // Between MoM and PEEC domains
    int* adjacent_elements;   // Element indices
    int num_adjacent_elements;
    
    // Physical properties
    double potential;         // For electrostatic problems
    double charge_density;    // For MoM
    double current_density;   // For PEEC
    
    // Solver-specific data
    void* mom_data;
    void* peec_data;
} mesh_vertex_t;

// Edge definition
typedef struct {
    int id;
    int vertex1_id;
    int vertex2_id;
    bool is_boundary;
    bool is_interface;
    double length;
    geom_point_t midpoint;
    geom_point_t tangent;
    
    // For PEEC wire models
    double radius;
    double resistance;
    double inductance;
    double capacitance;
    
    // Solver-specific data
    void* mom_data;           // RWG basis function
    void* peec_data;          // Partial element data
} mesh_edge_t;

// Mesh structure
typedef struct {
    char name[256];
    mesh_type_t type;
    mesh_algorithm_t algorithm;
    
    // Element data
    mesh_vertex_t* vertices;
    mesh_edge_t* edges;
    mesh_element_t* elements;
    int num_vertices;
    int num_edges;
    int num_elements;
    
    // Topology
    int** vertex_to_elements;     // Adjacency matrix
    int** element_to_elements;    // Element connectivity
    
    // Quality metrics
    mesh_quality_t quality;
    double min_element_size;
    double max_element_size;
    double average_element_size;
    
    // Domain information
    int num_mom_elements;
    int num_peec_elements;
    int num_interface_elements;
    
    // Bounding box
    geom_point_t min_bound;
    geom_point_t max_bound;
    
    // Solver interfaces
    void* mom_mesh_data;      // MoM-specific mesh data
    void* peec_mesh_data;     // PEEC-specific mesh data
    
    // Parallel decomposition
    int num_partitions;
    int* partition_offsets;
    int* partition_elements;
    
    // User data
    void* user_data;
} mesh_t;

// Mesh generation parameters
typedef struct {
    double element_size;          // Target element size
    double min_element_size;      // Minimum allowed size
    double max_element_size;      // Maximum allowed size
    double growth_rate;           // Size transition rate
    
    // Algorithm-specific parameters
    mesh_algorithm_t algorithm;
    mesh_type_t mesh_type;
    
    // Quality control
    double quality_threshold;     // Minimum quality factor
    int max_refinement_iterations;
    int smoothing_iterations;
    
    // Domain decomposition
    bool create_partitions;
    int num_partitions;
    
    // Frequency-dependent sizing
    double frequency;             // For electromagnetic sizing
    double wavelength_factor;     // Elements per wavelength
    
    // Solver-specific flags
    bool mom_enabled;
    bool peec_enabled;
    bool hybrid_enabled;
    
    // Parallel processing
    bool use_parallel;
    int num_threads;
    
    // Output control
    bool export_intermediate;
    bool validate_quality;
    bool compute_statistics;
} mesh_parameters_t;

// Manhattan grid parameters (PEEC specific)
typedef struct {
    double grid_size;             // Uniform grid spacing
    double x_min, x_max;         // X bounds
    double y_min, y_max;         // Y bounds
    double z_min, z_max;         // Z bounds
    bool uniform_grid;            // Force uniform spacing
    bool snap_to_grid;           // Snap geometry to grid
    double snap_tolerance;        // Snapping tolerance
} manhattan_parameters_t;

// Note: geom_geometry_t is defined in core_geometry.h (included above)

// Mesh generation request structure
typedef struct {
    geom_geometry_t* geometry;    // Geometry object
    double global_size;           // Global element size
    double min_size;              // Minimum element size
    double max_size;              // Maximum element size
    double min_quality;           // Minimum quality threshold
    int max_opt_iterations;       // Maximum optimization iterations
    int num_threads;              // Number of threads for parallel processing
    double frequency;             // Frequency for frequency-dependent sizing (Hz)
    double elements_per_wavelength; // Elements per wavelength for frequency-dependent sizing
    mesh_element_type_t preferred_type; // Preferred element type
} mesh_request_t;

// Mesh generation result structure
typedef struct {
    mesh_t* mesh;                 // Generated mesh
    int error_code;               // Error code (0 = success)
    char error_message[512];      // Error message
    mesh_quality_t quality;       // Mesh quality metrics
    double generation_time;       // Generation time in seconds
    int num_vertices;             // Number of vertices
    int num_elements;             // Number of elements
    int num_boundary_nodes;       // Number of boundary nodes
    double min_quality;           // Minimum quality value
    double avg_quality;           // Average quality value
    double max_quality;           // Maximum quality value
    int poor_quality_elements;   // Number of poor quality elements
    double memory_usage;          // Memory usage in MB
    bool mom_compatible;          // Compatible with MoM solver
    bool peec_compatible;         // Compatible with PEEC solver
    bool hybrid_compatible;       // Compatible with hybrid solver
} mesh_result_t;

/*********************************************************************
 * Core Mesh Functions
 *********************************************************************/

// Mesh creation and management
mesh_t* mesh_create(const char* name, mesh_type_t type);
void mesh_destroy(mesh_t* mesh);

// Mesh generation from geometry
mesh_t* mesh_generate_from_geometry(const geom_geometry_t* geometry, 
                                   const mesh_parameters_t* params);
mesh_t* mesh_generate_manhattan_grid(const geom_geometry_t* geometry,
                                    const manhattan_parameters_t* params);

// Element operations
int mesh_add_vertex(mesh_t* mesh, const geom_point_t* position);
int mesh_add_element(mesh_t* mesh, mesh_element_type_t type, 
                    const int* vertex_ids, int num_vertices);
mesh_vertex_t* mesh_get_vertex(mesh_t* mesh, int vertex_id);
mesh_element_t* mesh_get_element(mesh_t* mesh, int element_id);

// Quality assessment and improvement
mesh_quality_t mesh_compute_quality(const mesh_t* mesh);
int mesh_improve_quality(mesh_t* mesh, const mesh_quality_t* target_quality);
int mesh_smooth_laplacian(mesh_t* mesh, int iterations);
int mesh_refine_adaptive(mesh_t* mesh, double refinement_threshold);

// Domain decomposition
int mesh_partition_domains(mesh_t* mesh, int num_domains);
int mesh_get_domain_for_element(mesh_t* mesh, int element_id);
bool mesh_is_interface_element(mesh_t* mesh, int element_id);

// Solver interfaces
int mesh_export_to_mom(mesh_t* mesh, void* mom_solver);
int mesh_export_to_peec(mesh_t* mesh, void* peec_solver);
int mesh_export_to_file(mesh_t* mesh, const char* filename, const char* format);

// Manhattan grid operations (PEEC)
int mesh_convert_to_manhattan(mesh_t* mesh, const manhattan_parameters_t* params);
bool mesh_is_manhattan_grid(const mesh_t* mesh);
int mesh_get_manhattan_neighbors(mesh_t* mesh, int element_id, int* neighbors);

// Statistics and analysis
void mesh_compute_statistics(mesh_t* mesh);
int mesh_get_element_count_by_type(const mesh_t* mesh, mesh_element_type_t type);
double mesh_get_total_area(const mesh_t* mesh);
double mesh_get_total_volume(const mesh_t* mesh);

// Validation
bool mesh_validate_topology(const mesh_t* mesh);
bool mesh_validate_geometry(const mesh_t* mesh);
bool mesh_validate_quality(const mesh_t* mesh);

// Parallel operations
int mesh_distribute_parallel(mesh_t* mesh, int num_processes);
int mesh_gather_parallel(mesh_t* mesh);

/*********************************************************************
 * Unified Mesh Functions
 *********************************************************************/

// Forward declarations for unified mesh structures
typedef struct unified_mesh_parameters_t unified_mesh_parameters_t;
typedef struct unified_mesh_t unified_mesh_t;

// Unified mesh creation and management
int mesh_unified_init(void);
mesh_t* mesh_unified_create(unified_mesh_parameters_t* params);
void mesh_unified_destroy(mesh_t* mesh);

// Unified mesh generation
int mesh_generate_mom_rwg(mesh_t* mesh, double wavelength, double mesh_density);
int mesh_generate_peec_manhattan(mesh_t* mesh, double grid_size, double x_min, double x_max, 
                                double y_min, double y_max, double z_min, double z_max);

#ifdef __cplusplus
}
#endif

#endif // CORE_MESH_H