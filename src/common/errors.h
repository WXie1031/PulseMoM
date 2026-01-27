/********************************************************************************
 * Error Codes for PulseMoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file contains error code definitions shared across all layers.
 ********************************************************************************/

#ifndef COMMON_ERRORS_H
#define COMMON_ERRORS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Error code type (same as status_t, but kept for backward compatibility)
typedef status_t error_code_t;

// Legacy error codes (for backward compatibility)
#define CORE_SUCCESS STATUS_SUCCESS
#define CORE_ERROR_INVALID_INPUT STATUS_ERROR_INVALID_INPUT
#define CORE_ERROR_MEMORY_ALLOCATION STATUS_ERROR_MEMORY_ALLOCATION
#define CORE_ERROR_FILE_NOT_FOUND STATUS_ERROR_FILE_NOT_FOUND
#define CORE_ERROR_INVALID_FORMAT STATUS_ERROR_INVALID_FORMAT
#define CORE_ERROR_NUMERICAL_INSTABILITY STATUS_ERROR_NUMERICAL_INSTABILITY
#define CORE_ERROR_CONVERGENCE_FAILURE STATUS_ERROR_CONVERGENCE_FAILURE
#define CORE_ERROR_INVALID_STATE STATUS_ERROR_INVALID_STATE

#ifdef __cplusplus
}
#endif

#endif // COMMON_ERRORS_H
