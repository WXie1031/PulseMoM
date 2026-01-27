/********************************************************************************
 * Logarithmic Singularity Integration for MoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Handles logarithmic singularities in 2D problems and edge effects
 * Implements specialized quadrature rules for log(r) type singularities
 ********************************************************************************/

#ifndef INTEGRAL_LOGARITHMIC_SINGULAR_H
#define INTEGRAL_LOGARITHMIC_SINGULAR_H

#include "core_common.h"
#include "../../discretization/geometry/core_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detect logarithmic singularity in integral
 * 
 * @param tri Triangle element
 * @param obs_point Observation point
 * @param threshold Distance threshold for singularity detection
 * @return true if logarithmic singularity is detected
 */
int integral_detect_logarithmic_singular(const geom_triangle_t* tri,
                                         const geom_point_t* obs_point,
                                         double threshold);

/**
 * @brief Compute integral with logarithmic singularity using specialized quadrature
 * 
 * @param tri Triangle element
 * @param obs_point Observation point
 * @param kernel_func Kernel function pointer
 * @param k Wavenumber
 * @param data Additional data for kernel function
 * @return Complex integral value
 */
complex_t integral_logarithmic_singular_triangle(const geom_triangle_t* tri,
                                                  const geom_point_t* obs_point,
                                                  complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                                  double k,
                                                  void* data);

/**
 * @brief Compute logarithmic singular integral using adaptive subdivision
 * 
 * @param tri Triangle element
 * @param obs_point Observation point
 * @param kernel_func Kernel function pointer
 * @param k Wavenumber
 * @param data Additional data for kernel function
 * @param max_depth Maximum subdivision depth
 * @param tolerance Integration tolerance
 * @return Complex integral value
 */
complex_t integral_logarithmic_singular_adaptive(const geom_triangle_t* tri,
                                                 const geom_point_t* obs_point,
                                                 complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                                 double k,
                                                 void* data,
                                                 int max_depth,
                                                 double tolerance);

/**
 * @brief Compute 2D logarithmic singular integral (for edge problems)
 * 
 * @param edge_start Start point of edge
 * @param edge_end End point of edge
 * @param obs_point Observation point
 * @param kernel_func Kernel function pointer
 * @param k Wavenumber
 * @param data Additional data for kernel function
 * @return Complex integral value
 */
complex_t integral_logarithmic_singular_edge(const geom_point_t* edge_start,
                                              const geom_point_t* edge_end,
                                              const geom_point_t* obs_point,
                                              complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                              double k,
                                              void* data);

#ifdef __cplusplus
}
#endif

#endif // INTEGRAL_LOGARITHMIC_SINGULAR_H

