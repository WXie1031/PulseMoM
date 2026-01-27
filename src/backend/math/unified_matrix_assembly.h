/*****************************************************************************************
 * Unified MoM+PEEC Matrix Assembly Architecture
 * 
 * Implements the hybrid matrix structure for commercial-grade solvers:
 * | Z_mom   Z_mp |
 * | Z_pm   Z_pp  |
 * 
 * Where:
 * - Z_mom: Method of Moments blocks (surface integrals)
 * - Z_pp: PEEC blocks (R + jωL + 1/(jω)C)
 * - Z_mp/Z_pm: Coupling between MoM and PEEC regions
 *****************************************************************************************/

#ifndef UNIFIED_MATRIX_ASSEMBLY_H
#define UNIFIED_MATRIX_ASSEMBLY_H

#include <complex.h>
#include <stddef.h>
#include "math_backend_selector.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"

// Matrix block types for unified assembly
typedef enum {
    BLOCK_TYPE_MOM_MOM,         // MoM-MoM interaction
    BLOCK_TYPE_MOM_PEEC,        // MoM-PEEC coupling
    BLOCK_TYPE_PEEC_MOM,        // PEEC-MoM coupling  
    BLOCK_TYPE_PEEC_PEEC,       // PEEC-PEEC interaction
    BLOCK_TYPE_IDENTITY,        // Identity matrix
    BLOCK_TYPE_PRECONDITIONER   // Preconditioner block
} block_type_t;

// Matrix block descriptor
typedef struct {
    block_type_t type;          // Block type
    size_t row_start, row_end; // Row range
    size_t col_start, col_end; // Column range
    matrix_handle_t* matrix;    // Backend matrix handle
    int is_compressed;          // H-matrix compression flag
    double compression_ratio;   // Compression ratio (if compressed)
} matrix_block_t;

// Unified matrix assembly
typedef struct {
    matrix_block_t* blocks;     // Array of matrix blocks
    size_t num_blocks;          // Number of blocks
    size_t total_rows;          // Total matrix rows
    size_t total_cols;          // Total matrix columns
    math_backend_t backend;     // Numerical backend
    matrix_format_t format;     // Overall matrix format
    int use_hmatrix;           // Enable H-matrix compression
    double hmatrix_epsilon;     // H-matrix compression tolerance
} unified_matrix_t;

// Region partitioning for MoM/PEEC
typedef enum {
    REGION_TYPE_MOM,            // Method of Moments region
    REGION_TYPE_PEEC,           // PEEC region
    REGION_TYPE_COUPLING        // Coupling interface
} region_type_t;

// Simulation region descriptor
typedef struct {
    region_type_t type;         // Region type
    size_t region_id;          // Unique region identifier
    mesh_t* mesh;              // Associated mesh
    geometry_t* geometry;      // Associated geometry
    size_t num_dofs;           // Degrees of freedom
    size_t global_dof_offset;  // Global DOF offset
    int use_adaptive_mesh;     // Adaptive mesh refinement flag
    double frequency;          // Operating frequency
} simulation_region_t;

// Unified assembly parameters
typedef struct {
    // Physical parameters
    double frequency;           // Operating frequency
    double omega;              // Angular frequency (2πf)
    
    // Numerical parameters
    math_backend_t backend;     // Numerical backend
    matrix_format_t format;     // Matrix format
    int use_hmatrix;           // Enable H-matrix compression
    double hmatrix_epsilon;     // Compression tolerance
    int use_preconditioner;    // Enable preconditioning
    
    // Parallel parameters
    int num_threads;           // CPU threads
    int use_gpu;              // GPU acceleration
    int gpu_device_id;        // GPU device ID
    
    // Accuracy parameters
    double integration_tolerance; // Integration accuracy
    int max_integration_order;  // Maximum integration order
    double singularity_tolerance; // Singularity handling
    
    // Memory parameters
    size_t max_memory_mb;      // Maximum memory usage
    int use_memory_pool;      // Memory pooling
} assembly_params_t;

// Core assembly functions
unified_matrix_t* unified_matrix_create(assembly_params_t* params);
void unified_matrix_destroy(unified_matrix_t* matrix);

// Block assembly
int unified_matrix_add_block(unified_matrix_t* matrix, matrix_block_t* block);
int unified_matrix_assemble_blocks(unified_matrix_t* matrix);

// Region-based assembly
int unified_matrix_assemble_region_pair(unified_matrix_t* matrix,
                                       simulation_region_t* region1,
                                       simulation_region_t* region2,
                                       assembly_params_t* params);

// MoM-specific assembly
int unified_matrix_assemble_mom_block(unified_matrix_t* matrix,
                                     simulation_region_t* region1,
                                     simulation_region_t* region2,
                                     assembly_params_t* params);

// PEEC-specific assembly
int unified_matrix_assemble_peec_block(unified_matrix_t* matrix,
                                        simulation_region_t* region1,
                                        simulation_region_t* region2,
                                        assembly_params_t* params);

// Coupling assembly
int unified_matrix_assemble_coupling_block(unified_matrix_t* matrix,
                                            simulation_region_t* mom_region,
                                            simulation_region_t* peec_region,
                                            assembly_params_t* params);

// Matrix-vector operations
int unified_matrix_vector_multiply(const unified_matrix_t* matrix,
                                  const vector_handle_t* x,
                                  vector_handle_t* y,
                                  double complex alpha,
                                  double complex beta);

// Solver interface
int unified_matrix_solve(const unified_matrix_t* matrix,
                        const vector_handle_t* b,
                        vector_handle_t* x,
                        solver_params_t* solver_params);

// Preconditioner support
int unified_matrix_build_preconditioner(unified_matrix_t* matrix);
int unified_matrix_apply_preconditioner(const unified_matrix_t* matrix,
                                       const vector_handle_t* r,
                                       vector_handle_t* z);

// H-matrix compression
int unified_matrix_compress(unified_matrix_t* matrix, double epsilon);
int unified_matrix_decompress(unified_matrix_t* matrix);

// Memory management
size_t unified_matrix_get_memory_usage(const unified_matrix_t* matrix);
int unified_matrix_optimize_memory(unified_matrix_t* matrix);

// Performance profiling
typedef struct {
    double assembly_time;       // Matrix assembly time
    double compression_time;    // H-matrix compression time
    double solve_time;         // Linear solve time
    double memory_usage_mb;    // Memory usage
    double compression_ratio;  // Compression ratio
    int num_blocks;           // Number of blocks
    int num_compressed_blocks; // Number of compressed blocks
} assembly_stats_t;

int unified_matrix_get_stats(const unified_matrix_t* matrix, assembly_stats_t* stats);

// Industrial validation
int unified_matrix_validate_structure(const unified_matrix_t* matrix);
int unified_matrix_check_symmetry(const unified_matrix_t* matrix, double tolerance);
int unified_matrix_check_positive_definite(const unified_matrix_t* matrix);

// Export/Import for commercial formats
int unified_matrix_export_matrix_market(const unified_matrix_t* matrix, const char* filename);
int unified_matrix_export_harwell_boeing(const unified_matrix_t* matrix, const char* filename);
int unified_matrix_import_matrix_market(unified_matrix_t* matrix, const char* filename);

// Block matrix operations for advanced solvers
matrix_block_t* unified_matrix_get_block(unified_matrix_t* matrix, size_t row, size_t col);
int unified_matrix_set_block(unified_matrix_t* matrix, size_t row, size_t col, matrix_block_t* block);

// Adaptive refinement support
int unified_matrix_adapt_refinement(unified_matrix_t* matrix,
                                   simulation_region_t* region,
                                   double error_threshold);

// Frequency sweep optimization
typedef struct {
    double frequency_start;
    double frequency_end;
    int num_frequencies;
    int use_interpolation;
    double interpolation_tolerance;
} frequency_sweep_params_t;

int unified_matrix_frequency_sweep(unified_matrix_t* matrix,
                                   frequency_sweep_params_t* params,
                                   vector_handle_t** solutions);

#endif // UNIFIED_MATRIX_ASSEMBLY_H