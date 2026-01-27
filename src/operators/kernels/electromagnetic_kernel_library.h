/*********************************************************************
 * PEEC-MoM Unified Framework - Core Library
 * 
 * This module implements the shared infrastructure for both
 * PEEC and MoM solvers, providing common functionality and
 * optimized data structures.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef PEEC_MOM_CORE_H
#define PEEC_MOM_CORE_H

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Framework version and configuration
#define PEEC_MOM_VERSION_MAJOR 1
#define PEEC_MOM_VERSION_MINOR 0
#define PEEC_MOM_VERSION_PATCH 0

// Memory alignment for SIMD and GPU optimization
#define MEMORY_ALIGNMENT 64
#define CACHE_LINE_SIZE 64

// Precision settings
typedef double Real;
typedef double complex Complex;

// Forward declarations
typedef struct GeometryEngine GeometryEngine;
typedef struct MeshEngine MeshEngine;
typedef struct MaterialDatabase MaterialDatabase;
typedef struct LinearAlgebraEngine LinearAlgebraEngine;
typedef struct IOEngine IOEngine;
typedef struct Framework Framework;

/*********************************************************************
 * Error Handling and Logging
 *********************************************************************/
typedef enum {
    SUCCESS = 0,
    ERROR_MEMORY_ALLOCATION = -1,
    ERROR_INVALID_ARGUMENT = -2,
    ERROR_FILE_NOT_FOUND = -3,
    ERROR_FILE_FORMAT = -4,
    ERROR_NUMERICAL_INSTABILITY = -5,
    ERROR_CONVERGENCE_FAILURE = -6,
    ERROR_PARALLEL_EXECUTION = -7,
    ERROR_GPU_EXECUTION = -8,
    ERROR_LICENSE_VIOLATION = -9,
    ERROR_UNKNOWN = -99
} StatusCode;

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} LogLevel;

typedef void (*LogCallback)(LogLevel level, const char* message, void* user_data);

typedef struct {
    LogLevel min_level;
    LogCallback callback;
    void* user_data;
    bool enable_file_logging;
    char log_file_path[1024];
} LoggingConfig;

/*********************************************************************
 * Geometry Engine - Shared CAD Interface
 *********************************************************************/
typedef enum {
    GEOMETRY_FORMAT_STEP,
    GEOMETRY_FORMAT_IGES,
    GEOMETRY_FORMAT_OBJ,
    GEOMETRY_FORMAT_STL,
    GEOMETRY_FORMAT_OFF,
    GEOMETRY_FORMAT_PLY,
    GEOMETRY_FORMAT_GMSH,
    GEOMETRY_FORMAT_CUSTOM
} GeometryFormat;

typedef enum {
    ENTITY_TYPE_VERTEX,
    ENTITY_TYPE_EDGE,
    ENTITY_TYPE_FACE,
    ENTITY_TYPE_SOLID,
    ENTITY_TYPE_SHELL,
    ENTITY_TYPE_WIRE,
    ENTITY_TYPE_COMPOUND
} EntityType;

typedef struct {
    Real x, y, z;
} Point3D;

typedef struct {
    Point3D center;
    Real radius;
} BoundingSphere;

typedef struct {
    Point3D min_corner;
    Point3D max_corner;
} BoundingBox;

typedef struct {
    int entity_id;
    EntityType type;
    BoundingBox bbox;
    void* native_handle;  // Platform-specific handle
    int material_id;
    int physical_group;
    bool is_valid;
} GeometryEntity;

typedef struct {
    // Geometry data
    GeometryEntity* entities;
    int num_entities;
    int max_entities;
    
    // Spatial indexing
    void* spatial_index;  // R-tree or Octree
    
    // Validation and healing
    bool enable_validation;
    bool enable_healing;
    Real tolerance;
    
    // Thread safety
    void* mutex;
} GeometryEngineConfig;

// Geometry engine functions
GeometryEngine* geometry_engine_create(const GeometryEngineConfig* config);
void geometry_engine_destroy(GeometryEngine* engine);

StatusCode geometry_engine_import_file(GeometryEngine* engine, 
                                     const char* filename, 
                                     GeometryFormat format);
StatusCode geometry_engine_export_file(GeometryEngine* engine,
                                     const char* filename,
                                     GeometryFormat format);

int geometry_engine_get_num_entities(GeometryEngine* engine, EntityType type);
GeometryEntity* geometry_engine_get_entity(GeometryEngine* engine, int entity_id);
StatusCode geometry_engine_get_bounding_box(GeometryEngine* engine, BoundingBox* bbox);

// Geometry operations
StatusCode geometry_engine_boolean_union(GeometryEngine* engine,
                                         int entity_id1, int entity_id2,
                                         int* result_id);
StatusCode geometry_engine_boolean_intersection(GeometryEngine* engine,
                                              int entity_id1, int entity_id2,
                                              int* result_id);
StatusCode geometry_engine_boolean_difference(GeometryEngine* engine,
                                            int entity_id1, int entity_id2,
                                            int* result_id);

// Feature recognition
StatusCode geometry_engine_recognize_features(GeometryEngine* engine);
bool geometry_engine_is_planar_face(GeometryEngine* engine, int face_id);
bool geometry_engine_is_cylindrical_face(GeometryEngine* engine, int face_id);

/*********************************************************************
 * Mesh Engine - Unified Mesh Generation
 *********************************************************************/
typedef enum {
    MESH_TYPE_TRIANGULAR,
    MESH_TYPE_QUADRILATERAL,
    MESH_TYPE_TETRAHEDRAL,
    MESH_TYPE_HEXAHEDRAL,
    MESH_TYPE_WIRE,           // For PEEC
    MESH_TYPE_SURFACE,        // For MoM
    MESH_TYPE_VOLUME,         // For FEM
    MESH_TYPE_MIXED
} MeshType;

typedef enum {
    MESH_ALGORITHM_DELAUNAY,
    MESH_ALGORITHM_ADVANCING_FRONT,
    MESH_ALGORITHM_OCTREE,
    MESH_ALGORITHM_QUADTREE,
    MESH_ALGORITHM_WATSON,
    MESH_ALGORITHM_RUPPERT,
    MESH_ALGORITHM_CHEW
} MeshAlgorithm;

typedef struct {
    int node_id;
    Point3D coordinates;
    int* connected_elements;
    int num_connected_elements;
    int physical_group;
    int boundary_flag;
    Real mesh_size;
} MeshNode;

typedef struct {
    int element_id;
    int* node_ids;
    int num_nodes;
    MeshType type;
    int physical_group;
    int material_id;
    Real area;
    Real volume;
    Real quality_metric;
} MeshElement;

typedef struct {
    Real target_size;
    Real minimum_size;
    Real maximum_size;
    Real growth_rate;
    Real curvature_resolution;
    Real feature_angle;
    int adaptive_levels;
    int max_iterations;
    Real quality_threshold;
    bool use_curvature_adaptation;
    bool use_feature_detection;
    bool use_parallel_meshing;
    int num_threads;
} MeshParameters;

typedef struct {
    Real aspect_ratio;
    Real skewness;
    Real orthogonality;
    Real minimum_angle;
    Real maximum_angle;
    Real condition_number;
    Real overall_quality;
    int num_poor_elements;
    int num_good_elements;
    int num_excellent_elements;
} MeshQualityMetrics;

typedef struct {
    MeshNode* nodes;
    int num_nodes;
    int max_nodes;
    
    MeshElement* elements;
    int num_elements;
    int max_elements;
    
    MeshQualityMetrics quality;
    
    // Connectivity information
    int** node_to_elements;
    int** element_to_elements;
    
    // Parallel meshing support
    int num_partitions;
    int* partition_offsets;
} MeshData;

// Mesh engine functions
MeshEngine* mesh_engine_create(const MeshParameters* params);
void mesh_engine_destroy(MeshEngine* engine);

StatusCode mesh_engine_generate_surface_mesh(MeshEngine* engine,
                                           GeometryEngine* geometry,
                                           MeshType type,
                                           MeshData** mesh);
StatusCode mesh_engine_generate_wire_mesh(MeshEngine* engine,
                                        GeometryEngine* geometry,
                                        MeshData** mesh);
StatusCode mesh_engine_adapt_mesh(MeshEngine* engine,
                              MeshData* mesh,
                              Real error_threshold);

StatusCode mesh_engine_refine_mesh(MeshEngine* engine,
                                 MeshData* mesh,
                               int refinement_level);
StatusCode mesh_engine_coarsen_mesh(MeshEngine* engine,
                                  MeshData* mesh,
                                  Real coarsening_threshold);

StatusCode mesh_compute_quality_metrics(MeshEngine* engine,
                                      MeshData* mesh,
                                      MeshQualityMetrics* metrics);
StatusCode mesh_improve_quality(MeshEngine* engine,
                              MeshData* mesh,
                              Real target_quality);

/*********************************************************************
 * Material Database - Unified Material Models
 *********************************************************************/
typedef enum {
    MATERIAL_TYPE_CONDUCTOR,
    MATERIAL_TYPE_DIELECTRIC,
    MATERIAL_TYPE_MAGNETIC,
    MATERIAL_TYPE_LOSSY,
    MATERIAL_TYPE_ANISOTROPIC,
    MATERIAL_TYPE_DISPERSIVE,
    MATERIAL_TYPE_NONLINEAR
} MaterialType;

typedef enum {
    MATERIAL_MODEL_CONSTANT,
    MATERIAL_MODEL_DRUDE,
    MATERIAL_MODEL_LORENTZ,
    MATERIAL_MODEL_DEBYE,
    MATERIAL_MODEL_COLE_COLE,
    MATERIAL_MODEL_TABULATED
} MaterialModel;

typedef struct {
    // Basic properties
    Real epsilon_r;           // Relative permittivity
    Real mu_r;                  // Relative permeability
    Real sigma;                 // Conductivity [S/m]
    Real tan_delta;             // Loss tangent
    
    // Frequency-dependent model
    MaterialModel model;
    Real* frequency_points;     // Hz
    Complex* epsilon_data;
    Complex* mu_data;
    int num_frequency_points;
    
    // Anisotropic properties
    Real epsilon_tensor[9];     // 3x3 tensor
    Real mu_tensor[9];          // 3x3 tensor
    Real sigma_tensor[9];       // 3x3 tensor
    bool is_anisotropic;
    
    // Temperature dependence
    Real temperature_coefficient_epsilon;
    Real temperature_coefficient_mu;
    Real reference_temperature; // Celsius
    
    // Nonlinear properties
    Real nonlinear_coefficients[6];
    bool is_nonlinear;
    
    // Material identification
    int material_id;
    char name[256];
    char description[1024];
} MaterialProperties;

typedef struct {
    MaterialProperties* materials;
    int num_materials;
    int max_materials;
    
    // Material lookup by name
    void* name_hash_table;
    
    // Thread safety
    void* mutex;
} MaterialDatabase;

// Material database functions
MaterialDatabase* material_database_create(void);
void material_database_destroy(MaterialDatabase* db);

int material_database_add_material(MaterialDatabase* db,
                                 const MaterialProperties* material);
MaterialProperties* material_database_get_material(MaterialDatabase* db,
                                                 int material_id);
MaterialProperties* material_database_find_material(MaterialDatabase* db,
                                                  const char* name);

// Material evaluation
Complex material_database_get_epsilon(MaterialDatabase* db,
                                    int material_id,
                                    Real frequency,
                                    Real temperature);
Complex material_database_get_mu(MaterialDatabase* db,
                               int material_id,
                               Real frequency,
                               Real temperature);

// Built-in materials
StatusCode material_database_load_builtin(MaterialDatabase* db);
StatusCode material_database_add_conductor(MaterialDatabase* db,
                                         const char* name,
                                         Real conductivity);
StatusCode material_database_add_dielectric(MaterialDatabase* db,
                                          const char* name,
                                          Real epsilon_r,
                                          Real tan_delta);

/*********************************************************************
 * Linear Algebra Engine - Optimized Linear Algebra
 *********************************************************************/
typedef enum {
    MATRIX_FORMAT_DENSE,
    MATRIX_FORMAT_CSR,      // Compressed Sparse Row
    MATRIX_FORMAT_CSC,      // Compressed Sparse Column
    MATRIX_FORMAT_COO,      // Coordinate format
    MATRIX_FORMAT_BLOCK_SPARSE
} MatrixFormat;

typedef enum {
    LINEAR_SOLVER_DIRECT_LU,
    LINEAR_SOLVER_DIRECT_CHOLESKY,
    LINEAR_SOLVER_GMRES,
    LINEAR_SOLVER_BICGSTAB,
    LINEAR_SOLVER_CG,
    LINEAR_SOLVER_QMR,
    LINEAR_SOLVER_TFQMR
} LinearSolverType;

typedef struct {
    MatrixFormat format;
    int rows;
    int cols;
    int nnz;                // Number of non-zeros
    
    union {
        struct {
            Real* data;     // Dense matrix
        } dense;
        
        struct {
            Real* values;   // Non-zero values
            int* row_ptr;   // Row pointers
            int* col_idx;   // Column indices
        } csr;
        
        struct {
            Real* values;   // Non-zero values
            int* col_ptr;   // Column pointers
            int* row_idx;   // Row indices
        } csc;
        
        struct {
            Real* values;   // Non-zero values
            int* row_idx;   // Row indices
            int* col_idx;   // Column indices
        } coo;
    } data;
    
    bool is_complex;
    bool is_symmetric;
    bool is_positive_definite;
} Matrix;

typedef struct {
    Real* data;
    int size;
    bool is_complex;
    int alignment;          // Memory alignment
} Vector;

typedef struct {
    LinearSolverType type;
    int max_iterations;
    Real tolerance;
    int restart_size;       // For GMRES
    int preconditioner_type;
    bool use_parallel;
    int num_threads;
    bool use_gpu;
} LinearSolverConfig;

// Linear algebra functions
LinearAlgebraEngine* linalg_engine_create(void);
void linalg_engine_destroy(LinearAlgebraEngine* engine);

// Matrix operations
Matrix* matrix_create(MatrixFormat format, int rows, int cols);
void matrix_destroy(Matrix* matrix);
StatusCode matrix_set_value(Matrix* matrix, int i, int j, Real value);
Real matrix_get_value(const Matrix* matrix, int i, int j);
StatusCode matrix_add_matrix(const Matrix* A, const Matrix* B, Matrix* C);
StatusCode matrix_multiply(const Matrix* A, const Matrix* B, Matrix* C);
StatusCode matrix_transpose(const Matrix* A, Matrix* AT);

// Vector operations
Vector* vector_create(int size, bool is_complex);
void vector_destroy(Vector* vector);
StatusCode vector_set_value(Vector* vector, int i, Real value);
Real vector_get_value(const Vector* vector, int i);
StatusCode vector_add(const Vector* a, const Vector* b, Vector* c);
StatusCode vector_scale(Vector* vector, Real scalar);
Real vector_dot(const Vector* a, const Vector* b);

// Linear solvers
void* linear_solver_create(LinearSolverType type, const LinearSolverConfig* config);
void linear_solver_destroy(void* solver);
StatusCode linear_solver_solve(void* solver, const Matrix* A, const Vector* b, Vector* x);

// BLAS/LAPACK integration
StatusCode linalg_engine_set_blas_backend(const char* backend);  // "mkl", "openblas", "accelerate"
StatusCode linalg_engine_set_gpu_backend(const char* backend);  // "cuda", "opencl", "hip"

/*********************************************************************
 * I/O Engine - Unified Input/Output
 *********************************************************************/
typedef enum {
    FILE_FORMAT_AUTO,
    FILE_FORMAT_JSON,
    FILE_FORMAT_XML,
    FILE_FORMAT_HDF5,
    FILE_FORMAT_CSV,
    FILE_FORMAT_VTK,
    FILE_FORMAT_GMSH,
    FILE_FORMAT_NASTRAN,
    FILE_FORMAT_ABAQUS
} FileFormat;

typedef struct {
    char project_name[256];
    char description[1024];
    char created_by[256];
    char created_date[64];
    char version[32];
    
    // File paths
    char geometry_file[1024];
    char mesh_file[1024];
    char material_file[1024];
    char results_file[1024];
    char log_file[1024];
    
    // Solver settings
    void* solver_config;
    
    // Performance settings
    int num_threads;
    bool use_gpu;
    bool use_mpi;
    size_t memory_limit;
} ProjectInfo;

typedef struct {
    bool enable_checkpointing;
    int checkpoint_interval;      // Minutes
    bool enable_results_compression;
    FileFormat results_format;
    bool enable_parallel_io;
    int io_threads;
} IOConfig;

// I/O engine functions
IOEngine* io_engine_create(const IOConfig* config);
void io_engine_destroy(IOEngine* engine);

// Project management
StatusCode io_engine_create_project(IOEngine* engine, const char* project_name);
StatusCode io_engine_load_project(IOEngine* engine, const char* project_file);
StatusCode io_engine_save_project(IOEngine* engine);
ProjectInfo* io_engine_get_project_info(IOEngine* engine);

// Geometry I/O
StatusCode io_engine_import_geometry(IOEngine* engine, const char* filename,
                                   GeometryEngine* geometry);
StatusCode io_engine_export_geometry(IOEngine* engine, const char* filename,
                                   GeometryEngine* geometry);

// Mesh I/O
StatusCode io_engine_import_mesh(IOEngine* engine, const char* filename,
                               MeshEngine* mesh_engine, MeshData** mesh);
StatusCode io_engine_export_mesh(IOEngine* engine, const char* filename,
                               MeshData* mesh);

// Results I/O
StatusCode io_engine_export_results(IOEngine* engine, const char* filename,
                                  void* results, FileFormat format);
StatusCode io_engine_import_results(IOEngine* engine, const char* filename,
                                  void** results);

// Visualization export
StatusCode io_engine_export_vtk(IOEngine* engine, const char* filename,
                              MeshData* mesh, void* field_data);
StatusCode io_engine_export_paraview(IOEngine* engine, const char* filename,
                                   MeshData* mesh, void* field_data);

/*********************************************************************
 * Unified Framework - Main Interface
 *********************************************************************/
typedef enum {
    SOLVER_TYPE_MOM,
    SOLVER_TYPE_PEEC,
    SOLVER_TYPE_HYBRID,
    SOLVER_TYPE_FEM
} SolverType;

typedef enum {
    SIMULATION_TYPE_FREQUENCY_DOMAIN,
    SIMULATION_TYPE_TIME_DOMAIN,
    SIMULATION_TYPE_EIGENMODE,
    SIMULATION_TYPE_TRANSIENT
} SimulationType;

typedef struct {
    // Framework configuration
    LoggingConfig logging;
    GeometryEngineConfig geometry;
    MeshParameters mesh;
    IOConfig io;
    LinearSolverConfig linalg;
    
    // Performance settings
    int num_threads;
    bool use_gpu;
    bool use_mpi;
    size_t memory_limit;
    
    // Solver selection
    SolverType solver_type;
    SimulationType simulation_type;
    
    // Accuracy settings
    Real accuracy_target;
    int max_iterations;
    Real convergence_tolerance;
} FrameworkConfig;

typedef struct {
    // Framework components
    Framework* framework;
    
    // Component engines
    GeometryEngine* geometry_engine;
    MeshEngine* mesh_engine;
    MaterialDatabase* material_database;
    LinearAlgebraEngine* linalg_engine;
    IOEngine* io_engine;
    
    // Configuration
    FrameworkConfig config;
    
    // State information
    bool is_initialized;
    bool is_geometry_loaded;
    bool is_mesh_generated;
    bool is_material_assigned;
    
    // Performance monitoring
    double initialization_time;
    double geometry_load_time;
    double mesh_generation_time;
    size_t peak_memory_usage;
    
} FrameworkState;

// Main framework functions
Framework* framework_create(const FrameworkConfig* config);
void framework_destroy(Framework* framework);

// Framework lifecycle
StatusCode framework_initialize(Framework* framework);
StatusCode framework_finalize(Framework* framework);
bool framework_is_initialized(const Framework* framework);

// Component access
GeometryEngine* framework_get_geometry_engine(Framework* framework);
MeshEngine* framework_get_mesh_engine(Framework* framework);
MaterialDatabase* framework_get_material_database(Framework* framework);
LinearAlgebraEngine* framework_get_linalg_engine(Framework* framework);
IOEngine* framework_get_io_engine(Framework* framework);

// Configuration management
StatusCode framework_load_config(Framework* framework, const char* config_file);
StatusCode framework_save_config(Framework* framework, const char* config_file);
const FrameworkConfig* framework_get_config(const Framework* framework);

// Performance monitoring
StatusCode framework_get_performance_info(Framework* framework,
                                        FrameworkState* state);
StatusCode framework_reset_performance_counters(Framework* framework);

// Memory management
StatusCode framework_set_memory_limit(Framework* framework, size_t limit);
size_t framework_get_memory_usage(Framework* framework);
size_t framework_get_peak_memory_usage(Framework* framework);

// Version information
void framework_get_version(int* major, int* minor, int* patch);
const char* framework_get_version_string(void);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

// Timing utilities
typedef struct {
    double start_time;
    double elapsed_time;
    bool is_running;
} Timer;

void timer_start(Timer* timer);
void timer_stop(Timer* timer);
double timer_get_elapsed(const Timer* timer);

// Memory utilities
void* framework_aligned_malloc(size_t size, size_t alignment);
void framework_aligned_free(void* ptr);
StatusCode framework_memory_copy(void* dst, const void* src, size_t size);

// Mathematical utilities
Real framework_compute_relative_error(Real computed, Real reference);
Real framework_compute_absolute_error(Real computed, Real reference);
bool framework_is_converged(Real error, Real tolerance);

// Parallel utilities
int framework_get_num_threads(void);
StatusCode framework_set_num_threads(int num_threads);
int framework_get_thread_id(void);

#ifdef __cplusplus
}
#endif

#endif // PEEC_MOM_CORE_H