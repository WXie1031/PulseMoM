/********************************************************************************
 *  PulseEM - CGAL Mesh Engine Header
 *
 *  C interface to CGAL geometric algorithms for mesh generation
 *  Provides seamless integration with existing C-based mesh engine
 ********************************************************************************/

#ifndef CGAL_MESH_ENGINE_H
#define CGAL_MESH_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mesh_engine.h"
#include "core_geometry.h"
#include "core_mesh.h"

/* CGAL mesh generation capabilities */
typedef enum {
    CGAL_CAP_SURFACE_MESHING    = 0x01,
    CGAL_CAP_VOLUME_MESHING     = 0x02,
    CGAL_CAP_QUALITY_ASSESSMENT = 0x04,
    CGAL_CAP_OPTIMIZATION       = 0x08,
    CGAL_CAP_CONSTRAINED_MESH   = 0x10,
    CGAL_CAP_ADAPTIVE_REFINEMENT = 0x20,
    CGAL_CAP_ELECTROMAGNETIC    = 0x40,
    CGAL_CAP_ALL               = 0x7F
} cgal_capability_t;

/* CGAL mesh quality metrics */
typedef struct {
    double min_angle;           // Minimum angle (degrees)
    double max_angle;           // Maximum angle (degrees)
    double avg_angle;           // Average angle
    double min_dihedral;        // Minimum dihedral angle (3D)
    double max_dihedral;        // Maximum dihedral angle (3D)
    double avg_dihedral;        // Average dihedral angle (3D)
    double min_aspect_ratio;    // Minimum aspect ratio
    double max_aspect_ratio;    // Maximum aspect ratio
    double avg_aspect_ratio;    // Average aspect ratio
    double min_radius_ratio;    // Minimum radius ratio
    double max_radius_ratio;    // Maximum radius ratio
    double avg_radius_ratio;    // Average radius ratio
    double min_edge_length;     // Minimum edge length
    double max_edge_length;     // Maximum edge length
    double avg_edge_length;     // Average edge length
    double mesh_quality_index;  // Overall quality index (0-1)
    int num_poor_elements;      // Elements below quality threshold
    int num_invalid_elements;   // Invalid/inverted elements
} cgal_quality_stats_t;

/* CGAL mesh generation parameters */
typedef struct {
    /* Core meshing parameters */
    mesh_element_type_t element_type;
    mesh_generation_strategy_t strategy;
    
    /* CGAL-specific parameters */
    double min_angle_threshold;     // Minimum acceptable angle
    double max_angle_threshold;     // Maximum acceptable angle
    double max_aspect_ratio;        // Maximum aspect ratio
    double min_radius_ratio;        // Minimum radius ratio
    
    /* Quality optimization */
    bool enable_optimization;
    int optimization_iterations;
    double optimization_tolerance;
    
    /* Constrained meshing */
    bool enable_constraints;
    int num_constraints;
    int* constraint_edges;        // Edge pairs for constraints
    
    /* Adaptive refinement */
    bool enable_adaptivity;
    double refinement_threshold;
    int max_refinement_levels;
    
    /* Electromagnetic-specific */
    bool enable_em_adaptation;
    double frequency;               // Operating frequency
    double elements_per_wavelength;
    double skin_depth_factor;       // Skin depth consideration
    
    /* Performance */
    bool enable_parallel;
    int num_threads;
    bool enable_gpu;
    
    /* Robustness */
    bool use_exact_arithmetic;
    double geometric_tolerance;
    int max_retry_attempts;
    
} cgal_mesh_parameters_t;

/* CGAL mesh engine handle */
typedef struct cgal_mesh_engine cgal_mesh_engine_t;

/*********************************************************************
 * Engine Lifecycle
 *********************************************************************/

/* Create CGAL mesh engine */
cgal_mesh_engine_t* cgal_mesh_engine_create(void);

/* Destroy CGAL mesh engine */
void cgal_mesh_engine_destroy(cgal_mesh_engine_t* engine);

/* Get CGAL version information */
const char* cgal_mesh_engine_get_version(void);

/* Check CGAL capabilities */
int cgal_mesh_engine_get_capabilities(cgal_mesh_engine_t* engine);
int cgal_mesh_engine_check_capability(cgal_mesh_engine_t* engine, cgal_capability_t capability);

/*********************************************************************
 * Surface Mesh Generation
 *********************************************************************/

/* Generate triangular surface mesh using CGAL */
mesh_result_t* cgal_generate_triangular_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params
);

/* Generate quadrilateral surface mesh using CGAL */
mesh_result_t* cgal_generate_quadrilateral_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params
);

/* Generate mixed surface mesh using CGAL */
mesh_result_t* cgal_generate_mixed_surface_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params
);

/*********************************************************************
 * Volume Mesh Generation
 *********************************************************************/

/* Generate tetrahedral volume mesh using CGAL */
mesh_result_t* cgal_generate_tetrahedral_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params
);

/* Generate hexahedral volume mesh using CGAL */
mesh_result_t* cgal_generate_hexahedral_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params
);

/* Generate mixed volume mesh using CGAL */
mesh_result_t* cgal_generate_mixed_volume_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params
);

/*********************************************************************
 * Quality Assessment & Optimization
 *********************************************************************/

/* Assess mesh quality using CGAL */
int cgal_assess_mesh_quality(
    cgal_mesh_engine_t* engine,
    const mesh_mesh_t* mesh,
    cgal_quality_stats_t* quality_stats
);

/* Optimize mesh quality using CGAL */
int cgal_optimize_mesh_quality(
    cgal_mesh_engine_t* engine,
    mesh_mesh_t* mesh,
    const cgal_mesh_parameters_t* params
);

/* Validate mesh for electromagnetic simulation */
int cgal_validate_em_mesh(
    cgal_mesh_engine_t* engine,
    const mesh_mesh_t* mesh,
    double frequency,
    double accuracy_requirement
);

/*********************************************************************
 * Electromagnetic-Specific Functions
 *********************************************************************/

/* Generate MoM-optimized surface mesh */
mesh_result_t* cgal_generate_mom_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    double frequency,
    double elements_per_wavelength,
    const cgal_mesh_parameters_t* params
);

/* Generate PEEC-optimized volume mesh */
mesh_result_t* cgal_generate_peec_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    double grid_size,
    bool manhattan_constraints,
    const cgal_mesh_parameters_t* params
);

/* Generate hybrid MoM-PEEC mesh */
mesh_result_t* cgal_generate_hybrid_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    double frequency,
    mesh_solver_coupling_t coupling_mode,
    const cgal_mesh_parameters_t* params
);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

/* Convert mesh to CGAL format */
void* cgal_mesh_to_cgal_format(const mesh_mesh_t* mesh);

/* Convert CGAL mesh to internal format */
mesh_mesh_t* cgal_mesh_from_cgal_format(void* cgal_mesh);

/* Get CGAL mesh statistics */
int cgal_get_mesh_statistics(
    cgal_mesh_engine_t* engine,
    const mesh_mesh_t* mesh,
    mesh_statistics_t* stats
);

/* Export CGAL mesh to file */
int cgal_export_mesh(
    cgal_mesh_engine_t* engine,
    const mesh_mesh_t* mesh,
    const char* filename,
    mesh_file_format_t format
);

/* Import CGAL mesh from file */
mesh_mesh_t* cgal_import_mesh(
    cgal_mesh_engine_t* engine,
    const char* filename,
    mesh_file_format_t format
);

/*********************************************************************
 * Error Handling
 *********************************************************************/

/* Get last error code */
int cgal_mesh_engine_get_last_error(void);

/* Get error description */
const char* cgal_mesh_engine_get_error_string(int error_code);

/* Set verbosity level */
int cgal_mesh_engine_set_verbosity(cgal_mesh_engine_t* engine, int verbosity);

#ifdef __cplusplus
}
#endif

#endif /* CGAL_MESH_ENGINE_H */