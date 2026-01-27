# Error Codes and Status Codes

## Overview

The PulseMoM system uses a unified error code system defined in `src/common/types.h` as the `status_t` enum.

## Status Codes

All status codes are defined in `src/common/types.h`:

```c
typedef enum {
    STATUS_SUCCESS = 0,                        // Operation succeeded
    STATUS_ERROR_INVALID_INPUT = -1,           // Invalid input parameters
    STATUS_ERROR_MEMORY_ALLOCATION = -2,       // Memory allocation failed
    STATUS_ERROR_FILE_NOT_FOUND = -3,          // File not found
    STATUS_ERROR_INVALID_FORMAT = -4,          // Invalid file or data format
    STATUS_ERROR_NUMERICAL_INSTABILITY = -5,    // Numerical instability detected
    STATUS_ERROR_CONVERGENCE_FAILURE = -6,     // Iterative solver convergence failure
    STATUS_ERROR_NOT_IMPLEMENTED = -7,          // Feature not yet implemented
    STATUS_ERROR_INVALID_STATE = -8             // Invalid object/state (e.g., operation called before initialization)
} status_t;
```

## Usage Guidelines

### When to Use Each Error Code

1. **STATUS_SUCCESS (0)**: Return when operation completes successfully.

2. **STATUS_ERROR_INVALID_INPUT (-1)**: Use when:
   - Function receives NULL pointer when non-NULL is required
   - Parameter values are out of valid range
   - Required parameters are missing
   - Data structures are malformed

3. **STATUS_ERROR_MEMORY_ALLOCATION (-2)**: Use when:
   - `malloc()`, `calloc()`, or `realloc()` fails
   - Memory pool allocation fails
   - Out of memory conditions

4. **STATUS_ERROR_FILE_NOT_FOUND (-3)**: Use when:
   - File path does not exist
   - File cannot be opened for reading

5. **STATUS_ERROR_INVALID_FORMAT (-4)**: Use when:
   - File format is incorrect or unsupported
   - Data parsing fails due to format issues
   - STL/CAD file format errors

6. **STATUS_ERROR_NUMERICAL_INSTABILITY (-5)**: Use when:
   - Matrix is singular or near-singular
   - Division by zero or near-zero
   - Numerical overflow/underflow
   - Condition number exceeds threshold
   - Zero pivot detected in LU/Cholesky decomposition
   - Diagonal element is zero or near-zero in forward/backward substitution
   
   **Note**: Do NOT use `STATUS_ERROR_NUMERICAL` (not defined). Always use `STATUS_ERROR_NUMERICAL_INSTABILITY`.

7. **STATUS_ERROR_CONVERGENCE_FAILURE (-6)**: Use when:
   - Iterative solver fails to converge
   - Maximum iterations reached without convergence
   - Residual does not decrease
   - Iteration process diverges
   
   **Note**: Do NOT use `STATUS_ERROR_NOT_CONVERGED` (not defined). Always use `STATUS_ERROR_CONVERGENCE_FAILURE`.

8. **STATUS_ERROR_NOT_IMPLEMENTED (-7)**: Use when:
   - Function is a stub/placeholder
   - Feature is planned but not yet implemented
   - Backend-specific feature not available

9. **STATUS_ERROR_INVALID_STATE (-8)**: Use when:
   - Operation called before required initialization
   - Object is in wrong state for requested operation
   - Solver called before matrix/rhs is set
   - Results requested before solve completes
   - Geometry accessed before loading

## Error Code Location

- **Definition**: `src/common/types.h` - `status_t` enum
- **Legacy Compatibility**: `src/common/errors.h` - `CORE_ERROR_*` macros

## Backward Compatibility

The `src/common/errors.h` file provides legacy `CORE_ERROR_*` macros for backward compatibility:

```c
#define CORE_SUCCESS STATUS_SUCCESS
#define CORE_ERROR_INVALID_INPUT STATUS_ERROR_INVALID_INPUT
#define CORE_ERROR_MEMORY_ALLOCATION STATUS_ERROR_MEMORY_ALLOCATION
#define CORE_ERROR_FILE_NOT_FOUND STATUS_ERROR_FILE_NOT_FOUND
#define CORE_ERROR_INVALID_FORMAT STATUS_ERROR_INVALID_FORMAT
#define CORE_ERROR_NUMERICAL_INSTABILITY STATUS_ERROR_NUMERICAL_INSTABILITY
#define CORE_ERROR_CONVERGENCE_FAILURE STATUS_ERROR_CONVERGENCE_FAILURE
#define CORE_ERROR_INVALID_STATE STATUS_ERROR_INVALID_STATE
```

## Examples

### Example 1: Invalid Input
```c
int solver_interface_set_matrix(solver_interface_t* solver, const operator_matrix_t* matrix) {
    if (!solver || !matrix) {
        return STATUS_ERROR_INVALID_INPUT;  // NULL pointer
    }
    // ...
}
```

### Example 2: Invalid State
```c
int solver_interface_get_solution(const solver_interface_t* solver, operator_vector_t* solution) {
    if (!solver || !solution) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!solver->solution) {
        return STATUS_ERROR_INVALID_STATE;  // Solution not available yet
    }
    // ...
}
```

### Example 3: Memory Allocation
```c
operator_matrix_t* matrix = (operator_matrix_t*)calloc(1, sizeof(operator_matrix_t));
if (!matrix) {
    return STATUS_ERROR_MEMORY_ALLOCATION;
}
```

## Architecture Rules

1. **L1-L6 Layers**: All layers use `status_t` for error reporting
2. **Consistency**: Always use `STATUS_*` constants, never magic numbers
3. **Documentation**: Document which error codes each function can return
4. **Error Propagation**: Functions should propagate errors from called functions
5. **Validation**: Check inputs early and return appropriate error codes

## Adding New Error Codes

When adding a new error code:

1. Add to `status_t` enum in `src/common/types.h` with next negative value
2. Add corresponding `CORE_ERROR_*` macro in `src/common/errors.h` for backward compatibility
3. Update this skills document
4. Document usage in function comments

## Related Files

- `src/common/types.h` - Status code definitions
- `src/common/errors.h` - Legacy compatibility macros
- All source files - Use `status_t` for return values
