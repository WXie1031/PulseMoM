/*****************************************************************************************
 * PEEC Numerical Integration Header
 *****************************************************************************************/

#ifndef PEEC_INTEGRATION_H
#define PEEC_INTEGRATION_H

#include "../../common/core_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct peec_wire_element peec_wire_element_t;

// MSVC-compatible complex type
#if defined(_MSC_VER)
typedef complex_t peec_complex_t;
#else
#include <complex.h>
typedef double complex peec_complex_t;
#endif

// Partial inductance computation using Neumann's formula (complete integral)
peec_complex_t peec_compute_partial_inductance_wire_integral(
    const point3d_t* start1, const point3d_t* end1,
    const point3d_t* start2, const point3d_t* end2,
    double radius1, double radius2);

// Partial capacitance for quadrilateral using double surface integral
double peec_compute_partial_capacitance_quad_integral(
    const point3d_t* vertices1, int num_vertices1,
    const point3d_t* vertices2, int num_vertices2,
    double area1, double area2);

// Partial inductance for quadrilateral using double surface integral
peec_complex_t peec_compute_partial_inductance_quad_integral(
    const point3d_t* vertices1, int num_vertices1,
    const point3d_t* vertices2, int num_vertices2,
    const point3d_t* normal1, const point3d_t* normal2,
    double area1, double area2);

// Partial capacitance for triangle using double surface integral
double peec_compute_partial_capacitance_triangle_integral(
    const point3d_t* v1, const point3d_t* v2, const point3d_t* v3,
    const point3d_t* v1_obs, const point3d_t* v2_obs, const point3d_t* v3_obs,
    double area1, double area2);

#ifdef __cplusplus
}
#endif

#endif // PEEC_INTEGRATION_H

