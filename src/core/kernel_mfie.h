/**
 * @file kernel_mfie.h
 * @brief Magnetic Field Integral Equation (MFIE) kernel declarations
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef KERNEL_MFIE_H
#define KERNEL_MFIE_H

#include "core_common.h"
#include "core_geometry.h"
#include "core_kernels.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute Neumann boundary term: n' · ∇'G(r, r')
 */
complex_t kernel_mfie_neumann_term(const geom_point_t* r,
                                   const geom_point_t* r_prime,
                                   const geom_point_t* n_prime,
                                   double k);

/**
 * @brief Compute double gradient kernel: ∇∇G(r, r')
 * @param result Output 3x3 tensor (9 elements: xx, xy, xz, yx, yy, yz, zx, zy, zz)
 */
void kernel_mfie_double_gradient(const geom_point_t* r,
                                 const geom_point_t* r_prime,
                                 double k,
                                 complex_t* result);

/**
 * @brief Compute MFIE surface integral over triangle
 */
complex_t kernel_mfie_triangle_integral(const geom_triangle_t* tri,
                                       const geom_point_t* obs_point,
                                       const geom_point_t* n_prime,
                                       double k);

/**
 * @brief Compute MFIE self-term (principal value for closed surfaces)
 */
complex_t kernel_mfie_self_term(const geom_triangle_t* tri);

/**
 * @brief MFIE Duffy transform for triangle-triangle integration with 1/R³ singularity
 * @param tri_i First triangle (observation triangle)
 * @param tri_j Second triangle (source triangle)
 * @param frequency Operating frequency
 * @param threshold Distance threshold for Duffy transform
 * @return Complex integral value
 * 
 * This function implements a specialized Duffy transform for MFIE kernels
 * that have 1/R³ singularity (stronger than EFIE's 1/R singularity).
 * The transformation uses a modified Jacobian to handle the stronger singularity.
 */
complex_t kernel_mfie_duffy_transform(const geom_triangle_t* tri_i,
                                     const geom_triangle_t* tri_j,
                                     double frequency,
                                     double threshold);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_MFIE_H
