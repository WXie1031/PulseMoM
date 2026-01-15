/********************************************************************************
 *  PulseEM - Gmsh Surface Mesh Engine Header
 *
 *  3D surface mesh generation using Gmsh for MoM and PEEC applications
 *  Provides advanced surface meshing with CAD import, adaptive refinement,
 *  and high-quality triangle generation
 ********************************************************************************/

#ifndef GMSH_SURFACE_MESH_H
#define GMSH_SURFACE_MESH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Gmsh mesh element types */
typedef enum {
    GMSH_ELEMENT_TRIANGLE = 2,
    GMSH_ELEMENT_QUADRANGLE = 3,
    GMSH_ELEMENT_TETRAHEDRON = 4,
    GMSH_ELEMENT_HEXAHEDRON = 5,
    GMSH_ELEMENT_PRISM = 6,
    GMSH_ELEMENT_PYRAMID = 7,
    GMSH_ELEMENT_LINE = 1,
    GMSH_ELEMENT_POINT = 15
} gmsh_element_type_t;

/* Gmsh mesh algorithms */
typedef enum {
    GMSH_ALGORITHM_DELAUNAY = 1,
    GMSH_ALGORITHM_FRONTAL = 6,
    GMSH_ALGORITHM_BAMG = 7,
    GMSH_ALGORITHM_DELAQUAD = 8,
    GMSH_ALGORITHM_PACK = 9,
    GMSH_ALGORITHM_QUASI_STRUCT = 10
} gmsh_algorithm_t;

/* Gmsh mesh optimization */
typedef enum {
    GMSH_OPTIMIZATION_NONE = 0,
    GMSH_OPTIMIZATION_LAPLACE = 1,
    GMSH_OPTIMIZATION_HO_LAPLACE = 2,
    GMSH_OPTIMIZATION_HO_ELASTIC = 3,
    GMSH_OPTIMIZATION_HO_FAST_CURVING = 4
} gmsh_optimization_t;

/* Mesh parameters for Gmsh */
typedef struct {
    double element_size;
    double element_size_min;
    double element_size_max;
    double curvature_protection;
    double proximity_detection;
    
    /* Algorithm selection */
    gmsh_algorithm_t algorithm_2d;
    gmsh_algorithm_t algorithm_3d;
    gmsh_optimization_t optimization;
    
    /* Quality parameters */
    double min_angle;
    double max_angle;
    double aspect_ratio_max;
    double skewness_max;
    
    /* Refinement parameters */
    int refinement_levels;
    bool adaptive_refinement;
    double refinement_threshold;
    
    /* Electromagnetic specific */
    bool mom_compatible;
    bool peec_compatible;
    double frequency;
    double elements_per_wavelength;
    
    /* Surface mesh specific */
    bool surface_mesh_only;
    bool preserve_surface_curvature;
    bool surface_optimization;
    
    /* CAD import options */
    bool import_cad;
    double cad_tolerance;
    bool heal_geometry;
    bool remove_small_features;
    double small_feature_size;
    
    /* Performance options */
    int num_threads;
    bool parallel_meshing;
    int max_memory_mb;
    
    /* Output options */
    bool save_intermediate;
    bool verbose;
    int verbosity_level;
} gmsh_mesh_parameters_t;

/* Gmsh mesh result */
typedef struct {
    /* Mesh statistics */
    int num_vertices;
    int num_elements;
    int num_triangles;
    int num_quadrangles;
    int num_edges;
    int num_faces;
    
    /* Quality metrics */
    double min_quality;
    double avg_quality;
    double max_quality;
    int poor_quality_elements;
    double min_angle;
    double avg_angle;
    double max_angle;
    
    /* Performance metrics */
    double generation_time;
    int iterations;
    int refinement_steps;
    double memory_usage_mb;
    
    /* Compatibility flags */
    bool mom_compatible;
    bool peec_compatible;
    bool hybrid_compatible;
    
    /* Error information */
    int error_code;
    char error_message[256];
    
    /* Mesh data pointer (internal format) */
    void* mesh_data;
} gmsh_mesh_result_t;

/* Gmsh engine handle */
typedef struct gmsh_mesh_engine gmsh_mesh_engine_t;

/* Gmsh surface mesh engine API */

/**
 * @brief Create a Gmsh mesh engine instance
 * @return Engine handle or NULL on failure
 */
gmsh_mesh_engine_t* gmsh_mesh_engine_create(void);

/**
 * @brief Destroy a Gmsh mesh engine instance
 * @param engine Engine handle
 */
void gmsh_mesh_engine_destroy(gmsh_mesh_engine_t* engine);

/**
 * @brief Initialize Gmsh API
 * @param engine Engine handle
 * @return true on success, false on failure
 */
bool gmsh_mesh_engine_initialize(gmsh_mesh_engine_t* engine);

/**
 * @brief Generate 3D surface mesh using Gmsh
 * @param engine Engine handle
 * @param geometry Input geometry data
 * @param params Mesh parameters
 * @return Mesh result or NULL on failure
 */
gmsh_mesh_result_t* gmsh_generate_surface_mesh(
    gmsh_mesh_engine_t* engine,
    const void* geometry,
    const gmsh_mesh_parameters_t* params);

/**
 * @brief Generate 3D volume mesh using Gmsh
 * @param engine Engine handle
 * @param geometry Input geometry data
 * @param params Mesh parameters
 * @return Mesh result or NULL on failure
 */
gmsh_mesh_result_t* gmsh_generate_volume_mesh(
    gmsh_mesh_engine_t* engine,
    const void* geometry,
    const gmsh_mesh_parameters_t* params);

/**
 * @brief Generate MoM-optimized surface mesh
 * @param engine Engine handle
 * @param geometry Input geometry data
 * @param frequency Operating frequency
 * @param elements_per_wavelength Target density
 * @param params Additional parameters (can be NULL)
 * @return Mesh result or NULL on failure
 */
gmsh_mesh_result_t* gmsh_generate_mom_mesh(
    gmsh_mesh_engine_t* engine,
    const void* geometry,
    double frequency,
    double elements_per_wavelength,
    const gmsh_mesh_parameters_t* params);

/**
 * @brief Import CAD file and generate mesh
 * @param engine Engine handle
 * @param filename CAD file path
 * @param params Mesh parameters
 * @return Mesh result or NULL on failure
 */
gmsh_mesh_result_t* gmsh_import_and_mesh(
    gmsh_mesh_engine_t* engine,
    const char* filename,
    const gmsh_mesh_parameters_t* params);

/**
 * @brief Set mesh size field from external data
 * @param engine Engine handle
 * @param field_name Name of the size field
 * @param vertices Array of vertex coordinates
 * @param sizes Array of target sizes at vertices
 * @param num_vertices Number of vertices
 * @return true on success, false on failure
 */
bool gmsh_set_size_field(
    gmsh_mesh_engine_t* engine,
    const char* field_name,
    const double* vertices,
    const double* sizes,
    int num_vertices);

/**
 * @brief Apply adaptive refinement based on error estimator
 * @param engine Engine handle
 * @param error_estimator Error estimator function
 * @param max_error Maximum allowed error
 * @param max_iterations Maximum refinement iterations
 * @return true on success, false on failure
 */
bool gmsh_adaptive_refinement(
    gmsh_mesh_engine_t* engine,
    double (*error_estimator)(const double* coords, int element_id),
    double max_error,
    int max_iterations);

/**
 * @brief Get mesh quality statistics
 * @param engine Engine handle
 * @param result Mesh result
 * @return true on success, false on failure
 */
bool gmsh_assess_mesh_quality(
    gmsh_mesh_engine_t* engine,
    gmsh_mesh_result_t* result);

/**
 * @brief Optimize mesh quality
 * @param engine Engine handle
 * @param result Mesh result to optimize
 * @param optimization_type Type of optimization
 * @param num_iterations Number of optimization iterations
 * @return true on success, false on failure
 */
bool gmsh_optimize_mesh(
    gmsh_mesh_engine_t* engine,
    gmsh_mesh_result_t* result,
    gmsh_optimization_t optimization_type,
    int num_iterations);

/**
 * @brief Convert Gmsh mesh to internal format
 * @param engine Engine handle
 * @param gmsh_result Gmsh mesh result
 * @return Internal mesh structure or NULL on failure
 */
void* gmsh_convert_to_internal_format(
    gmsh_mesh_engine_t* engine,
    const gmsh_mesh_result_t* gmsh_result);

/**
 * @brief Get Gmsh version information
 * @param engine Engine handle
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 * @return true on success, false on failure
 */
bool gmsh_get_version(
    gmsh_mesh_engine_t* engine,
    int* major,
    int* minor,
    int* patch);

/**
 * @brief Get last error message
 * @param engine Engine handle
 * @return Error message string or NULL if no error
 */
const char* gmsh_mesh_get_error_string(gmsh_mesh_engine_t* engine);

/**
 * @brief Check if Gmsh API is available
 * @return true if Gmsh is available, false otherwise
 */
bool gmsh_mesh_is_available(void);

#ifdef __cplusplus
}
#endif

#endif /* GMSH_SURFACE_MESH_H */