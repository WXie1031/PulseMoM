/********************************************************************************
 * Optimized Integration Utilities
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Performance-optimized versions of integration utilities
 * - Static lookup tables for Gauss quadrature points
 * - Adaptive integration precision based on distance
 ********************************************************************************/

#ifndef INTEGRATION_UTILS_OPTIMIZED_H
#define INTEGRATION_UTILS_OPTIMIZED_H

#include "core_common.h"
#include "../../discretization/geometry/core_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get optimal Gauss quadrature order based on distance and element size
 * 
 * @param distance Distance between observation and source
 * @param element_size Characteristic size of element
 * @param is_near_singular Whether near-singular case
 * @return Optimal quadrature order (1, 4, 7, or 8)
 */
int integration_get_optimal_order(double distance, double element_size, int is_near_singular);

/**
 * @brief Get optimal Gauss quadrature order for triangle based on distance ratio
 * 
 * @param distance_ratio Distance / element_size ratio
 * @param is_near_singular Whether near-singular case
 * @return Optimal quadrature order
 */
int integration_get_triangle_order_adaptive(double distance_ratio, int is_near_singular);

/**
 * @brief Initialize static lookup tables for Gauss quadrature (performance optimization)
 * 
 * This function pre-computes and caches commonly used Gauss quadrature points
 * to avoid repeated computation in hot loops
 */
void integration_init_lookup_tables(void);

/**
 * @brief Cleanup lookup tables
 */
void integration_cleanup_lookup_tables(void);

/**
 * @brief Get cached Gauss quadrature points for triangle (if available)
 * 
 * @param order Quadrature order
 * @param points Output array for points
 * @param weights Output array for weights
 * @return 1 if cached data available, 0 otherwise
 */
int integration_get_cached_triangle_quadrature(int order, double points[][2], double* weights);

#ifdef __cplusplus
}
#endif

#endif // INTEGRATION_UTILS_OPTIMIZED_H

