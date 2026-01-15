/**
 * @file kernel_cfie.h
 * @brief Combined Field Integral Equation (CFIE) kernel declarations
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef KERNEL_CFIE_H
#define KERNEL_CFIE_H

#include "core_common.h"
#include "core_geometry.h"
#include "core_kernels.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute CFIE combined kernel: α * EFIE + (1-α) * MFIE
 */
complex_t kernel_cfie_combined(const geom_triangle_t* tri,
                               const geom_point_t* obs_point,
                               const geom_point_t* n_prime,
                               double k,
                               double alpha,
                               complex_t efie_kernel,
                               complex_t mfie_kernel);

/**
 * @brief Compute CFIE triangle integral
 */
complex_t kernel_cfie_triangle_integral(const geom_triangle_t* tri,
                                        const geom_point_t* obs_point,
                                        const geom_point_t* n_prime,
                                        double k,
                                        double alpha);

/**
 * @brief Compute CFIE self-term
 * @param tri Triangle element
 * @param alpha Combination parameter (0.0 = pure MFIE, 1.0 = pure EFIE)
 * @param k Wave number (2πf/c), if <= 0.0 uses default frequency
 * @return Combined self-term
 */
complex_t kernel_cfie_self_term(const geom_triangle_t* tri, double alpha, double k);

/**
 * @brief Get recommended CFIE alpha parameter
 */
double kernel_cfie_get_recommended_alpha(double frequency, 
                                        geom_element_type_t geometry_type);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_CFIE_H
