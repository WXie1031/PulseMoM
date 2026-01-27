/*****************************************************************************************
 * PEEC Numerical Integration for Partial Elements
 * 
 * This module provides PEEC-specific integration wrappers that call the unified
 * integration functions from electromagnetic_kernels library.
 * 
 * All numerical integration is now performed by the core integration library,
 * ensuring consistency between MoM and PEEC methods.
 *****************************************************************************************/

#include "peec_solver.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../common/core_common.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include "peec_geometry_support.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// MSVC-compatible complex number handling
#if defined(_MSC_VER)
typedef complex_t peec_complex_t;
#else
#include <complex.h>
typedef double complex peec_complex_t;
#endif

// Physical constants
#define MU0 (4.0 * M_PI * 1e-7)
#define EPS0 (8.854187817e-12)

/********************************************************************************
 * Helper Functions: Convert PEEC Geometry to Core Integration Structures
 ********************************************************************************/

// Convert point3d_t to double array
static void point3d_to_array(const point3d_t* p, double arr[3]) {
    arr[0] = p->x;
    arr[1] = p->y;
    arr[2] = p->z;
}

// Convert complex_t to peec_complex_t
static peec_complex_t convert_complex_to_peec(complex_t c) {
    #if defined(_MSC_VER)
    return (peec_complex_t){c.re, c.im};
    #else
    return c.re + c.im * I;
    #endif
}

// Convert wire element to wire_element_t for core library
static void convert_peec_wire_to_wire_element(
    const point3d_t* start, const point3d_t* end, double radius, double length,
    wire_element_t* wire_elem) {
    
    point3d_to_array(start, wire_elem->start);
    point3d_to_array(end, wire_elem->end);
    wire_elem->radius = radius;
    wire_elem->length = length;
}

// Convert triangle vertices to triangle_element_t
static void convert_triangle_to_triangle_element(
    const point3d_t* v1, const point3d_t* v2, const point3d_t* v3, double area,
    triangle_element_t* tri_elem) {
    
    point3d_to_array(v1, tri_elem->vertices[0]);
    point3d_to_array(v2, tri_elem->vertices[1]);
    point3d_to_array(v3, tri_elem->vertices[2]);
    
    // Compute normal (simplified)
    double v1v2[3] = {v2->x - v1->x, v2->y - v1->y, v2->z - v1->z};
    double v1v3[3] = {v3->x - v1->x, v3->y - v1->y, v3->z - v1->z};
    tri_elem->normal[0] = v1v2[1] * v1v3[2] - v1v2[2] * v1v3[1];
    tri_elem->normal[1] = v1v2[2] * v1v3[0] - v1v2[0] * v1v3[2];
    tri_elem->normal[2] = v1v2[0] * v1v3[1] - v1v2[1] * v1v3[0];
    
    tri_elem->area = area;
}

// Convert quadrilateral vertices to rectangle_element_t
static void convert_quad_to_rectangle_element(
    const point3d_t* vertices, int num_vertices, double area,
    rectangle_element_t* rect_elem) {
    
    if (num_vertices >= 4) {
        point3d_to_array(&vertices[0], rect_elem->vertices[0]);
        point3d_to_array(&vertices[1], rect_elem->vertices[1]);
        point3d_to_array(&vertices[2], rect_elem->vertices[2]);
        point3d_to_array(&vertices[3], rect_elem->vertices[3]);
    }
    
    // Compute normal (simplified)
    double v01[3] = {vertices[1].x - vertices[0].x, 
                     vertices[1].y - vertices[0].y, 
                     vertices[1].z - vertices[0].z};
    double v03[3] = {vertices[3].x - vertices[0].x, 
                     vertices[3].y - vertices[0].y, 
                     vertices[3].z - vertices[0].z};
    rect_elem->normal[0] = v01[1] * v03[2] - v01[2] * v03[1];
    rect_elem->normal[1] = v01[2] * v03[0] - v01[0] * v03[2];
    rect_elem->normal[2] = v01[0] * v03[1] - v01[1] * v03[0];
    
    rect_elem->area = area;
}

/********************************************************************************
 * Enhanced Partial Inductance for Wire (Using Public Integration Library)
 ********************************************************************************/
peec_complex_t peec_compute_partial_inductance_wire_integral(
    const point3d_t* start1, const point3d_t* end1,
    const point3d_t* start2, const point3d_t* end2,
    double radius1, double radius2) {
    
    if (!start1 || !end1 || !start2 || !end2) {
        #if defined(_MSC_VER)
        return (peec_complex_t){0.0, 0.0};
        #else
        return 0.0 + 0.0*I;
        #endif
    }
    
    // Convert to core library format
    wire_element_t wire1, wire2;
    double length1 = sqrt((end1->x - start1->x)*(end1->x - start1->x) +
                          (end1->y - start1->y)*(end1->y - start1->y) +
                          (end1->z - start1->z)*(end1->z - start1->z));
    double length2 = sqrt((end2->x - start2->x)*(end2->x - start2->x) +
                          (end2->y - start2->y)*(end2->y - start2->y) +
                          (end2->z - start2->z)*(end2->z - start2->z));
    
    convert_peec_wire_to_wire_element(start1, end1, radius1, length1, &wire1);
    convert_peec_wire_to_wire_element(start2, end2, radius2, length2, &wire2);
    
    // Call public integration function
    double k = 0.0;  // DC case for PEEC partial inductance
    complex_t result = integrate_wire_wire_neumann(&wire1, &wire2, k);
    
    return convert_complex_to_peec(result);
}

/********************************************************************************
 * Enhanced Partial Capacitance for Quadrilateral (Using Public Integration Library)
 ********************************************************************************/
double peec_compute_partial_capacitance_quad_integral(
    const point3d_t* vertices1, int num_vertices1,
    const point3d_t* vertices2, int num_vertices2,
    double area1, double area2) {
    
    if (!vertices1 || !vertices2 || num_vertices1 < 4 || num_vertices2 < 4) {
        return 0.0;
    }
    
    // Convert to core library format
    rectangle_element_t quad1, quad2;
    convert_quad_to_rectangle_element(vertices1, num_vertices1, area1, &quad1);
    convert_quad_to_rectangle_element(vertices2, num_vertices2, area2, &quad2);
    
    // For capacitance, we need double surface integral
    // Use triangle approximation for now (can be enhanced later)
    triangle_element_t tri1, tri2;
    
    // Split quad into two triangles
    convert_triangle_to_triangle_element(
        &vertices1[0], &vertices1[1], &vertices1[2], area1 * 0.5, &tri1);
    
    convert_triangle_to_triangle_element(
        &vertices2[0], &vertices2[1], &vertices2[2], area2 * 0.5, &tri2);
    
    double k = 0.0;  // DC case for PEEC partial capacitance
    double result = integrate_surface_surface_capacitance(&tri1, &tri2, k);
    
    // Add contribution from second triangle pair
    triangle_element_t tri1b, tri2b;
    convert_triangle_to_triangle_element(
        &vertices1[0], &vertices1[2], &vertices1[3], area1 * 0.5, &tri1b);
    convert_triangle_to_triangle_element(
        &vertices2[0], &vertices2[2], &vertices2[3], area2 * 0.5, &tri2b);
    
    result += integrate_surface_surface_capacitance(&tri1b, &tri2b, k);
    
    return result;
}

/********************************************************************************
 * Enhanced Partial Inductance for Quadrilateral (Using Public Integration Library)
 ********************************************************************************/
peec_complex_t peec_compute_partial_inductance_quad_integral(
    const point3d_t* vertices1, int num_vertices1,
    const point3d_t* vertices2, int num_vertices2,
    const point3d_t* normal1, const point3d_t* normal2,
    double area1, double area2) {
    
    if (!vertices1 || !vertices2 || num_vertices1 < 4 || num_vertices2 < 4) {
        #if defined(_MSC_VER)
        return (peec_complex_t){0.0, 0.0};
        #else
        return 0.0 + 0.0*I;
        #endif
    }
    
    // Convert to core library format
    // Use triangle approximation for now
    triangle_element_t tri1, tri2;
    
    // Split quad into two triangles
    convert_triangle_to_triangle_element(
        &vertices1[0], &vertices1[1], &vertices1[2], area1 * 0.5, &tri1);
    convert_triangle_to_triangle_element(
        &vertices2[0], &vertices2[1], &vertices2[2], area2 * 0.5, &tri2);
    
    double k = 0.0;  // DC case for PEEC partial inductance
    complex_t result = integrate_surface_surface_inductance(&tri1, &tri2, k, 4);  // Default order for PEEC
    
    // Add contribution from second triangle pair
    triangle_element_t tri1b, tri2b;
    convert_triangle_to_triangle_element(
        &vertices1[0], &vertices1[2], &vertices1[3], area1 * 0.5, &tri1b);
    convert_triangle_to_triangle_element(
        &vertices2[0], &vertices2[2], &vertices2[3], area2 * 0.5, &tri2b);
    
    complex_t result2 = integrate_surface_surface_inductance(&tri1b, &tri2b, k, 4);  // Default order for PEEC
    result.re += result2.re;
    result.im += result2.im;
    
    return convert_complex_to_peec(result);
}

/********************************************************************************
 * Enhanced Partial Capacitance for Triangle (Using Public Integration Library)
 ********************************************************************************/
double peec_compute_partial_capacitance_triangle_integral(
    const point3d_t* v1, const point3d_t* v2, const point3d_t* v3,
    const point3d_t* v1_obs, const point3d_t* v2_obs, const point3d_t* v3_obs,
    double area1, double area2) {
    
    if (!v1 || !v2 || !v3 || !v1_obs || !v2_obs || !v3_obs) {
        return 0.0;
    }
    
    // Convert to core library format
    triangle_element_t tri1, tri2;
    convert_triangle_to_triangle_element(v1, v2, v3, area1, &tri1);
    convert_triangle_to_triangle_element(v1_obs, v2_obs, v3_obs, area2, &tri2);
    
    // Call public integration function
    double k = 0.0;  // DC case for PEEC partial capacitance
    return integrate_surface_surface_capacitance(&tri1, &tri2, k);
}
