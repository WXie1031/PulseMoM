/********************************************************************************
 * Optimized Integration Utilities Implementation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Performance-optimized integration utilities with lookup tables
 * and adaptive precision
 ********************************************************************************/

#include "integration_utils_optimized.h"
#include "integration_utils.h"
#include "core_common.h"
#include <math.h>
#include <stdlib.h>

// Thresholds for adaptive integration
#define ADAPTIVE_FAR_FIELD_RATIO 10.0      // Far-field: distance > 10 * element_size
#define ADAPTIVE_MID_FIELD_RATIO 2.0       // Mid-field: 2 < distance/element_size < 10
#define ADAPTIVE_NEAR_FIELD_RATIO 0.5      // Near-field: distance < 0.5 * element_size

// Static lookup table structure
typedef struct {
    double triangle_points_1[1][2];
    double triangle_weights_1[1];
    double triangle_points_4[4][2];
    double triangle_weights_4[4];
    double triangle_points_7[7][2];
    double triangle_weights_7[7];
    double triangle_points_8[8][2];
    double triangle_weights_8[8];
    int initialized;
} gauss_lookup_table_t;

static gauss_lookup_table_t g_lookup_table = {0};

/**
 * @brief Get optimal Gauss quadrature order based on distance and element size
 */
int integration_get_optimal_order(double distance, double element_size, int is_near_singular) {
    if (is_near_singular) {
        // Near-singular case: use highest order
        return 8;
    }
    
    if (element_size < NUMERICAL_EPSILON) {
        return 4;  // Default for degenerate elements
    }
    
    double ratio = distance / element_size;
    
    if (ratio > ADAPTIVE_FAR_FIELD_RATIO) {
        // Far-field: use lower order (4 points)
        return 4;
    } else if (ratio > ADAPTIVE_MID_FIELD_RATIO) {
        // Mid-field: use medium order (4 points)
        return 4;
    } else if (ratio > ADAPTIVE_NEAR_FIELD_RATIO) {
        // Near-field: use higher order (7 points)
        return 7;
    } else {
        // Very near-field: use highest order (8 points)
        return 8;
    }
}

/**
 * @brief Get optimal Gauss quadrature order for triangle based on distance ratio
 */
int integration_get_triangle_order_adaptive(double distance_ratio, int is_near_singular) {
    if (is_near_singular) {
        return 8;  // Highest order for near-singular
    }
    
    if (distance_ratio > ADAPTIVE_FAR_FIELD_RATIO) {
        return 4;  // Far-field: 4 points
    } else if (distance_ratio > ADAPTIVE_MID_FIELD_RATIO) {
        return 4;  // Mid-field: 4 points
    } else if (distance_ratio > ADAPTIVE_NEAR_FIELD_RATIO) {
        return 7;  // Near-field: 7 points
    } else {
        return 8;  // Very near-field: 8 points
    }
}

/**
 * @brief Initialize static lookup tables
 */
void integration_init_lookup_tables(void) {
    if (g_lookup_table.initialized) {
        return;  // Already initialized
    }
    
    // Pre-compute and cache commonly used Gauss quadrature points
    gauss_quadrature_triangle(1, g_lookup_table.triangle_points_1, g_lookup_table.triangle_weights_1);
    gauss_quadrature_triangle(4, g_lookup_table.triangle_points_4, g_lookup_table.triangle_weights_4);
    gauss_quadrature_triangle(7, g_lookup_table.triangle_points_7, g_lookup_table.triangle_weights_7);
    gauss_quadrature_triangle(8, g_lookup_table.triangle_points_8, g_lookup_table.triangle_weights_8);
    
    g_lookup_table.initialized = 1;
}

/**
 * @brief Cleanup lookup tables
 */
void integration_cleanup_lookup_tables(void) {
    // Reset initialization flag
    g_lookup_table.initialized = 0;
}

/**
 * @brief Get cached Gauss quadrature points for triangle
 */
int integration_get_cached_triangle_quadrature(int order, double points[][2], double* weights) {
    if (!g_lookup_table.initialized) {
        integration_init_lookup_tables();
    }
    
    if (!points || !weights) {
        return 0;
    }
    
    switch (order) {
        case 1:
            for (int i = 0; i < 1; i++) {
                points[i][0] = g_lookup_table.triangle_points_1[i][0];
                points[i][1] = g_lookup_table.triangle_points_1[i][1];
                weights[i] = g_lookup_table.triangle_weights_1[i];
            }
            return 1;
            
        case 4:
            for (int i = 0; i < 4; i++) {
                points[i][0] = g_lookup_table.triangle_points_4[i][0];
                points[i][1] = g_lookup_table.triangle_points_4[i][1];
                weights[i] = g_lookup_table.triangle_weights_4[i];
            }
            return 1;
            
        case 7:
            for (int i = 0; i < 7; i++) {
                points[i][0] = g_lookup_table.triangle_points_7[i][0];
                points[i][1] = g_lookup_table.triangle_points_7[i][1];
                weights[i] = g_lookup_table.triangle_weights_7[i];
            }
            return 1;
            
        case 8:
            for (int i = 0; i < 8; i++) {
                points[i][0] = g_lookup_table.triangle_points_8[i][0];
                points[i][1] = g_lookup_table.triangle_points_8[i][1];
                weights[i] = g_lookup_table.triangle_weights_8[i];
            }
            return 1;
            
        default:
            return 0;  // Not cached, use regular function
    }
}

