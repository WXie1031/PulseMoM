/********************************************************************************
 * Common Type Definitions for PulseMoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file contains basic type definitions shared across all layers.
 * This is the ONLY file that should define fundamental types.
 ********************************************************************************/

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Precision definitions
typedef double real_t;

// Complex number type (platform-dependent)
#if defined(_MSC_VER)
typedef struct { 
    double re; 
    double im; 
} complex_t;
#else
#include <complex.h>
typedef double complex complex_t;
#endif

// 3D point/vector
typedef struct {
    double x, y, z;
} point3d_t;

// 3D vector with complex components
typedef struct {
    complex_t x, y, z;
} vector3d_complex_t;

// Status codes
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR_INVALID_INPUT = -1,
    STATUS_ERROR_MEMORY_ALLOCATION = -2,
    STATUS_ERROR_FILE_NOT_FOUND = -3,
    STATUS_ERROR_INVALID_FORMAT = -4,
    STATUS_ERROR_NUMERICAL_INSTABILITY = -5,
    STATUS_ERROR_CONVERGENCE_FAILURE = -6,
    STATUS_ERROR_NOT_IMPLEMENTED = -7,
    STATUS_ERROR_INVALID_STATE = -8
} status_t;

#ifdef __cplusplus
}
#endif

#endif // COMMON_TYPES_H
