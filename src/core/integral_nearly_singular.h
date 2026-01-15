/**
 * @file integral_nearly_singular.h
 * @brief Nearly singular integral handling declarations
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef INTEGRAL_NEARLY_SINGULAR_H
#define INTEGRAL_NEARLY_SINGULAR_H

#include "core_common.h"
#include "core_geometry.h"
#include "core_kernels.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detect if two elements are in near-singular configuration
 */
bool integral_detect_nearly_singular(const geom_triangle_t* elem1,
                                     const geom_triangle_t* elem2,
                                     double threshold);

/**
 * @brief Compute near-singular integral using high-order quadrature
 */
complex_t integral_nearly_singular_triangle(const geom_triangle_t* tri,
                                           const geom_point_t* obs_point,
                                           complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                           double k,
                                           void* kernel_data);

/**
 * @brief Compute near-singular integral with adaptive subdivision
 */
complex_t integral_nearly_singular_adaptive(const geom_triangle_t* tri,
                                           const geom_point_t* obs_point,
                                           complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                           double k,
                                           void* kernel_data,
                                           int max_subdivisions,
                                           double tolerance);

#ifdef __cplusplus
}
#endif

#endif // INTEGRAL_NEARLY_SINGULAR_H
