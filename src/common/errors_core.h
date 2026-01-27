/******************************************************************************
 * Unified Error Handling System
 * 
 * Provides consistent error codes and error reporting across all modules
 ******************************************************************************/

#ifndef CORE_ERRORS_H
#define CORE_ERRORS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Unified Error Codes
 ******************************************************************************/

typedef enum {
    PULSEMOM_SUCCESS = 0,
    
    // Input/Argument errors (1-99)
    PULSEMOM_ERROR_INVALID_ARG = -1,
    PULSEMOM_ERROR_NULL_POINTER = -2,
    PULSEMOM_ERROR_INVALID_SIZE = -3,
    PULSEMOM_ERROR_INVALID_RANGE = -4,
    PULSEMOM_ERROR_INVALID_FORMAT = -5,
    
    // Memory errors (100-199)
    PULSEMOM_ERROR_MEMORY = -100,
    PULSEMOM_ERROR_MEMORY_ALLOCATION = -101,
    PULSEMOM_ERROR_MEMORY_DEALLOCATION = -102,
    PULSEMOM_ERROR_OUT_OF_MEMORY = -103,
    
    // File I/O errors (200-299)
    PULSEMOM_ERROR_FILE_IO = -200,
    PULSEMOM_ERROR_FILE_NOT_FOUND = -201,
    PULSEMOM_ERROR_FILE_READ = -202,
    PULSEMOM_ERROR_FILE_WRITE = -203,
    PULSEMOM_ERROR_FILE_PERMISSION = -204,
    
    // Numerical errors (300-399)
    PULSEMOM_ERROR_NUMERICAL = -300,
    PULSEMOM_ERROR_SINGULAR_MATRIX = -301,
    PULSEMOM_ERROR_DIVISION_BY_ZERO = -302,
    PULSEMOM_ERROR_OVERFLOW = -303,
    PULSEMOM_ERROR_UNDERFLOW = -304,
    PULSEMOM_ERROR_CONVERGENCE = -305,
    
    // Solver errors (400-499)
    PULSEMOM_ERROR_SOLVER = -400,
    PULSEMOM_ERROR_SOLVER_NOT_INITIALIZED = -401,
    PULSEMOM_ERROR_SOLVER_FAILED = -402,
    PULSEMOM_ERROR_MESH_INVALID = -403,
    PULSEMOM_ERROR_GEOMETRY_INVALID = -404,
    
    // Configuration errors (500-599)
    PULSEMOM_ERROR_CONFIG = -500,
    PULSEMOM_ERROR_INVALID_CONFIG = -501,
    PULSEMOM_ERROR_MISSING_CONFIG = -502,
    
    // Library/Dependency errors (600-699)
    PULSEMOM_ERROR_LIBRARY = -600,
    PULSEMOM_ERROR_LIBRARY_NOT_FOUND = -601,
    PULSEMOM_ERROR_LIBRARY_VERSION = -602,
    
    // Internal errors (900-999)
    PULSEMOM_ERROR_INTERNAL = -900,
    PULSEMOM_ERROR_NOT_IMPLEMENTED = -901,
    PULSEMOM_ERROR_UNKNOWN = -999
} pulsemom_error_t;

/******************************************************************************
 * Error Information Structure
 ******************************************************************************/

typedef struct {
    pulsemom_error_t code;
    const char* message;
    const char* file;
    int line;
    const char* function;
} pulsemom_error_info_t;

/******************************************************************************
 * Error Handling Functions
 ******************************************************************************/

/**
 * Get error string from error code
 * 
 * @param error Error code
 * @return Error message string
 */
const char* pulsemom_error_string(pulsemom_error_t error);

/**
 * Get last error information
 * 
 * @return Error information structure
 */
pulsemom_error_info_t pulsemom_get_last_error(void);

/**
 * Set error information
 * 
 * @param error Error code
 * @param message Error message (optional, NULL to use default)
 * @param file Source file name (__FILE__)
 * @param line Line number (__LINE__)
 * @param function Function name (__FUNCTION__)
 */
void pulsemom_set_error(pulsemom_error_t error, const char* message,
                       const char* file, int line, const char* function);

/**
 * Clear error state
 */
void pulsemom_clear_error(void);

/**
 * Check if last operation was successful
 * 
 * @return true if success, false if error
 */
bool pulsemom_is_success(void);

/**
 * Error checking macro
 */
#define PULSEMOM_CHECK(expr) do { \
    pulsemom_error_t _err = (expr); \
    if (_err != PULSEMOM_SUCCESS) { \
        pulsemom_set_error(_err, NULL, __FILE__, __LINE__, __FUNCTION__); \
        return _err; \
    } \
} while(0)

/**
 * Error checking macro with cleanup
 */
#define PULSEMOM_CHECK_CLEANUP(expr, cleanup) do { \
    pulsemom_error_t _err = (expr); \
    if (_err != PULSEMOM_SUCCESS) { \
        cleanup; \
        pulsemom_set_error(_err, NULL, __FILE__, __LINE__, __FUNCTION__); \
        return _err; \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif // CORE_ERRORS_H
