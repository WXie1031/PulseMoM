/********************************************************************************
 * Core Common Definitions for PulseMoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Common definitions and types shared across the core modules
 ********************************************************************************/

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#if defined(_MSC_VER)
typedef struct { double re; double im; } complex_t;
#else
#include <complex.h>
typedef double complex complex_t;
#endif
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Precision definitions
typedef double real_t;

// Common constants
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define C0 299792458.0          // Speed of light in vacuum (m/s)
#define MU0 (4.0 * M_PI * 1e-7)  // Permeability of free space (H/m)
#define EPS0 (1.0 / (MU0 * C0 * C0))  // Permittivity of free space (F/m)
#define ETA0 (MU0 * C0)         // Impedance of free space (Ohms)
#define Z0 ETA0                 // Alternative notation

// Numerical tolerance constants
#define GEOMETRIC_EPSILON 1e-9      // Geometric comparison tolerance
#define AREA_EPSILON 1e-15          // Minimum valid triangle/area threshold
#define DISTANCE_EPSILON 1e-12      // Small epsilon for distance calculations
#define NUMERICAL_EPSILON 1e-12     // General numerical stability epsilon
#define MATRIX_PIVOT_EPSILON_SQ 1e-24  // Square of pivot threshold for matrix operations (1e-12 squared)
#define REGULARIZATION_MIN 1e-9     // Minimum regularization value
#define REGULARIZATION_MAX 1e-3     // Maximum regularization value
#define DEFAULT_AREA_FALLBACK 1e-6  // Default area value when triangle area is not available (for approximate calculations)
#define CONVERGENCE_TOLERANCE_DEFAULT 1e-6  // Default convergence tolerance for iterative solvers

// Frequency domain structure
typedef struct {
    double freq;      // Frequency (Hz)
    double omega;     // Angular frequency (rad/s)
    double k0;        // Free space wavenumber (rad/m)
    double eta0;      // Free space impedance (Ohms)
    complex_t eps0;   // Free space permittivity
    complex_t mu0;    // Free space permeability
} frequency_domain_t;

// 3D point/vector
typedef struct {
    double x, y, z;
} point3d_t;

// 3D vector with complex components
typedef struct {
    complex_t x, y, z;
} vector3d_complex_t;

// Triangle definition
typedef struct {
    int v1, v2, v3;      // Vertex indices
    int edge1, edge2, edge3;  // Edge indices
    double area;         // Triangle area
    point3d_t centroid;  // Centroid
    point3d_t normal;    // Unit normal vector
} triangle_t;

// Edge definition
typedef struct {
    int v1, v2;          // Vertex indices
    double length;       // Edge length
    point3d_t midpoint;  // Midpoint
    point3d_t tangent;   // Unit tangent vector
} edge_t;

// RWG basis function
typedef struct {
    int triangle_plus;   // Positive triangle index
    int triangle_minus;  // Negative triangle index
    int edge_index;      // Common edge index
    double edge_length;  // Edge length
    double area_plus;    // Area of positive triangle
    double area_minus;   // Area of negative triangle
    point3d_t centroid_plus;   // Centroid of positive triangle
    point3d_t centroid_minus;  // Centroid of negative triangle
    point3d_t edge_vector;     // Edge vector (from minus to plus)
} rwg_basis_t;

// Material properties
typedef struct {
    char name[64];      // Material name
    double eps_r;       // Relative permittivity
    double mu_r;        // Relative permeability
    double sigma;       // Conductivity (S/m)
    double tan_delta;   // Loss tangent
    complex_t eps;      // Complex permittivity
    complex_t mu;       // Complex permeability
} material_properties_t;

// Solver parameters
typedef struct {
    double tolerance;       // Convergence tolerance
    int max_iterations;     // Maximum iterations
    bool use_preconditioner; // Use preconditioning
    bool use_parallel;      // Enable parallel processing
    int num_threads;        // Number of threads
} solver_params_t;

// Performance metrics
typedef struct {
    double setup_time;          // Setup time (s)
    double matrix_fill_time;    // Matrix filling time (s)
    double solve_time;          // Solution time (s)
    double total_time;          // Total time (s)
    size_t memory_usage;        // Memory usage (bytes)
    int num_unknowns;           // Number of unknowns
    bool converged;            // Convergence status
} performance_metrics_t;

// Error codes
typedef enum {
    CORE_SUCCESS = 0,
    CORE_ERROR_INVALID_INPUT = -1,
    CORE_ERROR_MEMORY_ALLOCATION = -2,
    CORE_ERROR_FILE_NOT_FOUND = -3,
    CORE_ERROR_INVALID_FORMAT = -4,
    CORE_ERROR_NUMERICAL_INSTABILITY = -5,
    CORE_ERROR_CONVERGENCE_FAILURE = -6
} core_error_t;

// Function prototypes
void init_frequency_domain(frequency_domain_t* fd, double freq);
void compute_material_properties(material_properties_t* mat, double freq);
double vector3d_distance(const point3d_t* p1, const point3d_t* p2);
point3d_t vector3d_add(const point3d_t* v1, const point3d_t* v2);
point3d_t vector3d_subtract(const point3d_t* v1, const point3d_t* v2);
double vector3d_dot(const point3d_t* v1, const point3d_t* v2);
point3d_t vector3d_cross_point(const point3d_t* v1, const point3d_t* v2);
double vector3d_magnitude(const point3d_t* v);
point3d_t vector3d_normalize(const point3d_t* v);

// Complex arithmetic helper functions for MSVC compatibility
static inline double complex_magnitude(const complex_t* z) {
    return sqrt(z->re * z->re + z->im * z->im);
}

static inline complex_t complex_add(const complex_t* a, const complex_t* b) {
    complex_t result;
    result.re = a->re + b->re;
    result.im = a->im + b->im;
    return result;
}

static inline complex_t complex_subtract(const complex_t* a, const complex_t* b) {
    complex_t result;
    result.re = a->re - b->re;
    result.im = a->im - b->im;
    return result;
}

static inline complex_t complex_multiply(const complex_t* a, const complex_t* b) {
    complex_t result;
    result.re = a->re * b->re - a->im * b->im;
    result.im = a->re * b->im + a->im * b->re;
    return result;
}

static inline complex_t complex_divide(const complex_t* a, const complex_t* b) {
    complex_t result;
    double denom = b->re * b->re + b->im * b->im;
    result.re = (a->re * b->re + a->im * b->im) / denom;
    result.im = (a->im * b->re - a->re * b->im) / denom;
    return result;
}

static inline complex_t complex_scalar_multiply(const complex_t* a, double scalar) {
    complex_t result;
    result.re = a->re * scalar;
    result.im = a->im * scalar;
    return result;
}

// Complex zero constant helper function
static inline complex_t complex_zero(void) {
    complex_t zero = {0.0, 0.0};
    return zero;
}

// Common mathematical constants
static const double ONE_THIRD = 1.0 / 3.0;
static const double ONE_HALF = 0.5;
static const double ONE_QUARTER = 0.25;
static const double TWO_POINT_ZERO = 2.0;
static const double FOUR_POINT_ZERO = 4.0;
static const double EQUILATERAL_TRIANGLE_HEIGHT_FACTOR = 1.519671371; // sqrt(3)/2 * 2 (for avg_edge_len approx)

// Square root constants
#define SQRT_3 1.7320508075688772  // sqrt(3.0)
#define SQRT_3_OVER_2 0.8660254037844386  // sqrt(3.0)/2.0
#define SQRT_3_OVER_5 0.7745966692414834  // sqrt(3.0/5.0)
#define SQRT_30 5.477225575051661  // sqrt(30.0)
#define INV_SQRT_3 0.5773502691896258  // 1.0/sqrt(3.0)

// Mathematical constants
#define TWO_PI 6.283185307179586  // 2.0 * M_PI
#define TWO_PI_OVER_C0 2.0943951023931953e-8  // 2.0 * M_PI / C0 (for wavenumber calculation: k = TWO_PI_OVER_C0 * frequency)
#define FOUR_PI 12.566370614359172  // 4.0 * M_PI
#define INV_4PI 7.957747154594767e-2  // 1.0 / (4.0 * M_PI) (for Green's function normalization)

// Default frequency constant (1 GHz)
#define DEFAULT_FREQUENCY_HZ 1e9  // Default frequency in Hz (1 GHz)

// Tolerance constants for numerical algorithms
#define HIGH_ACCURACY_TOLERANCE 1e-4  // High accuracy threshold for convergence
#define DEFAULT_ALGORITHM_TOLERANCE 1e-4  // Default tolerance for ACA, MLFMM, H-matrix
#define DEFAULT_REGULARIZATION 1e-6  // Default regularization value for self-terms
#define MINIMUM_DENOMINATOR_THRESHOLD 1e-30  // Minimum threshold for denominator checks

// RWG basis function weight factors (typical for surface RWG)
static const double RWG_WEIGHT_XX = 0.4;  // Weight for xx component
static const double RWG_WEIGHT_YY = 0.4;  // Weight for yy component
static const double RWG_WEIGHT_ZZ = 0.2;  // Weight for zz component

// Default threshold constants
static const double DEFAULT_NEAR_FIELD_THRESHOLD = 0.1;  // Default near-field threshold (wavelengths)
static const double DEFAULT_CFIE_ALPHA = 0.5;           // Default CFIE combination parameter
static const double CURVATURE_THRESHOLD_DEFAULT = 0.1;  // Default curvature threshold
static const double CURVATURE_RATIO_THRESHOLD = 0.3;     // Curvature ratio threshold
static const double NEAR_SINGULAR_RATIO_DEFAULT = 0.1;   // Default near-singular ratio

// MFIE Self-Term Constant
#define MFIE_SELF_TERM_VALUE 0.5 // Principal value of MFIE surface integral for closed surfaces (1/2)

// Complex constants for MSVC compatibility
static const complex_t complex_one = {1.0, 0.0};
static const complex_t complex_i = {0.0, 1.0};

/**
 * @brief Create complex number from real scalar
 */
static inline complex_t complex_real(double real) {
    complex_t result;
    result.re = real;
    result.im = 0.0;
    return result;
}

/**
 * @brief Add complex number and real scalar
 */
static inline complex_t complex_add_real(const complex_t* a, double real) {
    complex_t result;
    result.re = a->re + real;
    result.im = a->im;
    return result;
}

/**
 * @brief Complex exponential: exp(z)
 */
static inline complex_t complex_exponential(complex_t z) {
    complex_t result;
    double exp_re = exp(z.re);
    result.re = exp_re * cos(z.im);
    result.im = exp_re * sin(z.im);
    return result;
}

/**
 * @brief Multiply complex number by real scalar
 */
static inline complex_t complex_multiply_real(const complex_t* a, double real) {
    complex_t result;
    result.re = a->re * real;
    result.im = a->im * real;
    return result;
}

/**
 * @brief Divide complex number by real scalar
 */
static inline complex_t complex_divide_real(const complex_t* a, double real) {
    complex_t result;
    if (fabs(real) > 1e-15) {
        result.re = a->re / real;
        result.im = a->im / real;
    } else {
        result.re = 0.0;
        result.im = 0.0;
    }
    return result;
}

#ifdef __cplusplus
}
#endif

#endif // CORE_COMMON_H
