/********************************************************************************
 * Physical Constants for PulseMoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file contains physical constants used across all layers.
 * L1 Physics layer should reference these constants.
 ********************************************************************************/

#ifndef COMMON_CONSTANTS_H
#define COMMON_CONSTANTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mathematical constants
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Physical constants (SI units)
#define C0 299792458.0          // Speed of light in vacuum (m/s)
#define MU0 (4.0 * M_PI * 1e-7)  // Permeability of free space (H/m)
#define EPS0 (1.0 / (MU0 * C0 * C0))  // Permittivity of free space (F/m)
#define ETA0 (MU0 * C0)         // Impedance of free space (Ohms)
#define Z0 ETA0                 // Alternative notation

// Numerical tolerance constants
#define GEOMETRIC_EPSILON 1e-9      // Geometric comparison tolerance
#define AREA_EPSILON 1e-15          // Minimum valid triangle/area threshold
#define DISTANCE_EPSILON 1e-12      // Small epsilon for distance calculations
#define NUMERICAL_EPSILON 1e-12     // General numerical stability epsilon
#define MATRIX_PIVOT_EPSILON_SQ 1e-24  // Square of pivot threshold for matrix operations
#define REGULARIZATION_MIN 1e-9     // Minimum regularization value
#define REGULARIZATION_MAX 1e-3     // Maximum regularization value
#define DEFAULT_AREA_FALLBACK 1e-6  // Default area value when triangle area is not available
#define CONVERGENCE_TOLERANCE_DEFAULT 1e-6  // Default convergence tolerance for iterative solvers

// Mathematical constants
#define ONE_THIRD (1.0 / 3.0)       // One third constant

#ifdef __cplusplus
}
#endif

#endif // COMMON_CONSTANTS_H
