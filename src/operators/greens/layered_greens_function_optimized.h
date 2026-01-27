#ifndef LAYERED_GREENS_FUNCTION_OPTIMIZED_H
#define LAYERED_GREENS_FUNCTION_OPTIMIZED_H

#include <complex.h>
#include <stdbool.h>

// Optimized constants and parameters
#define MAX_SOMMERFELD_POINTS 512
#define MAX_DCIM_IMAGES 64
#define SOMMERFELD_INTEGRATION_ORDER 8
#define DCIM_APPROXIMATION_ORDER 16
#define GREEN_CACHE_SIZE 1024
#define NEAR_FIELD_THRESHOLD 0.1
#define FAR_FIELD_THRESHOLD 10.0

// Enhanced layered medium structure with caching
typedef struct {
    int num_layers;
    double *thickness;      // Layer thicknesses [m]
    double *epsilon_r;      // Relative permittivities
    double *mu_r;          // Relative permeabilities
    double *sigma;         // Conductivities [S/m]
    double *tan_delta;     // Loss tangents
    
    // Cached parameters for performance
    double complex *cached_wavenumbers;
    double complex *cached_impedances;
    double *cached_attenuations;
    bool cache_valid;
    
    // Frequency-dependent properties
    double frequency_range[2]; // [min, max] frequency [Hz]
    int frequency_samples;
} LayeredMediumOptimized;

// Optimized frequency domain with precomputed values
typedef struct {
    double freq;           // Frequency [Hz]
    double omega;          // Angular frequency
    double complex k0;     // Free-space wavenumber
    double complex eta0;   // Free-space impedance
    
    // Precomputed values for efficiency
    double lambda0;        // Free-space wavelength
    double skin_depth_copper; // Copper skin depth
    double complex k0_squared; // k0^2
    double complex eta0_inverse; // 1/eta0
} FrequencyDomainOptimized;

// Advanced Green's function parameters with adaptive sampling
typedef struct {
    // Adaptive sampling parameters
    int n_points_adaptive;   // Number of adaptive integration points
    double krho_max_adaptive; // Maximum spectral variable (adaptive)
    double *krho_points_adaptive; // Adaptive spectral variable samples
    double *weights_adaptive; // Adaptive integration weights
    
    // DCIM parameters with enhanced accuracy
    bool use_dcim_enhanced;   // Enhanced DCIM with variable accuracy
    int n_images_adaptive;     // Adaptive number of complex images
    double dcim_tolerance;     // DCIM approximation tolerance
    double complex *amplitudes_adaptive;  // Adaptive image amplitudes
    double complex *exponents_adaptive;   // Adaptive image exponents
    
    // Integration method selection
    enum {
        INTEGRATION_ADAPTIVE,     // Adaptive quadrature
        INTEGRATION_GAUSS_LEGENDRE, // Gauss-Legendre quadrature
        INTEGRATION_LOBATTO,      // Lobatto quadrature
        INTEGRATION_ROMBERG       // Romberg integration
    } integration_method;
    
    // Performance optimization flags
    bool use_green_cache;      // Enable Green's function caching
    bool use_fast_math;        // Use fast math approximations
    bool use_vectorization;    // Enable SIMD vectorization
} GreensFunctionParamsOptimized;

// Spatial points with distance optimization
typedef struct {
    double x, y, z;        // Source coordinates
    double xp, yp, zp;     // Observation coordinates
    int layer_src;         // Source layer index
    int layer_obs;         // Observation layer index
    
    // Precomputed distances for efficiency
    double rho;            // Horizontal distance
    double dz;             // Vertical distance
    double r_total;        // Total distance
    bool is_near_field;    // Near/far field classification
    bool is_same_layer;    // Same layer check
} GreensFunctionPointsOptimized;

// Enhanced dyadic Green's function with symmetry exploitation
typedef struct {
    double complex G_ee[3][3];  // Electric-electric dyadic
    double complex G_em[3][3];  // Electric-magnetic dyadic
    double complex G_me[3][3];  // Magnetic-electric dyadic
    double complex G_mm[3][3];  // Magnetic-magnetic dyadic
    
    // Symmetry properties for optimization
    bool is_symmetric;
    bool is_reciprocal;
    bool is_lossless;
    
    // Accuracy metrics
    double estimated_error;
    int n_function_evaluations;
} GreensFunctionDyadicOptimized;

// Green's function cache for repeated evaluations
typedef struct {
    GreensFunctionPointsOptimized *points;
    GreensFunctionDyadicOptimized *green_functions;
    double *frequencies;
    int cache_size;
    int current_index;
    bool cache_enabled;
    
    // Cache statistics
    int cache_hits;
    int cache_misses;
    double cache_efficiency;
} GreenCache;

// Advanced Sommerfeld integral with adaptive refinement
double complex sommerfeld_integral_adaptive(
    double krho,
    double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    char polarization,
    double tolerance,
    int max_refinements
);

// Enhanced DCIM with variable accuracy
void dcim_approximation_enhanced(
    double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    double complex *amplitudes,
    double complex *exponents,
    int *n_images,
    double target_accuracy,
    int max_images
);

// Fast Green's function evaluation with caching
GreensFunctionDyadicOptimized* layered_medium_greens_function_optimized(
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    const GreensFunctionPointsOptimized *points,
    const GreensFunctionParamsOptimized *params,
    GreenCache *cache
);

// Vectorized Green's function for multiple points
void layered_medium_greens_function_vectorized(
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    const GreensFunctionPointsOptimized *points,
    int n_points,
    const GreensFunctionParamsOptimized *params,
    GreensFunctionDyadicOptimized *results
);

// Preconditioned Green's function for iterative solvers
typedef struct {
    double complex *preconditioner;
    double *condition_number;
    bool preconditioner_valid;
    int preconditioner_type; // 0: None, 1: Diagonal, 2: ILU, 3: SPAI
} GreenPreconditioner;

GreensFunctionDyadicOptimized* layered_medium_greens_function_preconditioned(
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    const GreensFunctionPointsOptimized *points,
    const GreensFunctionParamsOptimized *params,
    const GreenPreconditioner *preconditioner
);

// Multi-frequency Green's function for broadband analysis
void layered_medium_greens_function_multifrequency(
    const LayeredMediumOptimized *medium,
    const double *frequencies,
    int n_frequencies,
    const GreensFunctionPointsOptimized *points,
    const GreensFunctionParamsOptimized *params,
    GreensFunctionDyadicOptimized **results
);

// GPU-accelerated Green's function (optional)
#ifdef ENABLE_GPU_ACCELERATION
typedef struct {
    void *device_medium;
    void *device_frequency;
    void *device_points;
    void *device_results;
    int device_id;
    bool device_initialized;
} GPUContext;

GreensFunctionDyadicOptimized* layered_medium_greens_function_gpu(
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    const GreensFunctionPointsOptimized *points,
    const GreensFunctionParamsOptimized *params,
    GPUContext *gpu_context
);
#endif

// Performance monitoring and profiling
typedef struct {
    double computation_time;
    int function_calls;
    int cache_hits;
    int cache_misses;
    double memory_usage;
    double accuracy_achieved;
    int convergence_iterations;
} GreenFunctionProfile;

GreenFunctionProfile* get_green_function_profile(void);
void reset_green_function_profile(void);
void print_green_function_statistics(void);

// Advanced error estimation
double estimate_green_function_error(
    const GreensFunctionDyadicOptimized *gf,
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    const GreensFunctionPointsOptimized *points
);

// Memory-optimized Green's function with reduced allocations
GreensFunctionDyadicOptimized* layered_medium_greens_function_memory_optimized(
    const LayeredMediumOptimized *medium,
    const FrequencyDomainOptimized *freq,
    const GreensFunctionPointsOptimized *points,
    const GreensFunctionParamsOptimized *params,
    void *memory_pool,
    size_t memory_pool_size
);

// Initialization and cleanup
LayeredMediumOptimized* create_optimized_layered_medium(
    int num_layers,
    const double *thickness,
    const double *epsilon_r,
    const double *mu_r,
    const double *sigma,
    const double *tan_delta
);

void free_optimized_layered_medium(LayeredMediumOptimized *medium);
GreenCache* create_green_cache(int cache_size);
void free_green_cache(GreenCache *cache);
GreenPreconditioner* create_green_preconditioner(int type, int size);
void free_green_preconditioner(GreenPreconditioner *preconditioner);

#endif // LAYERED_GREENS_FUNCTION_OPTIMIZED_H