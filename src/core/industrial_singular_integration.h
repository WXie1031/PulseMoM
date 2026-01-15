/*****************************************************************************************
 * Industrial-Grade Singular Integration for MoM/PEEC
 * 
 * Implements the core singular integration algorithms that must be
 * self-written for commercial-grade accuracy. This is the "soul" of
 * the electromagnetic solver as mentioned in the analysis.
 * 
 * Supports:
 * - Triangle-triangle singular integrals (RWG basis)
 * - Triangle-wire singular integrals  
 * - Wire-wire singular integrals
 * - Duffy transformation for singularity cancellation
 * - Extraction techniques for near-singular cases
 *****************************************************************************************/

#ifndef INDUSTRIAL_SINGULAR_INTEGRATION_H
#define INDUSTRIAL_SINGULAR_INTEGRATION_H

#include <complex.h>
#include <math.h>

// Integration accuracy parameters (industrial standards)
#define SINGULAR_INTEGRATION_EPSILON 1e-12
#define NEAR_SINGULAR_TOLERANCE 1e-6
#define MAX_DUFFY_ORDER 15
#define MAX_EXTRACTION_ORDER 10
#define ADAPTIVE_REFINEMENT_LEVELS 8

// Singularity types
typedef enum {
    SINGULARITY_NONE,           // Regular integration
    SINGULARITY_POINT,          // Point singularity (1/R)
    SINGULARITY_EDGE,           // Edge singularity
    SINGULARITY_FACE,           // Face singularity
    SINGULARITY_NEAR_EDGE,      // Near-edge singularity
    SINGULARITY_NEAR_FACE,      // Near-face singularity
    SINGULARITY_WEAK,           // Weak singularity (integrable)
    SINGULARITY_STRONG          // Strong singularity (requires regularization)
} singularity_type_t;

// Integration method selection
typedef enum {
    INTEGRATION_REGULAR_GAUSS,      // Standard Gaussian quadrature
    INTEGRATION_DUFFY_TRANSFORM,    // Duffy transformation
    INTEGRATION_EXTRACTION,         // Singularity extraction
    INTEGRATION_ADAPTIVE_REFINEMENT, // Adaptive mesh refinement
    INTEGRATION_SEMIANALYTICAL,     // Semi-analytical formulas
    INTEGRATION_POLAR_TRANSFORM,    // Polar coordinate transformation
    INTEGRATION_COMPLEX_PLANE       // Complex plane integration
} integration_method_t;

// Triangle geometry for integration
typedef struct {
    double vertices[3][3];      // Triangle vertices
    double normal[3];           // Unit normal vector
    double area;               // Triangle area
    double centroid[3];        // Centroid coordinates
    double circumradius;       // Circumradius
    double inradius;           // Inradius
    int quality_flag;          // Quality indicator
} triangle_geometry_t;

// Wire geometry for integration
typedef struct {
    double start[3];           // Start point
    double end[3];             // End point
    double radius;             // Wire radius
    double length;             // Wire length
    double tangent[3];         // Unit tangent vector
    int segment_id;            // Segment identifier
} wire_geometry_t;

// Integration parameters
typedef struct {
    integration_method_t method;      // Integration method
    int gauss_order;               // Gaussian quadrature order
    int duffy_order;               // Duffy transformation order
    int extraction_order;          // Extraction order
    double tolerance;              // Integration tolerance
    int adaptive_levels;           // Adaptive refinement levels
    int use_semi_analytical;       // Enable semi-analytical formulas
    int verbose;                   // Debug output level
} integration_params_t;

// Integration result
typedef struct {
    double complex value;           // Integrated value
    double error_estimate;          // Error estimate
    int num_function_evaluations;   // Function evaluations
    singularity_type_t singularity_type; // Detected singularity
    integration_method_t method_used;     // Method actually used
    int convergence_flag;           // Convergence indicator
} integration_result_t;

// Core singular integration functions

// Triangle-triangle singular integration
double complex integrate_triangle_triangle_singular(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double complex (*kernel_func)(double, double, double, double, double, double),
    integration_params_t* params,
    integration_result_t* result);

// Triangle-wire singular integration  
double complex integrate_triangle_wire_singular(
    const triangle_geometry_t* triangle,
    const wire_geometry_t* wire,
    double complex (*kernel_func)(double, double, double, double, double, double),
    integration_params_t* params,
    integration_result_t* result);

// Wire-wire singular integration
double complex integrate_wire_wire_singular(
    const wire_geometry_t* wire1,
    const wire_geometry_t* wire2,
    double complex (*kernel_func)(double, double, double, double, double, double),
    integration_params_t* params,
    integration_result_t* result);

// Duffy transformation for triangle integration
double complex duffy_triangle_integration(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double complex (*singular_func)(double, double),
    int order,
    double tolerance);

// Singularity extraction technique
double complex extraction_triangle_integration(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double complex (*kernel_func)(double, double, double, double, double, double),
    double singularity_distance,
    int extraction_order);

// Adaptive refinement for near-singular cases
int adaptive_triangle_refinement(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double complex (*kernel_func)(double, double, double, double, double, double),
    integration_params_t* params,
    integration_result_t* result);

// Semi-analytical formulas for common cases
double complex semianalytical_triangle_self_term(
    const triangle_geometry_t* triangle);

double complex semianalytical_triangle_nearest_singular(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2);

// Industrial-grade RWG basis function integration
double complex integrate_rwg_basis_singular(
    const triangle_geometry_t* triangle_plus,
    const triangle_geometry_t* triangle_minus,
    const triangle_geometry_t* source_triangle,
    double complex (*green_func)(double, double, double),
    integration_params_t* params);

// PEEC partial element calculation
double complex compute_partial_inductance_singular(
    const wire_geometry_t* wire1,
    const wire_geometry_t* wire2,
    double frequency,
    integration_params_t* params);

double complex compute_partial_potential_singular(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double frequency,
    integration_params_t* params);

// Multi-layer Green's function singular integration
double complex integrate_multilayer_green_singular(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double complex (*multilayer_green)(double, double, double, double),
    double layer_thickness,
    double permittivity_ratio,
    integration_params_t* params);

// Advanced singularity detection
singularity_type_t detect_triangle_singularity(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double tolerance);

double compute_triangle_distance(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2);

double compute_triangle_proximity_parameter(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2);

// Integration order selection
int select_integration_order(
    singularity_type_t singularity,
    double target_accuracy,
    double element_size,
    double wavelength);

integration_method_t select_integration_method(
    singularity_type_t singularity,
    double proximity_parameter);

// Industrial validation and benchmarking
int validate_integration_accuracy(
    const triangle_geometry_t* triangle1,
    const triangle_geometry_t* triangle2,
    double complex reference_value,
    double tolerance);

double estimate_integration_error(
    const integration_result_t* result1,
    const integration_result_t* result2);

// Performance optimization for large-scale problems
int optimize_integration_parameters(
    integration_params_t* params,
    size_t num_integrations,
    double target_accuracy,
    double time_budget);

// Thread-safe integration for parallel computing
int integrate_triangle_triangle_parallel(
    const triangle_geometry_t* triangles1,
    const triangle_geometry_t* triangles2,
    size_t num_pairs,
    double complex* results,
    integration_params_t* params);

// Memory-efficient batch processing
int batch_triangle_integration(
    const triangle_geometry_t* triangles,
    size_t num_triangles,
    size_t* integration_pairs,
    size_t num_pairs,
    double complex* results,
    integration_params_t* params,
    size_t batch_size);

// Error handling and diagnostics
typedef struct {
    int error_code;
    char error_message[256];
    double worst_case_error;
    int num_failures;
    int num_successes;
    double average_accuracy;
} integration_diagnostics_t;

void get_integration_diagnostics(integration_diagnostics_t* diagnostics);
void reset_integration_diagnostics(void);

// Industrial constants and thresholds
#define TRIANGLE_QUALITY_THRESHOLD 0.1
#define WIRE_THICKNESS_THRESHOLD 1e-6
#define INTEGRATION_CONVERGENCE_THRESHOLD 1e-10
#define MAX_INTEGRATION_RETRIES 3
#define ADAPTIVE_REFINEMENT_RATIO 0.5

// Precomputed integration tables for performance
typedef struct {
    double* gauss_weights;      // Gauss quadrature weights
    double* gauss_points;       // Gauss quadrature points
    int max_order;             // Maximum precomputed order
    int initialized;           // Initialization flag
} integration_tables_t;

integration_tables_t* get_integration_tables(void);
void initialize_integration_tables(int max_order);
void cleanup_integration_tables(void);

#endif // INDUSTRIAL_SINGULAR_INTEGRATION_H