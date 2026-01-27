/*********************************************************************
 * Wideband Simulation Optimization Module
 * 
 * This module implements advanced techniques for efficient multi-layer
 * PCB wideband response simulation, matching commercial tools like 
 * Keysight ADS and EMX.
 * 
 * Features:
 * - Adaptive frequency sampling
 * - Model order reduction (MOR)
 * - Rational function fitting
 * - Passivity enforcement
 * - Causality checking
 * - GPU acceleration for wideband analysis
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef WIDEBAND_SIMULATION_OPTIMIZATION_H
#define WIDEBAND_SIMULATION_OPTIMIZATION_H

#include "enhanced_sparameter_extraction.h"
#include "mom_solver.h"
#include <stdbool.h>
#include <complex.h>

// Adaptive sampling strategies
typedef enum {
    SAMPLING_LINEAR,
    SAMPLING_LOGARITHMIC,
    SAMPLING_ADAPTIVE_MAGNITUDE,
    SAMPLING_ADAPTIVE_PHASE,
    SAMPLING_ADAPTIVE_SLOPE,
    SAMPLING_HYBRID
} AdaptiveSamplingStrategy;

// Model order reduction methods
typedef enum {
    MOR_PRIMA,           // Passive Reduced-order Interconnect Macromodeling Algorithm
    MOR_PVL,             // Padé Via Lanczos
    MOR_ENOR,            // Efficient Nodal Order Reduction
    MOR_SVD,             // Singular Value Decomposition based
    MOR_KRYLOV,          // Krylov subspace methods
    MOR_TBR              // Truncated Balanced Realization
} ModelOrderReductionMethod;

// Rational function fitting types
typedef enum {
    RATIONAL_VECTOR_FITTING,
    RATIONAL_LOEWNER_MATRIX,
    RATIONAL_Cauchy_MATRIX,
    RATIONAL_ORTHOGONAL_FITTING
} RationalFittingType;

// Wideband optimization parameters
typedef struct {
    double f_min;                           // Minimum frequency (Hz)
    double f_max;                           // Maximum frequency (Hz)
    int max_samples;                        // Maximum number of frequency samples
    int target_order;                       // Target model order for reduction
    double error_tolerance;                 // Error tolerance for adaptive sampling
    double passivity_tolerance;              // Passivity enforcement tolerance
    bool enforce_passivity;                  // Enable passivity enforcement
    bool check_causality;                    // Enable causality checking
    bool use_gpu_acceleration;               // Enable GPU acceleration
    AdaptiveSamplingStrategy sampling_strategy;
    ModelOrderReductionMethod mor_method;
    RationalFittingType rational_type;
} WidebandOptimizationParams;

// Adaptive frequency sample
typedef struct {
    double frequency;                       // Frequency point (Hz)
    double weight;                          // Adaptive weight
    double error_estimate;                  // Local error estimate
    bool is_critical;                       // Critical frequency point
} AdaptiveSample;

// Rational function model
typedef struct {
    int order;                              // Model order
    double complex* poles;                  // Poles (resized array)
    double complex* residues;               // Residues (resized array)
    double complex* d_coeffs;               // Direct coupling terms
    double complex* e_coeffs;               // Constant term (if any)
    double* frequencies;                    // Original frequency samples
    double complex* original_response;      // Original response data
    int num_frequencies;                    // Number of frequency points
    bool is_passive;                        // Passivity status
    bool is_causal;                         // Causality status
    double max_error;                       // Maximum fitting error
    double rms_error;                       // RMS fitting error
} RationalFunctionModel;

// Wideband simulation result
typedef struct {
    SParameterSet* sparams;                 // Optimized S-parameters
    RationalFunctionModel* rational_model;  // Rational function approximation
    double* optimized_frequencies;          // Optimized frequency samples
    int num_optimized_samples;              // Number of optimized samples
    double total_speedup;                   // Achieved speedup factor
    double max_error;                       // Maximum approximation error
    double rms_error;                       // RMS approximation error
    int original_samples;                   // Original number of samples
    int reduced_samples;                    // Reduced number of samples
    bool passivity_enforced;                // Passivity enforcement status
    bool causality_verified;                // Causality verification status
} WidebandSimulationResult;

// GPU acceleration context
typedef struct {
    bool is_initialized;                    // GPU context initialized
    int device_id;                          // GPU device ID
    size_t memory_available;              // Available GPU memory
    size_t memory_used;                     // Used GPU memory
    int max_threads_per_block;              // CUDA max threads per block
    int num_multiprocessors;                // Number of multiprocessors
    double speedup_factor;                  // Measured speedup factor
} GPUAccelerationContext;

/*********************************************************************
 * Wideband Optimization Functions
 *********************************************************************/

// Create wideband optimization parameters
WidebandOptimizationParams* create_wideband_optimization_params(void);

// Destroy wideband optimization parameters
void destroy_wideband_optimization_params(WidebandOptimizationParams* params);

// Set optimization parameters
void set_wideband_frequency_range(WidebandOptimizationParams* params, 
                                double f_min, double f_max);
void set_wideband_sampling_strategy(WidebandOptimizationParams* params, 
                                  AdaptiveSamplingStrategy strategy);
void set_wideband_mor_method(WidebandOptimizationParams* params, 
                             ModelOrderReductionMethod method);
void set_wideband_error_tolerance(WidebandOptimizationParams* params, 
                                double tolerance);
void enable_wideband_gpu_acceleration(WidebandOptimizationParams* params, 
                                   bool enable);

/*********************************************************************
 * Adaptive Sampling Functions
 *********************************************************************/

// Perform adaptive frequency sampling
AdaptiveSample* perform_adaptive_sampling(const SParameterSet* sparams,
                                        const WidebandOptimizationParams* params,
                                        int* num_samples);

// Optimize frequency samples based on response characteristics
int optimize_frequency_samples(AdaptiveSample* samples, 
                            int num_samples,
                            const WidebandOptimizationParams* params);

// Estimate local error for adaptive sampling
double estimate_local_error(const double complex* response1,
                          const double complex* response2,
                          const double complex* response3,
                          int num_ports);

// Check if frequency point is critical
bool is_critical_frequency(const double* frequencies,
                         const double complex* responses,
                         int idx, int num_ports);

/*********************************************************************
 * Model Order Reduction Functions
 *********************************************************************/

// Perform model order reduction on S-parameters
RationalFunctionModel* perform_model_order_reduction(
    const SParameterSet* sparams,
    const WidebandOptimizationParams* params);

// Apply PRIMA algorithm for passive model reduction
RationalFunctionModel* apply_prima_reduction(
    const double complex* original_system,
    int original_order,
    int reduced_order,
    const double* frequencies,
    int num_frequencies);

// Apply PVL (Padé Via Lanczos) reduction
RationalFunctionModel* apply_pvl_reduction(
    const double complex* moments,
    int num_moments,
    int reduced_order);

/*********************************************************************
 * Rational Function Fitting Functions
 *********************************************************************/

// Perform vector fitting on frequency response
RationalFunctionModel* perform_vector_fitting(
    const double* frequencies,
    const double complex* response,
    int num_points,
    int initial_order,
    double tolerance);

// Perform Loewner matrix rational fitting
RationalFunctionModel* perform_loewner_fitting(
    const double* frequencies,
    const double complex* response,
    int num_points,
    int target_order);

// Refine rational function model
int refine_rational_model(RationalFunctionModel* model,
                         const double* frequencies,
                         const double complex* response,
                         int num_points,
                         double tolerance);

/*********************************************************************
 * Passivity and Causality Functions
 *********************************************************************/

// Check passivity of rational function model
bool check_rational_model_passivity(const RationalFunctionModel* model);

// Enforce passivity on rational function model
int enforce_rational_model_passivity(RationalFunctionModel* model,
                                    double tolerance);

// Check causality using Kramers-Kronig relations
bool check_rational_model_causality(const RationalFunctionModel* model);

// Enforce causality on rational function model
int enforce_rational_model_causality(RationalFunctionModel* model);

// Hamiltonian eigenvalue test for passivity
double complex* perform_hamiltonian_eigenvalue_test(
    const RationalFunctionModel* model,
    int* num_eigenvalues);

/*********************************************************************
 * GPU Acceleration Functions
 *********************************************************************/

// Initialize GPU acceleration context
GPUAccelerationContext* initialize_gpu_acceleration(void);

// Destroy GPU acceleration context
void destroy_gpu_acceleration_context(GPUAccelerationContext* context);

// Perform GPU-accelerated wideband simulation
WidebandSimulationResult* perform_gpu_wideband_simulation(
    const PCBEMModel* em_model,
    const WidebandOptimizationParams* params,
    GPUAccelerationContext* gpu_context);

// GPU-accelerated matrix operations
int perform_gpu_matrix_vector_multiplication(
    const double complex* matrix,
    const double complex* vector,
    double complex* result,
    int size,
    GPUAccelerationContext* context);

/*********************************************************************
 * Main Wideband Simulation Functions
 *********************************************************************/

// Perform optimized wideband simulation
WidebandSimulationResult* perform_wideband_simulation(
    const PCBEMModel* em_model,
    const WidebandOptimizationParams* params);

// Perform multi-layer PCB wideband optimization
WidebandSimulationResult* optimize_multilayer_pcb_simulation(
    PCBEMModel** layer_models,
    int num_layers,
    const WidebandOptimizationParams* params);

// Generate wideband macromodel from S-parameters
RationalFunctionModel* generate_wideband_macromodel(
    const SParameterSet* sparams,
    const WidebandOptimizationParams* params);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

// Create rational function model
RationalFunctionModel* create_rational_function_model(
    int order, int num_frequencies);

// Destroy rational function model
void destroy_rational_function_model(RationalFunctionModel* model);

// Evaluate rational function model
double complex evaluate_rational_model(
    const RationalFunctionModel* model,
    double frequency);

// Export rational function model to file
int export_rational_model(const RationalFunctionModel* model,
                         const char* filename);

// Import rational function model from file
RationalFunctionModel* import_rational_model(const char* filename);

// Calculate frequency response from rational model
int calculate_rational_response(
    const RationalFunctionModel* model,
    const double* frequencies,
    double complex* response,
    int num_points);

// Compare original and reduced models
double calculate_model_error(const SParameterSet* original,
                           const RationalFunctionModel* reduced,
                           double* max_error,
                           double* rms_error);

// Destroy wideband simulation result
void destroy_wideband_simulation_result(WidebandSimulationResult* result);

// Export wideband simulation results
int export_wideband_results(const WidebandSimulationResult* result,
                          const char* filename);

#endif // WIDEBAND_SIMULATION_OPTIMIZATION_H