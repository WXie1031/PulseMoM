/********************************************************************************
 * PulseEM - Integration Utility Functions
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * E-mail: chenhc@seu.edu.cn 
 * 
 * File: integration_utils.c
 * Description: Utility functions for numerical integration
 * 
 * Functions:
 * - Gaussian quadrature for 1D integration
 * - 3D distance computation
 * - Vector operations (dot product, cross product)
 * - Triangle geometry utilities
 ********************************************************************************/

#include "integration_utils.h"
#include "../../common/core_common.h"
#include "../kernels/electromagnetic_kernels.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>

// Helper macro for complex number creation
static inline complex_t mc(double re, double im) {
    complex_t z;
    z.re = re;
    z.im = im;
    return z;
}

/********************************************************************************
 * Gaussian Quadrature for 1D Integration
 * Returns points and weights for standard Gaussian quadrature on [-1, 1]
 * This function must be defined before quadrilateral and hexahedron functions
 ********************************************************************************/
void gauss_quadrature_1d(int n, double* points, double* weights) {
    if (!points || !weights || n < 2) return;
    
    // Standard Gaussian quadrature for [-1, 1]
    switch (n) {
        case 2:
            points[0] = -0.577350269189626;
            points[1] = 0.577350269189626;
            weights[0] = 1.0;
            weights[1] = 1.0;
            break;
        case 3:
            points[0] = -0.774596669241483;
            points[1] = 0.0;
            points[2] = 0.774596669241483;
            weights[0] = 0.555555555555556;
            weights[1] = 0.888888888888889;
            weights[2] = 0.555555555555556;
            break;
        case 4:
            points[0] = -0.861136311594053;
            points[1] = -0.339981043584856;
            points[2] = 0.339981043584856;
            points[3] = 0.861136311594053;
            weights[0] = 0.347854845137454;
            weights[1] = 0.652145154862546;
            weights[2] = 0.652145154862546;
            weights[3] = 0.347854845137454;
            break;
        case 5:
            points[0] = -0.906179845938664;
            points[1] = -0.538469310105683;
            points[2] = 0.0;
            points[3] = 0.538469310105683;
            points[4] = 0.906179845938664;
            weights[0] = 0.236926885056189;
            weights[1] = 0.478628670499366;
            weights[2] = 0.568888888888889;
            weights[3] = 0.478628670499366;
            weights[4] = 0.236926885056189;
            break;
        default:
            // Default to 4-point
            if (n >= 4) {
                gauss_quadrature_1d(4, points, weights);
            } else {
                gauss_quadrature_1d(2, points, weights);
            }
            break;
    }
}

/********************************************************************************
 * Gaussian Quadrature for Triangular Domains — Dunavant (1985) style rules
 * (u,v) = (λ1, λ2), w = 1-u-v; weights sum to 1 for ∫_T f dS ≈ A Σ w_i f(r_i).
 * order 8 = degree-6 rule with 12 nodes (Burkhardt dunavant_subrule_06).
 ********************************************************************************/
void gauss_quadrature_triangle(int order, double points[][2], double* weights) {
    if (!points || !weights || order < 1) return;
    
    switch (order) {
        case 1: {
            points[0][0] = 1.0/3.0;
            points[0][1] = 1.0/3.0;
            weights[0] = 1.0;
            break;
        }
        case 4: {
            points[0][0] = 1.0/3.0;
            points[0][1] = 1.0/3.0;
            points[1][0] = 0.6;
            points[1][1] = 0.2;
            points[2][0] = 0.2;
            points[2][1] = 0.6;
            points[3][0] = 0.2;
            points[3][1] = 0.2;
            weights[0] = 0.25;
            weights[1] = 0.25;
            weights[2] = 0.25;
            weights[3] = 0.25;
            break;
        }
        case 7: {
            const double a = 0.059715871789770;
            const double b = 0.470142064105115;
            const double c = 0.797426985353087;
            const double d = 0.101286507323456;
            points[0][0] = 1.0/3.0;
            points[0][1] = 1.0/3.0;
            weights[0] = 0.225;
            points[1][0] = b;
            points[1][1] = b;
            weights[1] = 0.132394152788506;
            points[2][0] = a;
            points[2][1] = b;
            weights[2] = 0.132394152788506;
            points[3][0] = b;
            points[3][1] = a;
            weights[3] = 0.132394152788506;
            points[4][0] = d;
            points[4][1] = d;
            weights[4] = 0.125939180544827;
            points[5][0] = d;
            points[5][1] = c;
            weights[5] = 0.125939180544827;
            points[6][0] = c;
            points[6][1] = d;
            weights[6] = 0.125939180544827;
            break;
        }
        case 8: {
            const double a1 = 0.501426509658179;
            const double b1 = 0.249286745170910;
            const double a2 = 0.873821971016996;
            const double b2 = 0.063089014491502;
            const double p00 = 0.053145049844817;
            const double p01 = 0.310352451033784;
            const double p02 = 0.636502499121399;
            const double w1 = 0.116786275726379;
            const double w2 = 0.050844906370207;
            const double w3 = 0.082851075618374;
            points[0][0] = b1;  points[0][1] = b1;  weights[0] = w1;
            points[1][0] = a1;  points[1][1] = b1;  weights[1] = w1;
            points[2][0] = b1;  points[2][1] = a1;  weights[2] = w1;
            points[3][0] = b2;  points[3][1] = b2;  weights[3] = w2;
            points[4][0] = a2;  points[4][1] = b2;  weights[4] = w2;
            points[5][0] = b2;  points[5][1] = a2;  weights[5] = w2;
            points[6][0] = p01;  points[6][1] = p02;  weights[6] = w3;
            points[7][0] = p02;  points[7][1] = p01;  weights[7] = w3;
            points[8][0] = p00;  points[8][1] = p02;  weights[8] = w3;
            points[9][0] = p02;  points[9][1] = p00;  weights[9] = w3;
            points[10][0] = p00;  points[10][1] = p01;  weights[10] = w3;
            points[11][0] = p01;  points[11][1] = p00;  weights[11] = w3;
            break;
        }
        default: {
            if (order >= 4) {
                gauss_quadrature_triangle(4, points, weights);
            } else {
                gauss_quadrature_triangle(1, points, weights);
            }
            break;
        }
    }
}

int gauss_quadrature_triangle_num_points(int order) {
    switch (order) {
        case 1: return 1;
        case 4: return 4;
        case 7: return 7;
        case 8: return 12;
        default:
            if (order >= 4) return 4;
            return 1;
    }
}

/********************************************************************************
 * Gaussian Quadrature for Quadrilateral Domains
 * Returns points and weights in parametric coordinates (u, v) in [-1, 1]^2
 * Uses tensor product of 1D Gauss quadrature
 ********************************************************************************/
void gauss_quadrature_quadrilateral(int order, double points[][2], double* weights) {
    if (!points || !weights || order < 1) return;
    
    // Get 1D Gauss quadrature points and weights
    double* u_1d = (double*)malloc(order * sizeof(double));
    double* w_1d = (double*)malloc(order * sizeof(double));
    if (!u_1d || !w_1d) {
        if (u_1d) free(u_1d);
        if (w_1d) free(w_1d);
        return;
    }
    
    gauss_quadrature_1d(order, u_1d, w_1d);
    
    // Tensor product: create 2D points from 1D points
    int idx = 0;
    for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
            points[idx][0] = u_1d[i];
            points[idx][1] = u_1d[j];
            weights[idx] = w_1d[i] * w_1d[j];
            idx++;
        }
    }
    
    free(u_1d);
    free(w_1d);
}

/********************************************************************************
 * Gaussian Quadrature for Hexahedral Domains
 * Returns points and weights in parametric coordinates (u, v, w) in [-1, 1]^3
 * Uses tensor product of 1D Gauss quadrature
 ********************************************************************************/
void gauss_quadrature_hexahedron(int order, double points[][3], double* weights) {
    if (!points || !weights || order < 1) return;
    
    // Get 1D Gauss quadrature points and weights
    double* u_1d = (double*)malloc(order * sizeof(double));
    double* w_1d = (double*)malloc(order * sizeof(double));
    if (!u_1d || !w_1d) {
        if (u_1d) free(u_1d);
        if (w_1d) free(w_1d);
        return;
    }
    
    gauss_quadrature_1d(order, u_1d, w_1d);
    
    // Tensor product: create 3D points from 1D points
    int idx = 0;
    for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
            for (int k = 0; k < order; k++) {
                points[idx][0] = u_1d[i];
                points[idx][1] = u_1d[j];
                points[idx][2] = u_1d[k];
                weights[idx] = w_1d[i] * w_1d[j] * w_1d[k];
                idx++;
            }
        }
    }
    
    free(u_1d);
    free(w_1d);
}

/********************************************************************************
 * 3D Distance Computation
 ********************************************************************************/
double compute_distance_3d(const double* p1, const double* p2) {
    if (!p1 || !p2) return 0.0;
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dz = p2[2] - p1[2];
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/********************************************************************************
 * Vector Operations
 ********************************************************************************/

// Dot product of two 3D vectors (using double arrays)
// Renamed from vector3d_dot to vector3d_dot_array to avoid conflict with
// core_common.h's vector3d_dot(point3d_t*, point3d_t*)
double vector3d_dot_array(const double* v1, const double* v2) {
    if (!v1 || !v2) return 0.0;
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

// Cross product of two 3D vectors: result = v1 × v2
void vector3d_cross_array(const double* v1, const double* v2, double* result) {
    if (!v1 || !v2 || !result) return;
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

// Get triangle centroid
void get_triangle_centroid(const triangle_element_t* tri, double* centroid) {
    if (!tri || !centroid) return;
    centroid[0] = (tri->vertices[0][0] + tri->vertices[1][0] + tri->vertices[2][0]) / 3.0;
    centroid[1] = (tri->vertices[0][1] + tri->vertices[1][1] + tri->vertices[2][1]) / 3.0;
    centroid[2] = (tri->vertices[0][2] + tri->vertices[1][2] + tri->vertices[2][2]) / 3.0;
}

// Vertex opposite to local edge edge_idx: edge k connects vertices k and (k+1)%3, opposite is (k+2)%3
void get_opposite_vertex(const triangle_element_t* tri, int edge_idx, double* vertex) {
    if (!tri || !vertex) return;
    if (edge_idx < 0 || edge_idx > 2) {
        edge_idx = 0;
    }
    int opp = (edge_idx + 2) % 3;
    vertex[0] = tri->vertices[opp][0];
    vertex[1] = tri->vertices[opp][1];
    vertex[2] = tri->vertices[opp][2];
}

// Helper macro for complex number creation (exported for use in other modules)
complex_t make_complex(double re, double im) {
    complex_t z;
    z.re = re;
    z.im = im;
    return z;
}

