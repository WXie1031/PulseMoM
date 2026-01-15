/********************************************************************************
 *  PulseEM - Unified Electromagnetic Simulation Platform
 *
 *  Copyright (C) 2024-2025 PulseEM Technologies
 *
 *  Commercial License - All Rights Reserved
 *  Unauthorized copying, modification, or distribution is strictly prohibited
 *  Proprietary and confidential - see LICENSE file for details
 *
 *  File: mesh_subsystem.h
 *  Description: Independent mesh subsystem with commercial-grade capabilities
 ********************************************************************************/

#ifndef MESH_SUBSYSTEM_H
#define MESH_SUBSYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include <complex.h>

/* Forward declarations */
typedef struct mesh_geometry mesh_geometry_t;
typedef struct mesh_mesh mesh_mesh_t;
typedef struct mesh_optimizer mesh_optimizer_t;
typedef struct mesh_refiner mesh_refiner_t;
typedef struct mesh_exporter mesh_exporter_t;
typedef struct mesh_subsystem mesh_subsystem_t;

/* Geometry types supported */
typedef enum {
    MESH_GEOM_CAD_STEP,      /* STEP format */
    MESH_GEOM_CAD_IGES,      /* IGES format */
    MESH_GEOM_CAD_ACIS,      /* ACIS format */
    MESH_GEOM_CAD_PARASOLID, /* Parasolid format */
    MESH_GEOM_CAD_CATIA,     /* CATIA format */
    MESH_GEOM_CAD_PROE,      /* Pro/ENGINEER format */
    MESH_GEOM_CAD_SOLIDWORKS,/* SolidWorks format */
    MESH_GEOM_CAD_INVENTOR,  /* Inventor format */
    MESH_GEOM_CAD_NX,        /* NX format */
    MESH_GEOM_STANDARD,      /* Standard formats: STL, OBJ, PLY */
    MESH_GEOM_NATIVE,        /* Native PulseEM format */
    MESH_GEOM_CUSTOM         /* Custom format */
} mesh_geometry_format_t;

/* Element types */
typedef enum {
    MESH_ELEMENT_TRIANGLE = 3,
    MESH_ELEMENT_QUADRILATERAL = 4,
    MESH_ELEMENT_TETRAHEDRON = 4,
    MESH_ELEMENT_HEXAHEDRON = 8,
    MESH_ELEMENT_PRISM = 6,
    MESH_ELEMENT_PYRAMID = 5,
    MESH_ELEMENT_EDGE = 2,
    MESH_ELEMENT_VERTEX = 1
} mesh_element_type_t;

/* Mesh quality metrics */
typedef enum {
    MESH_QUALITY_ASPECT_RATIO,
    MESH_QUALITY_SKEWNESS,
    MESH_QUALITY_ORTHOGONALITY,
    MESH_QUALITY_WARPAGE,
    MESH_QUALITY_PARALLELISM,
    MESH_QUALITY_CURVATURE,
    MESH_QUALITY_SIZE_TRANSITION,
    MESH_QUALITY_EDGE_RATIO,
    MESH_QUALITY_COLLAPSE_RATIO,
    MESH_QUALITY_VOLUME_RATIO
} mesh_quality_metric_t;

/* Refinement strategies */
typedef enum {
    MESH_REFINE_UNIFORM,
    MESH_REFINE_ADAPTIVE,
    MESH_REFINE_GRADED,
    MESH_REFINE_BOUNDARY_LAYER,
    MESH_REFINE_CURVATURE_BASED,
    MESH_REFINE_SOLUTION_BASED,
    MESH_REFINE_ERROR_BASED,
    MESH_REFINE_FEATURE_BASED
} mesh_refinement_strategy_t;

/* Error estimation types */
typedef enum {
    MESH_ERROR_RESIDUAL,
    MESH_ERROR_RECOVERY,
    MESH_ERROR_EXPLICIT,
    MESH_ERROR_IMPLICIT,
    MESH_ERROR_GOAL_ORIENTED
} mesh_error_estimator_t;

/* Solver coupling modes */
typedef enum {
    MESH_COUPLING_NONE,
    MESH_COUPLING_LOOSE,
    MESH_COUPLING_TIGHT,
    MESH_COUPLING_EMBEDDED
} mesh_solver_coupling_t;

/* Geometry structure */
typedef struct {
    char name[256];
    mesh_geometry_format_t format;
    char filename[1024];
    double tolerance;           /* Geometry tolerance */
    bool healing_enabled;       /* Enable geometry healing */
    bool simplification_enabled; /* Enable simplification */
    double min_feature_size;    /* Minimum feature size */
    int entity_count;          /* Number of geometric entities */
    void* native_handle;       /* Native geometry handle */
} mesh_geometry_config_t;

/* Mesh configuration */
typedef struct {
    /* Element configuration */
    mesh_element_type_t element_type;
    int order;                  /* Element order (1=linear, 2=quadratic) */
    bool curved_elements;       /* Enable curved elements */
    
    /* Size configuration */
    double global_size;         /* Global element size */
    double min_size;            /* Minimum element size */
    double max_size;            /* Maximum element size */
    double size_function;       /* Size function coefficient */
    
    /* Quality configuration */
    double min_quality;         /* Minimum acceptable quality */
    int max_optimization_iterations; /* Maximum optimization iterations */
    bool preserve_features;     /* Preserve geometric features */
    
    /* Refinement configuration */
    mesh_refinement_strategy_t refinement_strategy;
    mesh_error_estimator_t error_estimator;
    double refinement_threshold; /* Refinement threshold */
    int max_refinement_levels;   /* Maximum refinement levels */
    
    /* Parallel processing */
    int num_threads;            /* Number of threads */
    bool enable_gpu;           /* Enable GPU acceleration */
    
    /* Solver coupling */
    mesh_solver_coupling_t solver_coupling;
    char solver_name[64];       /* Target solver name */
} mesh_config_t;

/* Quality metrics structure */
typedef struct {
    double min_value;
    double max_value;
    double avg_value;
    double std_deviation;
    int below_threshold_count;
    int total_elements;
    mesh_quality_metric_t metric;
} mesh_quality_stats_t;

/* Refinement criteria */
typedef struct {
    mesh_error_estimator_t error_type;
    double error_threshold;
    double refinement_factor;
    int max_refinement_level;
    bool preserve_gradation;    /* Preserve size gradation */
    double gradation_rate;      /* Size gradation rate */
} mesh_refinement_config_t;

/* Export formats */
typedef enum {
    MESH_EXPORT_NATIVE,       /* Native PulseEM format */
    MESH_EXPORT_STL,          /* STL format */
    MESH_EXPORT_OBJ,          /* OBJ format */
    MESH_EXPORT_PLY,          /* PLY format */
    MESH_EXPORT_VTK,          /* VTK format */
    MESH_EXPORT_CGNS,         /* CGNS format */
    MESH_EXPORT_MED,          /* MED format (Salome) */
    MESH_EXPORT_MSH,          /* Gmsh MSH format */
    MESH_EXPORT_NAS,          /* NASTRAN format */
    MESH_EXPORT_UNV,          /* I-DEAS UNV format */
    MESH_EXPORT_CFD,          /* CFD format */
    MESH_EXPORT_FEA,          /* FEA format */
    MESH_EXPORT_CEM,          /* Computational EM format */
    MESH_EXPORT_CUSTOM        /* Custom format */
} mesh_export_format_t;

/* Function prototypes */

/* Geometry management */
mesh_geometry_t* mesh_geometry_create(const char* name, mesh_geometry_format_t format);
void mesh_geometry_destroy(mesh_geometry_t* geometry);
int mesh_geometry_load_file(mesh_geometry_t* geometry, const char* filename);
int mesh_geometry_save_file(const mesh_geometry_t* geometry, const char* filename);
int mesh_geometry_heal(mesh_geometry_t* geometry, double tolerance);
int mesh_geometry_simplify(mesh_geometry_t* geometry, double tolerance);
int mesh_geometry_defeature(mesh_geometry_t* geometry, double min_size);

/* Mesh generation */
mesh_mesh_t* mesh_mesh_create(const char* name);
void mesh_mesh_destroy(mesh_mesh_t* mesh);
int mesh_mesh_set_config(mesh_mesh_t* mesh, const mesh_config_t* config);
int mesh_mesh_generate_from_geometry(mesh_mesh_t* mesh, const mesh_geometry_t* geometry);
int mesh_mesh_generate_surface(mesh_mesh_t* mesh, const mesh_geometry_t* geometry);
int mesh_mesh_generate_volume(mesh_mesh_t* mesh, const mesh_geometry_t* geometry);

/* Quality optimization */
mesh_optimizer_t* mesh_optimizer_create(void);
void mesh_optimizer_destroy(mesh_optimizer_t* optimizer);
int mesh_optimizer_set_target_quality(mesh_optimizer_t* optimizer, double target_quality);
int mesh_optimizer_set_metric(mesh_optimizer_t* optimizer, mesh_quality_metric_t metric);
int mesh_optimizer_improve_mesh(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh);
int mesh_optimizer_smooth_mesh(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh);
int mesh_optimizer_remesh_poor_elements(mesh_optimizer_t* optimizer, mesh_mesh_t* mesh);

/* Adaptive refinement */
mesh_refiner_t* mesh_refiner_create(void);
void mesh_refiner_destroy(mesh_refiner_t* refiner);
int mesh_refiner_set_config(mesh_refiner_t* refiner, const mesh_refinement_config_t* config);
int mesh_refiner_refine_uniformly(mesh_refiner_t* refiner, mesh_mesh_t* mesh);
int mesh_refiner_refine_adaptively(mesh_refiner_t* refiner, mesh_mesh_t* mesh, double* error_field);
int mesh_refiner_refine_boundary_layer(mesh_refiner_t* refiner, mesh_mesh_t* mesh, double thickness);
int mesh_refiner_coarsen_mesh(mesh_refiner_t* refiner, mesh_mesh_t* mesh);

/* Quality assessment */
int mesh_mesh_analyze_quality(const mesh_mesh_t* mesh, mesh_quality_stats_t* stats);
int mesh_mesh_get_element_quality(const mesh_mesh_t* mesh, int element_id, double* quality);
int mesh_mesh_get_worst_elements(const mesh_mesh_t* mesh, int num_elements, int* element_ids);
int mesh_mesh_get_quality_histogram(const mesh_mesh_t* mesh, int num_bins, int* histogram);

/* Solver coupling */
int mesh_mesh_prepare_for_solver(const mesh_mesh_t* mesh, const char* solver_name);
int mesh_mesh_export_for_solver(const mesh_mesh_t* mesh, const char* solver_name, const char* filename);
int mesh_mesh_check_solver_compatibility(const mesh_mesh_t* mesh, const char* solver_name);

/* Export capabilities */
mesh_exporter_t* mesh_exporter_create(mesh_export_format_t format);
void mesh_exporter_destroy(mesh_exporter_t* exporter);
int mesh_exporter_set_format(mesh_exporter_t* exporter, mesh_export_format_t format);
int mesh_exporter_export_mesh(const mesh_exporter_t* exporter, const mesh_mesh_t* mesh, const char* filename);
int mesh_exporter_export_geometry(const mesh_exporter_t* exporter, const mesh_geometry_t* geometry, const char* filename);

/* Statistics and information */
int mesh_mesh_get_statistics(const mesh_mesh_t* mesh, char* buffer, int buffer_size);
int mesh_geometry_get_statistics(const mesh_geometry_t* geometry, char* buffer, int buffer_size);
int mesh_mesh_print_info(const mesh_mesh_t* mesh);
int mesh_geometry_print_info(const mesh_geometry_t* geometry);

/* Memory management */
double mesh_estimate_memory_usage(const mesh_mesh_t* mesh);
double mesh_geometry_estimate_memory_usage(const mesh_geometry_t* geometry);

/* Error handling */
typedef enum {
    MESH_SUCCESS = 0,
    MESH_ERROR_INVALID_ARGUMENT = -1,
    MESH_ERROR_OUT_OF_MEMORY = -2,
    MESH_ERROR_FILE_IO = -3,
    MESH_ERROR_GEOMETRY_INVALID = -4,
    MESH_ERROR_MESH_GENERATION_FAILED = -5,
    MESH_ERROR_QUALITY_TOO_LOW = -6,
    MESH_ERROR_REFINEMENT_FAILED = -7,
    MESH_ERROR_EXPORT_FAILED = -8,
    MESH_ERROR_LICENSE = -9,
    MESH_ERROR_INTERNAL = -99
} mesh_error_t;

const char* mesh_error_string(mesh_error_t error);

/* Subsystem management */
mesh_subsystem_t* mesh_subsystem_create(void);
void mesh_subsystem_destroy(mesh_subsystem_t* subsystem);
int mesh_subsystem_initialize(mesh_subsystem_t* subsystem);
int mesh_subsystem_configure(mesh_subsystem_t* subsystem, const char* config_file);
int mesh_subsystem_register_solver(mesh_subsystem_t* subsystem, const char* solver_name, void* solver_interface);
int mesh_subsystem_get_capabilities(char* buffer, int buffer_size);

#endif /* MESH_SUBSYSTEM_H */