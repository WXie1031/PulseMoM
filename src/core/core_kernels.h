/**
 * @file core_kernels.h
 * @brief Unified physical kernels for MoM and PEEC solvers
 * @details Green functions, influence coefficients, and numerical integration
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_KERNELS_H
#define CORE_KERNELS_H

#if !defined(_MSC_VER)
#include <complex.h>
#endif
#include <stdbool.h>
#include "core_common.h"
#include "core_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

// Precision definitions
typedef double kernel_real_t;
#if defined(_MSC_VER)
typedef complex_t kernel_complex_t;
#else
typedef double complex kernel_complex_t;
#endif

// Kernel types
typedef enum {
    KERNEL_TYPE_FREE_SPACE,
    KERNEL_TYPE_LAYERED_MEDIA,
    KERNEL_TYPE_WAVEGUIDE,
    KERNEL_TYPE_CAVITY,
    KERNEL_TYPE_PERIODIC,
    KERNEL_TYPE_ROUGH_SURFACE
} kernel_type_t;

typedef enum {
    KERNEL_FORMULATION_EFIELD,     // Electric field integral equation
    KERNEL_FORMULATION_MFIELD,     // Magnetic field integral equation (MFIE)
    KERNEL_FORMULATION_COMBINED,   // Combined field integral equation (CFIE)
    KERNEL_FORMULATION_POTENTIAL,  // Scalar potential
    KERNEL_FORMULATION_VECTOR_POTENTIAL  // Vector potential
} kernel_formulation_t;

// Media properties
typedef struct {
    kernel_complex_t epsilon;      // Permittivity
    kernel_complex_t mu;          // Permeability
    kernel_complex_t sigma;       // Conductivity
    double thickness;             // Layer thickness
    double roughness;             // Surface roughness
} kernel_layer_t;

typedef struct {
    kernel_layer_t* layers;
    int num_layers;
    double frequency;
    kernel_complex_t k0;          // Free-space wavenumber
    kernel_complex_t eta0;        // Free-space impedance
} kernel_media_t;

// Integration parameters
typedef struct {
    int gauss_order;              // Gaussian quadrature order
    int singularity_order;        // Singularity handling order
    double accuracy;              // Integration accuracy
    int max_subdivisions;         // Maximum subdivisions for adaptive integration
    bool use_adaptive;            // Use adaptive integration
    bool use_singularity_extraction;  // Use singularity extraction
} kernel_integration_t;

// Element data for kernel evaluation
typedef struct {
    geom_element_type_t type;
    union {
        geom_triangle_t triangle;
        geom_quadrilateral_t quad;
        geom_line_t line;
        geom_rectangle_t rectangle;
    } geometry;
    
    // Basis function data
    void* basis_data;             // RWG for MoM, pulse for PEEC
    int basis_order;              // Basis function order
    
    // Material properties
    int material_id;
    kernel_complex_t epsilon;
    kernel_complex_t mu;
    kernel_complex_t sigma;
    
    // Evaluation points
    geom_point_t* evaluation_points;
    int num_evaluation_points;
    
    // Results
    kernel_complex_t* potentials;
    kernel_complex_t* fields;
} kernel_element_t;

/*********************************************************************
 * Green Function Kernels
 *********************************************************************/

// Free-space Green functions
kernel_complex_t kernel_green_function_free_space(double r, kernel_complex_t k);
kernel_complex_t kernel_green_function_gradient_free_space(const geom_point_t* r_vec, kernel_complex_t k);
kernel_complex_t kernel_green_function_hessian_free_space(const geom_point_t* r_vec, kernel_complex_t k);

// Layered media Green functions
kernel_complex_t kernel_green_function_layered(double r, double z, double zp, 
                                              const kernel_media_t* media);
kernel_complex_t kernel_green_function_gradient_layered(const geom_point_t* r_vec, 
                                                       double z, double zp, 
                                                       const kernel_media_t* media);

// Periodic Green functions
kernel_complex_t kernel_green_function_periodic(double r, const geom_point_t* k_vector,
                                               const geom_point_t* lattice_vectors[2]);

// Rough surface Green functions (for conductor roughness)
kernel_complex_t kernel_green_function_rough(double r, double roughness_parameter,
                                            kernel_complex_t k);

/*********************************************************************
 * Influence Coefficient Calculation
 *********************************************************************/

// Self-term influence coefficients
kernel_complex_t kernel_self_impedance(const kernel_element_t* element, 
                                      kernel_formulation_t formulation,
                                      const kernel_integration_t* integration);

// Mutual influence coefficients
kernel_complex_t kernel_mutual_impedance(const kernel_element_t* element_i, 
                                        const kernel_element_t* element_j,
                                        kernel_formulation_t formulation,
                                        const kernel_integration_t* integration);

// Partial element coefficients (PEEC)
typedef struct {
    kernel_complex_t resistance;    // R
    kernel_complex_t inductance;    // L
    kernel_complex_t potential;     // P (1/C)
    kernel_complex_t conductance;   // G
} kernel_partial_elements_t;

kernel_partial_elements_t kernel_compute_partial_elements(const kernel_element_t* element_i,
                                                         const kernel_element_t* element_j,
                                                         const kernel_media_t* media,
                                                         const kernel_integration_t* integration);

// Skin effect and proximity effect
kernel_complex_t kernel_skin_effect_impedance(double frequency, double conductivity, 
                                             double permeability, double radius,
                                             double skin_depth);
kernel_complex_t kernel_proximity_effect_impedance(const kernel_element_t* element_i,
                                                  const kernel_element_t* element_j,
                                                  double frequency,
                                                  const kernel_media_t* media);

/*********************************************************************
 * Numerical Integration
 *********************************************************************/

// Gaussian quadrature
void kernel_gauss_quadrature_1d(int order, double* points, double* weights);
void kernel_gauss_quadrature_2d(int order, double* points, double* weights);
void kernel_gauss_quadrature_triangle(int order, double* points, double* weights);
void kernel_gauss_quadrature_quadrilateral(int order, double* points, double* weights);

// Singularity extraction and handling
kernel_complex_t kernel_integrate_singular(const kernel_element_t* element_i,
                                           const kernel_element_t* element_j,
                                           kernel_formulation_t formulation,
                                           const kernel_integration_t* integration);

// Adaptive integration
kernel_complex_t kernel_integrate_adaptive(const kernel_element_t* element_i,
                                        const kernel_element_t* element_j,
                                        kernel_formulation_t formulation,
                                        double tolerance, int max_level);

// Duffy transformation for singular integrals
kernel_complex_t kernel_duffy_transform(const geom_triangle_t* triangle_i,
                                       const geom_triangle_t* triangle_j,
                                       kernel_formulation_t formulation,
                                       const kernel_media_t* media);

/*********************************************************************
 * Fast Algorithm Support
 *********************************************************************/

// Adaptive Cross Approximation (ACA)
typedef struct {
    int rank;                     // Approximation rank
    double tolerance;             // Approximation tolerance
    kernel_complex_t* U;          // Left singular vectors
    kernel_complex_t* V;          // Right singular vectors
    int max_rank;                 // Maximum rank
    bool converged;
} kernel_aca_data_t;

kernel_aca_data_t kernel_aca_decompose(const kernel_element_t* element_i,
                                     const kernel_element_t* element_j,
                                     int max_rank, double tolerance);
void kernel_aca_free(kernel_aca_data_t* aca);

// Multilevel Fast Multipole Method (MLFMM)
typedef struct {
    int levels;                   // Number of levels
    double box_size;              // Root box size
    geom_point_t center;          // Root box center
    kernel_complex_t expansion_factor;
} kernel_mlfmm_params_t;

kernel_complex_t kernel_mlfmm_evaluate(const kernel_element_t* element_i,
                                      const kernel_element_t* element_j,
                                      const kernel_mlfmm_params_t* params,
                                      const kernel_media_t* media);

/*********************************************************************
 * Vector Fitting and Model Order Reduction
 *********************************************************************/

// Frequency response data
typedef struct {
    double* frequencies;          // Hz
    kernel_complex_t* response;   // Complex response values
    int num_points;
} kernel_frequency_data_t;

// Vector fitting parameters
typedef struct {
    int num_poles;                // Number of poles
    int max_iterations;           // Maximum iterations
    double tolerance;              // Convergence tolerance
    bool enforce_passivity;        // Enforce passivity
    bool enforce_stability;        // Enforce stability
    bool use_relaxation;          // Use relaxed VF
} kernel_vector_fitting_params_t;

// Vector fitting results
typedef struct {
    kernel_complex_t* poles;       // Fitted poles
    kernel_complex_t* residues;    // Fitted residues
    kernel_complex_t constant;     // Constant term
    kernel_complex_t proportional;  // Proportional term
    int num_poles;
    double rms_error;
    bool passivity_enforced;
    bool stability_enforced;
} kernel_vector_fitting_result_t;

kernel_vector_fitting_result_t kernel_vector_fitting(const kernel_frequency_data_t* data,
                                                   const kernel_vector_fitting_params_t* params);
void kernel_vector_fitting_free(kernel_vector_fitting_result_t* result);

// Passivity checking and enforcement
bool kernel_check_passivity(const kernel_vector_fitting_result_t* result,
                           double* max_singular_value);
int kernel_enforce_passivity(kernel_vector_fitting_result_t* result,
                            double passivity_tolerance);

/*********************************************************************
 * Kernel Engine Management
 *********************************************************************/

typedef struct kernel_engine kernel_engine_t;

// Engine creation and configuration
kernel_engine_t* kernel_engine_create(kernel_type_t type, kernel_formulation_t formulation);
void kernel_engine_destroy(kernel_engine_t* engine);

// Media configuration
int kernel_engine_set_media(kernel_engine_t* engine, const kernel_media_t* media);
int kernel_engine_add_layer(kernel_engine_t* engine, const kernel_layer_t* layer);

// Integration configuration
int kernel_engine_set_integration(kernel_engine_t* engine, 
                                 const kernel_integration_t* integration);

// Frequency configuration
int kernel_engine_set_frequency(kernel_engine_t* engine, double frequency);

// Batch processing
int kernel_engine_compute_impedance_matrix(kernel_engine_t* engine,
                                          const kernel_element_t* elements,
                                          int num_elements,
                                          kernel_complex_t* matrix);

int kernel_engine_compute_partial_elements(kernel_engine_t* engine,
                                          const kernel_element_t* elements,
                                          int num_elements,
                                          kernel_partial_elements_t* elements_matrix);

// Fast algorithm configuration
int kernel_engine_enable_aca(kernel_engine_t* engine, double tolerance, int max_rank);
int kernel_engine_enable_mlfmm(kernel_engine_t* engine, const kernel_mlfmm_params_t* params);

// Performance monitoring
typedef struct {
    int num_kernel_evaluations;
    int num_integration_points;
    double computation_time;
    double memory_usage;
    int compression_ratio;      // For ACA/MLFMM
} kernel_performance_t;

kernel_performance_t kernel_engine_get_performance(const kernel_engine_t* engine);

#ifdef __cplusplus
}
// MFIE kernel functions (see kernel_mfie.h)
#include "kernel_mfie.h"

// CFIE kernel functions (see kernel_cfie.h)
#include "kernel_cfie.h"

// Nearly singular integral functions (see integral_nearly_singular.h)
#include "integral_nearly_singular.h"

#endif

#endif // CORE_KERNELS_H
