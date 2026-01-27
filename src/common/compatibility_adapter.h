/********************************************************************************
 * Compatibility Adapter (Temporary)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file provides backward compatibility during migration.
 * It maps old include paths to new architecture paths.
 *
 * NOTE: This is a temporary file. Remove after full migration.
 ********************************************************************************/

#ifndef COMPATIBILITY_ADAPTER_H
#define COMPATIBILITY_ADAPTER_H

// Map old core_common.h to new common/types.h
#ifndef CORE_COMMON_H
#define CORE_COMMON_H
#include "common/types.h"
#include "common/constants.h"
#include "common/errors.h"

// Type aliases for backward compatibility
#define real_t real_t
#define complex_t complex_t
#define point3d_t point3d_t
#define status_t status_t
#define error_code_t status_t

// Legacy error codes
#define CORE_SUCCESS STATUS_SUCCESS
#define CORE_ERROR_INVALID_INPUT STATUS_ERROR_INVALID_INPUT
#define CORE_ERROR_MEMORY_ALLOCATION STATUS_ERROR_MEMORY_ALLOCATION
#define CORE_ERROR_FILE_NOT_FOUND STATUS_ERROR_FILE_NOT_FOUND
#define CORE_ERROR_INVALID_FORMAT STATUS_ERROR_INVALID_FORMAT
#define CORE_ERROR_NUMERICAL_INSTABILITY STATUS_ERROR_NUMERICAL_INSTABILITY
#define CORE_ERROR_CONVERGENCE_FAILURE STATUS_ERROR_CONVERGENCE_FAILURE

#endif // CORE_COMMON_H

#endif // COMPATIBILITY_ADAPTER_H
