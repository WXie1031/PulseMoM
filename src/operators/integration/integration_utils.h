/********************************************************************************
 * PulseEM - Integration Utility Functions Header
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: integration_utils.h
 * Description: Utility functions for numerical integration
 ********************************************************************************/

#ifndef INTEGRATION_UTILS_H
#define INTEGRATION_UTILS_H

#include "../../common/core_common.h"
// Include electromagnetic_kernels.h for full definitions (triangle_element_t is defined there)
#include "../kernels/electromagnetic_kernels.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gaussian quadrature for 1D integration
void gauss_quadrature_1d(int n, double* points, double* weights);

// 3D distance computation
double compute_distance_3d(const double* p1, const double* p2);

// Vector operations (using double arrays, not point3d_t)
// Note: These are different from vector3d_dot/vector3d_cross_point in core_common.h
// which use point3d_t* parameters. This version uses double* arrays.
// Renamed to avoid conflict with core_common.h's vector3d_dot(point3d_t*, point3d_t*)
double vector3d_dot_array(const double* v1, const double* v2);
void vector3d_cross_array(const double* v1, const double* v2, double* result);

// Triangle geometry utilities
void get_triangle_centroid(const triangle_element_t* tri, double* centroid);
void get_opposite_vertex(const triangle_element_t* tri, int edge_idx, double* vertex);

// Gaussian quadrature for triangular domains (barycentric coordinates)
// Returns points in barycentric coordinates (u, v) where w = 1 - u - v
// order: 1, 4, 7, or 8 (number of quadrature points)
// points: output array of size [order][2] for (u, v) coordinates
// weights: output array of size [order] for quadrature weights
void gauss_quadrature_triangle(int order, double points[][2], double* weights);

// Gaussian quadrature for quadrilateral domains (parametric coordinates)
// Returns points in parametric coordinates (u, v) in [-1, 1]^2
// order: number of points per dimension (typically 2, 3, or 4)
// points: output array of size [order*order][2] for (u, v) coordinates
// weights: output array of size [order*order] for quadrature weights
void gauss_quadrature_quadrilateral(int order, double points[][2], double* weights);

// Gaussian quadrature for hexahedral domains (parametric coordinates)
// Returns points in parametric coordinates (u, v, w) in [-1, 1]^3
// order: number of points per dimension (typically 2, 3, or 4)
// points: output array of size [order*order*order][3] for (u, v, w) coordinates
// weights: output array of size [order*order*order] for quadrature weights
void gauss_quadrature_hexahedron(int order, double points[][3], double* weights);

// Complex number helper
complex_t make_complex(double re, double im);

#ifdef __cplusplus
}
#endif

#endif // INTEGRATION_UTILS_H

