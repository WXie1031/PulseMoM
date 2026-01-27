/******************************************************************************
 * Unified Error Handling System - Implementation
 ******************************************************************************/

#include "errors_core.h"
#include <string.h>
#include <stdio.h>

// Thread-local error storage (simplified - uses static for now)
static pulsemom_error_info_t g_last_error = {PULSEMOM_SUCCESS, NULL, NULL, 0, NULL};

const char* pulsemom_error_string(pulsemom_error_t error) {
    switch (error) {
        case PULSEMOM_SUCCESS:
            return "Success";
        
        // Input errors
        case PULSEMOM_ERROR_INVALID_ARG:
            return "Invalid argument";
        case PULSEMOM_ERROR_NULL_POINTER:
            return "Null pointer";
        case PULSEMOM_ERROR_INVALID_SIZE:
            return "Invalid size";
        case PULSEMOM_ERROR_INVALID_RANGE:
            return "Invalid range";
        case PULSEMOM_ERROR_INVALID_FORMAT:
            return "Invalid format";
        
        // Memory errors
        case PULSEMOM_ERROR_MEMORY:
            return "Memory error";
        case PULSEMOM_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case PULSEMOM_ERROR_MEMORY_DEALLOCATION:
            return "Memory deallocation failed";
        case PULSEMOM_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        
        // File I/O errors
        case PULSEMOM_ERROR_FILE_IO:
            return "File I/O error";
        case PULSEMOM_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case PULSEMOM_ERROR_FILE_READ:
            return "File read error";
        case PULSEMOM_ERROR_FILE_WRITE:
            return "File write error";
        case PULSEMOM_ERROR_FILE_PERMISSION:
            return "File permission denied";
        
        // Numerical errors
        case PULSEMOM_ERROR_NUMERICAL:
            return "Numerical error";
        case PULSEMOM_ERROR_SINGULAR_MATRIX:
            return "Singular matrix";
        case PULSEMOM_ERROR_DIVISION_BY_ZERO:
            return "Division by zero";
        case PULSEMOM_ERROR_OVERFLOW:
            return "Numerical overflow";
        case PULSEMOM_ERROR_UNDERFLOW:
            return "Numerical underflow";
        case PULSEMOM_ERROR_CONVERGENCE:
            return "Convergence failure";
        
        // Solver errors
        case PULSEMOM_ERROR_SOLVER:
            return "Solver error";
        case PULSEMOM_ERROR_SOLVER_NOT_INITIALIZED:
            return "Solver not initialized";
        case PULSEMOM_ERROR_SOLVER_FAILED:
            return "Solver failed";
        case PULSEMOM_ERROR_MESH_INVALID:
            return "Invalid mesh";
        case PULSEMOM_ERROR_GEOMETRY_INVALID:
            return "Invalid geometry";
        
        // Configuration errors
        case PULSEMOM_ERROR_CONFIG:
            return "Configuration error";
        case PULSEMOM_ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        case PULSEMOM_ERROR_MISSING_CONFIG:
            return "Missing configuration";
        
        // Library errors
        case PULSEMOM_ERROR_LIBRARY:
            return "Library error";
        case PULSEMOM_ERROR_LIBRARY_NOT_FOUND:
            return "Library not found";
        case PULSEMOM_ERROR_LIBRARY_VERSION:
            return "Library version mismatch";
        
        // Internal errors
        case PULSEMOM_ERROR_INTERNAL:
            return "Internal error";
        case PULSEMOM_ERROR_NOT_IMPLEMENTED:
            return "Not implemented";
        case PULSEMOM_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

pulsemom_error_info_t pulsemom_get_last_error(void) {
    return g_last_error;
}

void pulsemom_set_error(pulsemom_error_t error, const char* message,
                       const char* file, int line, const char* function) {
    g_last_error.code = error;
    g_last_error.message = message ? message : pulsemom_error_string(error);
    g_last_error.file = file;
    g_last_error.line = line;
    g_last_error.function = function;
}

void pulsemom_clear_error(void) {
    g_last_error.code = PULSEMOM_SUCCESS;
    g_last_error.message = NULL;
    g_last_error.file = NULL;
    g_last_error.line = 0;
    g_last_error.function = NULL;
}

bool pulsemom_is_success(void) {
    return g_last_error.code == PULSEMOM_SUCCESS;
}
