#ifndef CAD_MESH_GENERATION_H
#define CAD_MESH_GENERATION_H

#include "../core/core_common.h"
// #include "../matrix.h"  // File not found, commented out
// #include "../electromagnetic.h"  // File not found, commented out
#include "../solvers/mom/mom_solver.h"
#include "advanced_file_formats.h"

// Forward declaration to avoid type conflicts - already defined in mom_solver.h

#define MAX_CAD_ENTITIES 100000
#define MAX_MESH_NODES 1000000
#define MAX_MESH_ELEMENTS 500000
#define MAX_MATERIAL_TYPES 100
#define MAX_GEOMETRY_LAYERS 1000

typedef enum {
    CAD_ENTITY_TYPE_POINT,
    CAD_ENTITY_TYPE_LINE,
    CAD_ENTITY_TYPE_CIRCLE,
    CAD_ENTITY_TYPE_ARC,
    CAD_ENTITY_TYPE_ELLIPSE,
    CAD_ENTITY_TYPE_SPLINE,
    CAD_ENTITY_TYPE_BEZIER,
    CAD_ENTITY_TYPE_NURBS,
    CAD_ENTITY_TYPE_POLYGON,
    CAD_ENTITY_TYPE_POLYLINE,
    CAD_ENTITY_TYPE_RECTANGLE,
    CAD_ENTITY_TYPE_TRIANGLE,
    CAD_ENTITY_TYPE_QUADRILATERAL,
    CAD_ENTITY_TYPE_TETRAHEDRON,
    CAD_ENTITY_TYPE_HEXAHEDRON,
    CAD_ENTITY_TYPE_PRISM,
    CAD_ENTITY_TYPE_PYRAMID,
    CAD_ENTITY_TYPE_SPHERE,
    CAD_ENTITY_TYPE_CYLINDER,
    CAD_ENTITY_TYPE_CONE,
    CAD_ENTITY_TYPE_TORUS,
    CAD_ENTITY_TYPE_CUSTOM
} CadEntityType;

// MeshType enum - renamed to avoid conflicts with core_mesh.h
typedef enum {
    CAD_MESH_TYPE_TRIANGULAR,
    CAD_MESH_TYPE_QUADRILATERAL,
    CAD_MESH_TYPE_TETRAHEDRAL,
    CAD_MESH_TYPE_HEXAHEDRAL,
    CAD_MESH_TYPE_PRISMATIC,
    CAD_MESH_TYPE_PYRAMIDAL,
    CAD_MESH_TYPE_MIXED,
    CAD_MESH_TYPE_ADAPTIVE,
    CAD_MESH_TYPE_STRUCTURED,
    CAD_MESH_TYPE_UNSTRUCTURED,
    CAD_MESH_TYPE_HYBRID,
    CAD_MESH_TYPE_HIERARCHICAL,
    CAD_MESH_TYPE_ANISOTROPIC,
    CAD_MESH_TYPE_CURVILINEAR,
    CAD_MESH_TYPE_HIGH_ORDER
} CadMeshType;

// MeshAlgorithm enum - renamed to avoid conflicts with core_mesh.h
typedef enum {
    CAD_MESH_ALGORITHM_DELAUNAY,
    CAD_MESH_ALGORITHM_ADVANCING_FRONT,
    CAD_MESH_ALGORITHM_OCTREE,
    CAD_MESH_ALGORITHM_QUADTREE,
    CAD_MESH_ALGORITHM_VORONOI,
    CAD_MESH_ALGORITHM_BOWYER_WATSON,
    CAD_MESH_ALGORITHM_GREEN_SIBSON,
    CAD_MESH_ALGORITHM_WATSON,
    CAD_MESH_ALGORITHM_RUPPERT,
    CAD_MESH_ALGORITHM_CHEW,
    CAD_MESH_ALGORITHM_SHEWCHUK,
    CAD_MESH_ALGORITHM_PERSson,
    CAD_MESH_ALGORITHM_STRANG,
    CAD_MESH_ALGORITHM_ISOTROPIC,
    CAD_MESH_ALGORITHM_ANISOTROPIC,
    CAD_MESH_ALGORITHM_ADAPTIVE_REFINEMENT,
    CAD_MESH_ALGORITHM_COARSENING,
    CAD_MESH_ALGORITHM_SMOOTHING,
    CAD_MESH_ALGORITHM_OPTIMIZATION
} CadMeshAlgorithm;

typedef enum {
    QUALITY_METRIC_ASPECT_RATIO,
    QUALITY_METRIC_SKEWNESS,
    QUALITY_METRIC_ORTHOGONALITY,
    QUALITY_METRIC_WARPAGE,
    QUALITY_METRIC_TAPER,
    QUALITY_METRIC_STRETCH,
    QUALITY_METRIC_MINIMUM_ANGLE,
    QUALITY_METRIC_MAXIMUM_ANGLE,
    QUALITY_METRIC_CONDITION_NUMBER,
    QUALITY_METRIC_DISTORTION,
    QUALITY_METRIC_JACOBIAN,
    QUALITY_METRIC_NORMALIZED_JACOBIAN,
    QUALITY_METRIC_SHAPE,
    QUALITY_METRIC_RELATIVE_SIZE,
    QUALITY_METRIC_EDGE_RATIO,
    QUALITY_METRIC_AREA_VOLUME,
    QUALITY_METRIC_CIRCUMRADIUS_INRADIUS_RATIO
} QualityMetric;

typedef struct {
    double x, y, z;
    int node_id;
    int global_id;
    int* connected_elements;
    int num_connected_elements;
    int boundary_flag;
    int material_id;
    int physical_group;
    double mesh_size;
    int is_boundary_node;
    int is_corner_node;
    int is_edge_node;
} MeshNode;

typedef struct {
    int* node_ids;
    int num_nodes;
    int element_id;
    int element_type;
    int material_id;
    int physical_group;
    double area;
    double volume;
    double aspect_ratio;
    double quality_metric;
    int is_boundary_element;
    int is_curved;
    int polynomial_order;
} MeshElement;

typedef struct {
    CadEntityType type;
    double* control_points;
    int num_control_points;
    double* weights;
    int num_weights;
    double* knots;
    int num_knots;
    int degree;
    int is_rational;
    int is_periodic;
    int is_closed;
    double bounding_box[6];
    double center[3];
    double normal[3];
    double tangent[3];
    double curvature;
    int entity_id;
    int layer_id;
    int material_id;
} CadEntity;

typedef struct {
    double epsilon_r;
    double mu_r;
    double conductivity;
    double loss_tangent;
    double dielectric_strength;
    double thermal_conductivity;
    double density;
    double young_modulus;
    double poisson_ratio;
    double thermal_expansion;
    complex_t permittivity;
    complex_t permeability;
    int material_id;
    char material_name[256];
    int is_anisotropic;
    int is_dispersive;
    int is_nonlinear;
    int is_lossy;
} MaterialProperties;

typedef struct {
    double target_element_size;
    double minimum_element_size;
    double maximum_element_size;
    double element_size_growth_rate;
    double curvature_resolution;
    double proximity_resolution;
    double feature_angle;
    double mesh_grading;
    int adaptive_refinement_levels;
    int maximum_refinement_iterations;
    double quality_threshold;
    double convergence_tolerance;
    int use_curvature_adaptation;
    int use_proximity_adaptation;
    int use_feature_detection;
    int use_boundary_layer;
    int use_periodic_meshing;
    int use_symmetry_exploitation;
    int use_parallel_meshing;
    int num_threads;
} MeshGenerationParameters;

typedef struct {
    CadMeshType type;
    CadMeshAlgorithm algorithm;
    int polynomial_order;
    int use_high_order_elements;
    int use_curved_elements;
    int use_anisotropic_meshing;
    int use_adaptive_meshing;
    int use_structured_meshing;
    int use_unstructured_meshing;
    int use_hybrid_meshing;
    int use_hierarchical_meshing;
    int use_mesh_optimization;
    int use_mesh_smoothing;
    int use_mesh_coarsening;
    int use_mesh_refinement;
    int use_quality_improvement;
    int use_boundary_recovery;
    int use_constrained_meshing;
    int use_periodic_boundary_conditions;
} MeshGenerationConfig;

typedef struct {
    double aspect_ratio;
    double skewness;
    double orthogonality;
    double warpage;
    double minimum_angle;
    double maximum_angle;
    double condition_number;
    double jacobian;
    double shape_quality;
    double size_quality;
    double overall_quality;
    int num_poor_elements;
    int num_good_elements;
    int num_excellent_elements;
    double average_quality;
    double minimum_quality;
    double maximum_quality;
    double quality_standard_deviation;
} MeshQualityMetrics;

typedef struct {
    CadEntity* entities;
    int num_entities;
    MeshNode* nodes;
    int num_nodes;
    MeshElement* elements;
    int num_elements;
    MaterialProperties* materials;
    int num_materials;
    MeshGenerationParameters parameters;
    MeshGenerationConfig config;
    MeshQualityMetrics quality_metrics;
    double* geometry_data;
    int geometry_size;
    int* node_connectivity;
    int* element_connectivity;
    double* node_coordinates;
    int* element_types;
    int* material_assignments;
    int* physical_groups;
    int mesh_generation_completed;
    double computation_time;
    double memory_usage;
    int convergence_status;
} CadMeshGenerationSolver;

CadMeshGenerationSolver* cad_mesh_generation_create(MeshGenerationConfig* config);
void cad_mesh_generation_destroy(CadMeshGenerationSolver* solver);

int cad_mesh_generation_simulate(CadMeshGenerationSolver* solver);
int cad_mesh_generation_generate_mesh(CadMeshGenerationSolver* solver);
int cad_mesh_generation_refine_mesh(CadMeshGenerationSolver* solver);
int cad_mesh_generation_optimize_mesh(CadMeshGenerationSolver* solver);

int cad_mesh_generation_import_cad_file(CadMeshGenerationSolver* solver, const char* filename, FileFormatType format);
int cad_mesh_generation_import_geometry(CadMeshGenerationSolver* solver, double* geometry_data, int geometry_size);
int cad_mesh_generation_export_mesh(CadMeshGenerationSolver* solver, const char* filename, const char* format);

int cad_mesh_generation_setup_entities(CadMeshGenerationSolver* solver, CadEntity* entities, int num_entities);
int cad_mesh_generation_setup_materials(CadMeshGenerationSolver* solver, MaterialProperties* materials, int num_materials);
int cad_mesh_generation_setup_parameters(CadMeshGenerationSolver* solver, MeshGenerationParameters* parameters);

int cad_mesh_generation_generate_nodes(CadMeshGenerationSolver* solver);
int cad_mesh_generation_generate_elements(CadMeshGenerationSolver* solver);
int cad_mesh_generation_connect_mesh(CadMeshGenerationSolver* solver);

int cad_mesh_generation_apply_boundary_conditions(CadMeshGenerationSolver* solver);
int cad_mesh_generation_apply_material_properties(CadMeshGenerationSolver* solver);
int cad_mesh_generation_apply_mesh_constraints(CadMeshGenerationSolver* solver);

int cad_mesh_generation_compute_quality_metrics(CadMeshGenerationSolver* solver);
int cad_mesh_generation_improve_mesh_quality(CadMeshGenerationSolver* solver);
int cad_mesh_generation_smooth_mesh(CadMeshGenerationSolver* solver);
int cad_mesh_generation_coarsen_mesh(CadMeshGenerationSolver* solver);

int cad_mesh_generation_detect_features(CadMeshGenerationSolver* solver);
int cad_mesh_generation_extract_boundaries(CadMeshGenerationSolver* solver);
int cad_mesh_generation_identify_regions(CadMeshGenerationSolver* solver);

int cad_mesh_generation_adaptive_refinement(CadMeshGenerationSolver* solver, double error_threshold);
int cad_mesh_generation_curvature_adaptation(CadMeshGenerationSolver* solver);
int cad_mesh_generation_proximity_adaptation(CadMeshGenerationSolver* solver);

int cad_mesh_generation_parallel_meshing(CadMeshGenerationSolver* solver);
int cad_mesh_generation_domain_decomposition(CadMeshGenerationSolver* solver);
int cad_mesh_generation_load_balancing(CadMeshGenerationSolver* solver);

int cad_mesh_generation_validate_mesh(CadMeshGenerationSolver* solver);
int cad_mesh_generation_check_mesh_consistency(CadMeshGenerationSolver* solver);
int cad_mesh_generation_check_mesh_quality(CadMeshGenerationSolver* solver);
int cad_mesh_generation_check_boundary_integrity(CadMeshGenerationSolver* solver);

void cad_mesh_generation_get_mesh_statistics(CadMeshGenerationSolver* solver, int* num_nodes, int* num_elements, int* num_materials);
void cad_mesh_generation_get_quality_metrics(CadMeshGenerationSolver* solver, MeshQualityMetrics* metrics);
void cad_mesh_generation_get_node_coordinates(CadMeshGenerationSolver* solver, double** coordinates, int* num_nodes);
void cad_mesh_generation_get_element_connectivity(CadMeshGenerationSolver* solver, int** connectivity, int* num_elements);

int cad_mesh_generation_export_to_electromagnetic_solver(CadMeshGenerationSolver* solver, mom_solver_t* mom_solver);
int cad_mesh_generation_export_to_fdtd_solver(CadMeshGenerationSolver* solver);
int cad_mesh_generation_export_to_fem_solver(CadMeshGenerationSolver* solver);

void cad_mesh_generation_print_summary(CadMeshGenerationSolver* solver);
void cad_mesh_generation_benchmark(CadMeshGenerationSolver* solver);

#endif