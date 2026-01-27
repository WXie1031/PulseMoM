/**
 * @file peec_directionality_kernels.h
 * @brief PEEC directionality kernels declarations
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef PEEC_DIRECTIONALITY_KERNELS_H
#define PEEC_DIRECTIONALITY_KERNELS_H

#include "peec_solver.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../common/core_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute directional partial inductance
 */
complex_t peec_directional_partial_inductance(const geom_line_t* seg1,
                                              const geom_line_t* seg2,
                                              const geom_point_t* direction1,
                                              const geom_point_t* direction2);

/**
 * @brief Compute directional coupling coefficient
 */
double peec_directional_coupling_coefficient(const void* cond1,
                                            const void* cond2,
                                            const geom_point_t* direction1,
                                            const geom_point_t* direction2);

/**
 * @brief Compute frequency-dependent directional coupling
 */
complex_t peec_directional_coupling_frequency(const geom_line_t* seg1,
                                               const geom_line_t* seg2,
                                               const geom_point_t* direction1,
                                               const geom_point_t* direction2,
                                               double frequency);

#ifdef __cplusplus
}
#endif

#endif // PEEC_DIRECTIONALITY_KERNELS_H
