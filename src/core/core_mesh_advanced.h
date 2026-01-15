/**
 * @file core_mesh_advanced.h
 * @brief Advanced meshing algorithms for commercial-grade electromagnetic simulation
 * @details High-performance mesh generation with adaptive refinement, quality optimization,
 *          and parallel processing similar to FEKO, EMX, and HFSS capabilities
 */

#ifndef CORE_MESH_ADVANCED_H
#define CORE_MESH_ADVANCED_H

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MESH_REFINEMENT_LEVELS 10
#define MAX_MESH_QUALITY_METRICS 20
#define MESH_QUALITY_TARGET 0.8
#define MESH_MIN_ANGLE_DEGREES 15.0
#define MESH_MAX_ANGLE_DEGREES 165.0
#define MESH_ASPECT_RATIO_MAX 10.0
#define MESH_SIZE_TRANSITION_RATIO 2.0

typedef enum {
    MESH_TYPE_TRIANGULAR = 0,      /* Triangular elements for MoM */
    MESH_TYPE_QUADRILATERAL = 1,   /* Quadrilateral elements */
    MESH_TYPE_TETRAHEDRAL = 2,     /* Tetrahedral elements for 3D */
    MESH_TYPE_HEXAHEDRAL = 3,      /* Hexahedral elements */
    MESH_TYPE_MANHATTAN = 4,       /* Manhattan rectangular for PEEC */
    MESH_TYPE_HYBRID = 5,          /* Mixed element types */
    MESH_TYPE_CURVILINEAR = 6,     /* Curved elements for curved surfaces */
    MESH_TYPE_HIGHER_ORDER = 7     /* Higher-order elements */
} advanced_mesh_type_t;

typedef enum {
    REFINEMENT_STRATEGY_UNIFORM = 0,
    REFINEMENT_STRATEGY_ADAPTIVE = 1,
    REFINEMENT_STRATEGY_GRADED = 2,
    REFINEMENT_STRATEGY_EDGE_BASED = 3,
    REFINEMENT_STRATEGY_ERROR_BASED = 4,
    REFINEMENT_STRATEGY_FEATURE_BASED = 5,
    REFINEMENT_STRATEGY_SINGULARITY = 6
} mesh_refinement_strategy_t;

typedef enum {
    QUALITY_MEASURE_SHAPE = 0,
    QUALITY_MEASURE_SKEWNESS = 1,
    QUALITY_MEASURE_ASPECT_RATIO = 2,
    QUALITY_MEASURE_WARPAGE = 3,
    QUALITY_MEASURE_ORTHOGONALITY = 4,
    QUALITY_MEASURE_CONDITION_NUMBER = 5,
    QUALITY_MEASURE_MIN_ANGLE = 6,
    QUALITY_MEASURE_MAX_ANGLE = 7,
    QUALITY_MEASURE_VOLUME = 8,
    QUALITY_MEASURE_SURFACE_AREA = 9
} mesh_quality_measure_t;

typedef enum {
    MESH_GENERATOR_DELAUNAY = 0,
    MESH_GENERATOR_ADVANCING_FRONT = 1,
    MESH_GENERATOR_OCTREE = 2,
    MESH_GENERATOR_VORONOI = 3,
    MESH_GENERATOR_GRID_BASED = 4,
    MESH_GENERATOR_SWEEP = 5,
    MESH_GENERATOR_MAPPING = 6
} mesh_generation_algorithm_t;

typedef struct {
    int element_id;
    int* node_ids;
    int num_nodes;
    int refinement_level;
    double quality_metric;
    double element_size;
    double* shape_functions;      /* For higher-order elements */
    int* neighbor_ids;
    int num_neighbors;
    bool is_curved;
    double curvature_radius;
    int physical_region;          /* Material or boundary region */
    bool is_active;               /* For adaptive refinement */
} advanced_mesh_element_t;

typedef struct {
    int node_id;
    double x, y, z;
    double* basis_functions;      /* For higher-order nodes */
    int* connected_elements;
    int num_connected_elements;
    double node_spacing;          /* Target element size */
    int boundary_marker;
    bool is_boundary_node;
    int refinement_level;
    double* metric_tensor;        /* For anisotropic meshing */
} advanced_mesh_node_t;

typedef struct {
    double minimum_angle;
    double maximum_angle;
    double aspect_ratio;
    double skewness;
    double warpage;
    double orthogonality;
    double condition_number;
    double shape_measure;
    double* quality_histogram;     /* Quality distribution */
    int histogram_bins;
    double average_quality;
    double worst_quality;
    double best_quality;
    int num_poor_elements;
    int num_good_elements;
} mesh_quality_statistics_t;

typedef struct {
    double target_size;
    double minimum_size;
    double maximum_size;
    double size_gradation;       /* Size transition rate */
    double* size_field;           /* Spatially varying size field */
    int* anisotropic_directions;  /* For anisotropic meshing */
    bool enable_adaptive_sizing;
    bool enable_curvature_adaptation;
    bool enable_feature_preservation;
    double feature_angle_threshold;
    double curvature_adaptation_factor;
} mesh_sizing_control_t;

typedef struct {
    mesh_refinement_strategy_t strategy;
    double refinement_threshold;
    double coarsening_threshold;
    int maximum_refinement_level;
    double* error_indicators;     /* For error-based refinement */
    double* solution_gradients;   /* For gradient-based refinement */
    int* refinement_flags;        /* Refinement markers */
    bool enable_h_adaptation;     /* h-refinement (element subdivision) */
    bool enable_p_adaptation;     /* p-refinement (order increase) */
    bool enable_r_adaptation;     /* r-refinement (node relocation) */
    double refinement_fraction;     /* Fraction of elements to refine */
    double coarsening_fraction;     /* Fraction of elements to coarsen */
} adaptive_refinement_control_t;

typedef struct {
    advanced_mesh_type_t mesh_type;
    mesh_generation_algorithm_t algorithm;
    mesh_sizing_control_t sizing;
    adaptive_refinement_control_t refinement;
    mesh_quality_statistics_t quality;
    bool enable_parallel_generation;
    bool enable_quality_optimization;
    bool enable_boundary_layer_meshing;
    bool enable_periodic_boundaries;
    int num_threads;
    double tolerance;
    int max_iterations;
    char geometry_kernel[32];     /* Geometry kernel type */
} advanced_mesh_parameters_t;

typedef struct {
    advanced_mesh_node_t* nodes;
    advanced_mesh_element_t* elements;
    int num_nodes;
    int num_elements;
    int num_boundary_nodes;
    int max_refinement_level;
    mesh_quality_statistics_t quality_stats;
    double* node_coordinates;       /* Compact coordinate array */
    int* element_connectivity;      /* Compact connectivity array */
    int* boundary_markers;          /* Boundary condition markers */
    double total_mesh_time;
    int refinement_iterations;
    bool is_converged;
    double mesh_memory_usage;       /* Memory usage in MB */
} advanced_mesh_t;

typedef struct {
    double* coordinates;            /* Node coordinates */
    int* triangles;               /* Triangle connectivity */
    int* quadrilaterals;          /* Quad connectivity */
    int* edges;                   /* Edge connectivity */
    int num_nodes;
    int num_triangles;
    int num_quadrilaterals;
    int num_edges;
    double* node_attributes;        /* Node attributes (size, spacing, etc.) */
    double* element_attributes;     /* Element attributes (quality, area, etc.) */
    int* node_markers;              /* Node boundary markers */
    int* element_markers;           /* Element region markers */
    double* metric_tensor;          /* Anisotropic metric tensor */
    int* refinement_tree;           /* Refinement hierarchy */
    int tree_depth;
} mesh_data_structure_t;

typedef struct {
    double* surface_coordinates;    /* Surface mesh coordinates */
    int* surface_triangles;       /* Surface triangle connectivity */
    int num_surface_nodes;
    int num_surface_triangles;
    double* volume_coordinates;     /* Volume mesh coordinates */
    int* volume_tetrahedra;       /* Volume tetrahedron connectivity */
    int num_volume_nodes;
    int num_volume_tetrahedra;
    double* boundary_layer_nodes; /* Boundary layer nodes */
    int* boundary_layer_elements; /* Boundary layer elements */
    int num_bl_nodes;
    int num_bl_elements;
    double* curvature_field;        /* Surface curvature field */
    double* thickness_field;        /* Boundary layer thickness */
    int* surface_classification;    /* Surface type classification */
} boundary_layer_mesh_t;

/**
 * @brief Initialize advanced meshing library
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_init(void);

/**
 * @brief Create advanced mesh with specified parameters
 * @param params Mesh generation parameters
 * @return Pointer to advanced mesh, NULL on failure
 */
advanced_mesh_t* advanced_mesh_create(advanced_mesh_parameters_t* params);

/**
 * @brief Destroy advanced mesh and free memory
 * @param mesh Advanced mesh to destroy
 */
void advanced_mesh_destroy(advanced_mesh_t* mesh);

/**
 * @brief Generate Delaunay triangulation mesh
 * @param coordinates Input point coordinates
 * @param num_points Number of input points
 * @param boundary_edges Boundary edge constraints
 * @param num_edges Number of boundary edges
 * @param params Mesh parameters
 * @param mesh Output mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_delaunay(double* coordinates, int num_points,
                                    int* boundary_edges, int num_edges,
                                    advanced_mesh_parameters_t* params,
                                    advanced_mesh_t* mesh);

/**
 * @brief Generate advancing front mesh
 * @param boundary_coordinates Boundary point coordinates
 * @param num_boundary_points Number of boundary points
 * @param params Mesh parameters
 * @param mesh Output mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_advancing_front(double* boundary_coordinates, int num_boundary_points,
                                         advanced_mesh_parameters_t* params,
                                         advanced_mesh_t* mesh);

/**
 * @brief Generate octree-based mesh
 * @param bounding_box Domain bounding box [xmin, ymin, zmin, xmax, ymax, zmax]
 * @param params Mesh parameters
 * @param mesh Output mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_octree(double* bounding_box, advanced_mesh_parameters_t* params,
                                advanced_mesh_t* mesh);

/**
 * @brief Perform adaptive mesh refinement
 * @param mesh Input mesh to refine
 * @param refinement_params Refinement control parameters
 * @param refined_mesh Output refined mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_adaptive_refinement(advanced_mesh_t* mesh,
                                    adaptive_refinement_control_t* refinement_params,
                                    advanced_mesh_t* refined_mesh);

/**
 * @brief Optimize mesh quality
 * @param mesh Input mesh to optimize
 * @param quality_target Target quality metric
 * @param max_optimization_iterations Maximum optimization iterations
 * @param optimized_mesh Output optimized mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_optimize_quality(advanced_mesh_t* mesh, double quality_target,
                                 int max_optimization_iterations,
                                 advanced_mesh_t* optimized_mesh);

/**
 * @brief Generate boundary layer mesh
 * @param surface_mesh Input surface mesh
 * @param thickness_function Boundary layer thickness function
 * @param num_layers Number of boundary layers
 * @param growth_ratio Layer growth ratio
 * @param bl_mesh Output boundary layer mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_boundary_layer(mesh_data_structure_t* surface_mesh,
                                         double (*thickness_function)(double, double, double),
                                         int num_layers, double growth_ratio,
                                         boundary_layer_mesh_t* bl_mesh);

/**
 * @brief Generate curved surface mesh for high-order elements
 * @param surface_definition Surface parameterization function
 * @param curvature_tolerance Curvature adaptation tolerance
 * @param params Mesh parameters
 * @param curved_mesh Output curved mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_curved_surface(double (*surface_definition)(double, double, double*),
                                        double curvature_tolerance,
                                        advanced_mesh_parameters_t* params,
                                        advanced_mesh_t* curved_mesh);

/**
 * @brief Calculate mesh quality statistics
 * @param mesh Input mesh
 * @param quality_stats Output quality statistics
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_calculate_quality(advanced_mesh_t* mesh, mesh_quality_statistics_t* quality_stats);

/**
 * @brief Check mesh quality against criteria
 * @param mesh Input mesh
 * @param quality_criteria Quality criteria to check
 * @param quality_report Output quality report
 * @return Number of quality violations, 0 if all criteria satisfied
 */
int advanced_mesh_check_quality(advanced_mesh_t* mesh, mesh_quality_statistics_t* quality_criteria,
                              char* quality_report);

/**
 * @brief Export mesh to various formats
 * @param mesh Input mesh
 * @param filename Output filename
 * @param format Output format ("VTK", "STL", "NASTRAN", "ABAQUS", "GMSH")
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_export(advanced_mesh_t* mesh, const char* filename, const char* format);

/**
 * @brief Import mesh from file
 * @param filename Input filename
 * @param format Input format
 * @param mesh Output mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_import(const char* filename, const char* format, advanced_mesh_t* mesh);

/**
 * @brief Convert mesh between different types
 * @param input_mesh Input mesh
 * @param target_type Target mesh type
 * @param output_mesh Output converted mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_convert_type(advanced_mesh_t* input_mesh, advanced_mesh_type_t target_type,
                               advanced_mesh_t* output_mesh);

/**
 * @brief Generate mesh for specific electromagnetic applications
 * @param application_type Application type ("ANTENNA", "WAVEGUIDE", "FILTER", "PACKAGE")
 * @param geometry_parameters Application-specific geometry parameters
 * @param params Mesh parameters
 * @param application_mesh Output application mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_for_application(const char* application_type,
                                         double* geometry_parameters,
                                         advanced_mesh_parameters_t* params,
                                         advanced_mesh_t* application_mesh);

/**
 * @brief Perform parallel mesh generation
 * @param domain_decomposition Domain decomposition information
 * @param params Mesh parameters
 * @param parallel_mesh Output parallel mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_parallel_generation(int* domain_decomposition, int num_domains,
                                      advanced_mesh_parameters_t* params,
                                      advanced_mesh_t* parallel_mesh);

/**
 * @brief Generate anisotropic mesh based on solution gradients
 * @param background_mesh Background mesh
 * @param solution_gradients Solution gradient field
 * @param metric_tensor Metric tensor field
 * @param params Mesh parameters
 * @param anisotropic_mesh Output anisotropic mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_anisotropic(advanced_mesh_t* background_mesh,
                                    double* solution_gradients,
                                    double* metric_tensor,
                                    advanced_mesh_parameters_t* params,
                                    advanced_mesh_t* anisotropic_mesh);

/**
 * @brief Cleanup advanced meshing library
 */
void advanced_mesh_cleanup(void);

/* Specialized functions for electromagnetic simulation */

/**
 * @brief Generate MoM-compatible triangular mesh with RWG basis functions
 * @param geometry_input Input geometry description
 * @param wavelength Operating wavelength for mesh sizing
 * @param mesh_density Mesh density factor
 * @param mom_mesh Output MoM mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_mom_rwg(double* geometry_input, double wavelength,
                                 double mesh_density, advanced_mesh_t* mom_mesh);

/**
 * @brief Generate PEEC-compatible Manhattan mesh with partial elements
 * @param layout_geometry PCB/IC layout geometry
 * @param layer_stackup Layer stackup information
 * @param via_definitions Via definitions
 * @param peec_mesh Output PEEC mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_peec_manhattan(double* layout_geometry,
                                         int* layer_stackup,
                                         int* via_definitions,
                                         advanced_mesh_t* peec_mesh);

/**
 * @brief Generate hybrid mesh for mixed MoM-PEEC simulation
 * @param mom_geometry MoM simulation geometry
 * @param peec_geometry PEEC simulation geometry
 * @param coupling_region Coupling region definition
 * @param hybrid_mesh Output hybrid mesh
 * @return 0 on success, -1 on failure
 */
int advanced_mesh_generate_hybrid_mom_peec(double* mom_geometry,
                                          double* peec_geometry,
                                          double* coupling_region,
                                          advanced_mesh_t* hybrid_mesh);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MESH_ADVANCED_H */