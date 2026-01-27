/**
 * @file mtl_hybrid_coupling.h
 * @brief MTL hybrid coupling interface for MoM and PEEC solvers
 * @details Provides bidirectional coupling between MTL, MoM, and PEEC solvers
 */

#ifndef MTL_HYBRID_COUPLING_H
#define MTL_HYBRID_COUPLING_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../solvers/mtl/mtl_solver_module.h"
#include "../../../solvers/mom/mom_solver.h"
#include "../../../solvers/peec/peec_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Coupling configuration
typedef struct {
    double coupling_threshold;        // Threshold for strong coupling (0.0-1.0)
    int max_coupling_iterations;     // Maximum coupling iterations
    double coupling_tolerance;       // Coupling convergence tolerance
    bool enable_field_coupling;      // Enable field-based coupling
    bool enable_circuit_coupling;    // Enable circuit-based coupling
    bool enable_full_hybrid;         // Enable three-way coupling
    bool enable_adaptive_coupling;   // Enable adaptive coupling refinement
    int coupling_update_frequency;   // How often to update coupling (iterations)
} mtl_coupling_config_t;

// Coupling algorithms
typedef enum {
    MTL_COUPLING_ALGORITHM_DIRECT,      // Direct matrix coupling
    MTL_COUPLING_ALGORITHM_ITERATIVE,  // Iterative subdomain coupling
    MTL_COUPLING_ALGORITHM_SCHUR,       // Schur complement coupling
    MTL_COUPLING_ALGORITHM_LAGRANGE,    // Lagrange multiplier coupling
    MTL_COUPLING_ALGORITHM_MORTAR       // Mortar method coupling
} mtl_coupling_algorithm_t;

// Coupling state information
typedef struct {
    int iteration_count;              // Current coupling iteration
    double convergence_metric;        // Current convergence value
    bool is_converged;               // Convergence status
    double max_coupling_strength;     // Maximum coupling strength
    int num_boundary_points;         // Number of coupling boundary points
    double coupling_update_time;      // Time for last coupling update
} mtl_coupling_status_t;

// Field coupling data
typedef struct {
    double complex* e_field;          // Electric field values
    double complex* h_field;          // Magnetic field values
    double* positions_x;             // Field evaluation points X
    double* positions_y;             // Field evaluation points Y
    double* positions_z;             // Field evaluation points Z
    int num_field_points;            // Number of field points
    double frequency;                // Frequency for field calculation
} mtl_field_coupling_data_t;

// Circuit coupling data
typedef struct {
    double complex* voltages;         // Node voltages
    double complex* currents;           // Branch currents
    double complex* impedances;         // Branch impedances
    int* node_connections;            // Connection node indices
    int num_circuit_nodes;            // Number of circuit nodes
    int num_branches;                 // Number of branches
    double frequency;                 // Frequency for circuit analysis
} mtl_circuit_coupling_data_t;

/**
 * @brief Initialize coupling between MTL and external solvers
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle (MoM or PEEC)
 * @param mode Coupling mode
 * @param config Coupling configuration
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_initialize(mtl_solver_t* mtl_solver, void* external_solver, 
                           mtl_coupling_mode_t mode, const mtl_coupling_config_t* config);

/**
 * @brief Perform iterative coupling between MTL and external solvers
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle
 * @param mode Coupling mode
 * @param algorithm Coupling algorithm to use
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_iterate(mtl_solver_t* mtl_solver, void* external_solver, 
                        mtl_coupling_mode_t mode, mtl_coupling_algorithm_t algorithm);

/**
 * @brief Finalize coupling and cleanup
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle
 * @param mode Coupling mode
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_finalize(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode);

/**
 * @brief Get current coupling status
 * @param mtl_solver MTL solver handle
 * @param status Output status structure
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_get_status(const mtl_solver_t* mtl_solver, mtl_coupling_status_t* status);

/**
 * @brief Compute field coupling between MTL and MoM
 * @param mtl_solver MTL solver handle
 * @param mom_solver MoM solver handle
 * @param field_data Field coupling data
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_compute_field_interaction(mtl_solver_t* mtl_solver, mom_solver_t* mom_solver,
                                          mtl_field_coupling_data_t* field_data);

/**
 * @brief Compute circuit coupling between MTL and PEEC
 * @param mtl_solver MTL solver handle
 * @param peec_solver PEEC solver handle
 * @param circuit_data Circuit coupling data
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_compute_circuit_interaction(mtl_solver_t* mtl_solver, peec_solver_t* peec_solver,
                                            mtl_circuit_coupling_data_t* circuit_data);

/**
 * @brief Update coupling matrices based on current solution
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle
 * @param mode Coupling mode
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_update_matrices(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode);

/**
 * @brief Check coupling convergence
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle
 * @param tolerance Convergence tolerance
 * @return True if converged, false otherwise
 */
bool mtl_coupling_check_convergence(const mtl_solver_t* mtl_solver, void* external_solver, double tolerance);

/**
 * @brief Exchange boundary conditions between solvers
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle
 * @param mode Coupling mode
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_exchange_boundary_conditions(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode);

/**
 * @brief Create field coupling data structure
 * @param num_points Number of field points
 * @return Allocated field coupling data structure
 */
mtl_field_coupling_data_t* mtl_field_coupling_create(int num_points);

/**
 * @brief Destroy field coupling data structure
 * @param data Field coupling data structure
 */
void mtl_field_coupling_destroy(mtl_field_coupling_data_t* data);

/**
 * @brief Create circuit coupling data structure
 * @param num_nodes Number of circuit nodes
 * @param num_branches Number of branches
 * @return Allocated circuit coupling data structure
 */
mtl_circuit_coupling_data_t* mtl_circuit_coupling_create(int num_nodes, int num_branches);

/**
 * @brief Destroy circuit coupling data structure
 * @param data Circuit coupling data structure
 */
void mtl_circuit_coupling_destroy(mtl_circuit_coupling_data_t* data);

/**
 * @brief Set default coupling configuration
 * @param config Configuration structure to initialize
 */
void mtl_coupling_config_default(mtl_coupling_config_t* config);

/**
 * @brief Print coupling information
 * @param mtl_solver MTL solver handle
 */
void mtl_coupling_print_info(const mtl_solver_t* mtl_solver);

#ifdef __cplusplus
}
#endif

#endif /* MTL_HYBRID_COUPLING_H */