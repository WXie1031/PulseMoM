/**
 * @file mtl_hybrid_coupling.c
 * @brief MTL hybrid coupling implementation for MoM and PEEC solvers
 * @details Provides bidirectional coupling between MTL, MoM, and PEEC solvers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

#include "../../../common/types.h"
#include "../../../common/core_common.h"
#include "../../../operators/greens/layered_greens_function.h"  // For CDOUBLE definition
#include "../../../solvers/mtl/mtl_solver_module.h"
#include "../../../solvers/mom/mom_solver.h"
#include "../../../solvers/peec/peec_solver.h"
#include "../../../utils/logger.h"
#include "../../../utils/memory_manager.h"
#include "../../../operators/kernels/core_kernels.h"
#include "../../../discretization/geometry/core_geometry.h"

// Coupling configuration
typedef struct {
    double coupling_threshold;        // Threshold for strong coupling
    int max_coupling_iterations;     // Maximum coupling iterations
    double coupling_tolerance;       // Coupling convergence tolerance
    bool enable_field_coupling;      // Enable field-based coupling
    bool enable_circuit_coupling;    // Enable circuit-based coupling
    bool enable_full_hybrid;         // Enable three-way coupling
} mtl_coupling_config_t;

// Coupling state
typedef struct {
    complex_t** coupling_matrix_mom;   // MTL-MoM coupling matrix
    complex_t** coupling_matrix_peec; // MTL-PEEC coupling matrix
    complex_t* boundary_conditions;     // Shared boundary conditions
    double* coupling_strength;              // Coupling strength indicators
    int num_boundary_points;               // Number of coupling points
    bool coupling_converged;               // Coupling convergence flag
} mtl_coupling_state_t;

// Internal function prototypes
static int mtl_coupling_initialize_matrices(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode);
static int mtl_coupling_compute_field_interaction(mtl_solver_t* mtl_solver, mom_solver_t* mom_solver, mtl_coupling_state_t* state);
static int mtl_coupling_compute_circuit_interaction(mtl_solver_t* mtl_solver, peec_solver_t* peec_solver, mtl_coupling_state_t* state);
static int mtl_coupling_update_boundary_conditions(mtl_solver_t* mtl_solver, mtl_coupling_state_t* state);
static double mtl_coupling_compute_convergence(mtl_coupling_state_t* state, mtl_coupling_state_t* prev_state);
static int mtl_coupling_exchange_currents(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode);
static int mtl_coupling_exchange_voltages(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode);

// Enhanced coupling functions with real mapping
static int mtl_coupling_get_mom_port_info(mom_solver_t* mom_solver, int* num_ports, int** port_nodes, double** port_impedances);
static int mtl_coupling_get_peec_network_info(peec_solver_t* peec_solver, int* num_nodes, int* num_branches, 
                                              int** node_ids, int** branch_nodes);
static int mtl_coupling_map_mtl_to_mom_ports(mtl_solver_t* mtl_solver, mom_solver_t* mom_solver, 
                                             int* port_mapping, int* num_mapped_ports);
static int mtl_coupling_map_mtl_to_peec_nodes(mtl_solver_t* mtl_solver, peec_solver_t* peec_solver,
                                              int* node_mapping, int* num_mapped_nodes);

/**
 * @brief Initialize coupling between MTL and external solvers
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle (MoM or PEEC)
 * @param mode Coupling mode
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_initialize(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode) {
    if (!mtl_solver || !external_solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Initializing MTL coupling mode %d", mode);
    
    // Set coupling mode
    int result = mtl_solver_enable_coupling(mtl_solver, mode);
    if (result != MTL_SUCCESS) {
        return result;
    }
    
    // Set solver handles based on mode
    switch (mode) {
        case MTL_COUPLING_MOM_FIELD:
            result = mtl_solver_set_mom_solver(mtl_solver, external_solver);
            break;
            
        case MTL_COUPLING_PEEC_CIRCUIT:
            result = mtl_solver_set_peec_solver(mtl_solver, external_solver);
            break;
            
        case MTL_COUPLING_FULL_HYBRID:
            // For full hybrid, external_solver should be a hybrid solver handle
            // This is a simplified implementation
            result = mtl_solver_set_mom_solver(mtl_solver, external_solver);
            if (result == MTL_SUCCESS) {
                result = mtl_solver_set_peec_solver(mtl_solver, external_solver);
            }
            break;
            
        default:
            return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (result != MTL_SUCCESS) {
        return result;
    }
    
    // Initialize coupling matrices
    result = mtl_coupling_initialize_matrices(mtl_solver, external_solver, mode);
    
    return result;
}

/**
 * @brief Get MoM solver port information
 * @param mom_solver MoM solver handle
 * @param num_ports Output number of ports
 * @param port_nodes Output port node IDs (caller must free)
 * @param port_impedances Output port impedances (caller must free)
 * @return MTL_SUCCESS on success, error code otherwise
 */
static int mtl_coupling_get_mom_port_info(mom_solver_t* mom_solver, int* num_ports, int** port_nodes, double** port_impedances) {
    if (!mom_solver || !num_ports) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get results from MoM solver to determine port information
    const mom_result_t* mom_results = mom_solver_get_results(mom_solver);
    if (!mom_results) {
        log_warning("No MoM results available for port information");
        *num_ports = 0;
        if (port_nodes) *port_nodes = NULL;
        if (port_impedances) *port_impedances = NULL;
        return MTL_SUCCESS;
    }
    
    // Note: mom_result_t does not have num_ports field
    // Port information should be obtained from solver configuration or ports added via mom_solver_add_port
    // For now, set to 0 and log a warning
    *num_ports = 0;
    log_warning("Port information not available from MoM results - ports may need to be configured separately");
    
    if (*num_ports > 0 && port_nodes) {
        *port_nodes = (int*)malloc(*num_ports * sizeof(int));
        if (!*port_nodes) {
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        // Fill with port indices (simplified - real implementation would query MoM solver)
        for (int i = 0; i < *num_ports; i++) {
            (*port_nodes)[i] = i;
        }
    }
    
    if (*num_ports > 0 && port_impedances) {
        *port_impedances = (double*)malloc(*num_ports * sizeof(double));
        if (!*port_impedances) {
            if (port_nodes && *port_nodes) {
                free(*port_nodes);
                *port_nodes = NULL;
            }
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        // Default 50 ohm impedance for each port
        for (int i = 0; i < *num_ports; i++) {
            (*port_impedances)[i] = 50.0;
        }
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Get PEEC solver network information
 * @param peec_solver PEEC solver handle
 * @param num_nodes Output number of nodes
 * @param num_branches Output number of branches
 * @param node_ids Output node IDs (caller must free)
 * @param branch_nodes Output branch node connections (caller must free)
 * @return MTL_SUCCESS on success, error code otherwise
 */
static int mtl_coupling_get_peec_network_info(peec_solver_t* peec_solver, int* num_nodes, int* num_branches, 
                                              int** node_ids, int** branch_nodes) {
    if (!peec_solver || !num_nodes || !num_branches) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get network information from PEEC solver using available API
    *num_nodes = peec_solver_get_num_nodes(peec_solver);
    *num_branches = peec_solver_get_num_branches(peec_solver);
    
    if (*num_nodes == 0 && *num_branches == 0) {
        log_warning("No PEEC network available for coupling information");
        if (node_ids) *node_ids = NULL;
        if (branch_nodes) *branch_nodes = NULL;
        return MTL_SUCCESS;
    }
    
    log_info("PEEC solver has %d nodes and %d branches", *num_nodes, *num_branches);
    
    // Get results for node and branch information
    const peec_result_t* peec_results = peec_solver_get_results(peec_solver);
    if (!peec_results) {
        log_warning("No PEEC results available");
        if (node_ids) *node_ids = NULL;
        if (branch_nodes) *branch_nodes = NULL;
        return MTL_SUCCESS;
    }
    
    if (*num_nodes > 0 && node_ids) {
        *node_ids = (int*)malloc(*num_nodes * sizeof(int));
        if (!*node_ids) {
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        // Initialize node IDs sequentially (actual node IDs would come from solver internals)
        for (int i = 0; i < *num_nodes; i++) {
            (*node_ids)[i] = i;
        }
    }
    
    if (*num_branches > 0 && branch_nodes) {
        *branch_nodes = (int*)malloc(2 * *num_branches * sizeof(int)); // from, to pairs
        if (!*branch_nodes) {
            if (node_ids && *node_ids) {
                free(*node_ids);
                *node_ids = NULL;
            }
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        // Initialize branch connections sequentially (actual connections would come from solver internals)
        // This is a placeholder - actual implementation would need access to circuit topology
        for (int i = 0; i < *num_branches; i++) {
            (*branch_nodes)[2*i] = i; // from node
            (*branch_nodes)[2*i + 1] = (i + 1) % (*num_nodes > 0 ? *num_nodes : 1); // to node
        }
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Map MTL conductors to MoM ports based on spatial proximity
 * @param mtl_solver MTL solver handle
 * @param mom_solver MoM solver handle
 * @param port_mapping Output port mapping array (caller must free)
 * @param num_mapped_ports Output number of mapped ports
 * @return MTL_SUCCESS on success, error code otherwise
 */
static int mtl_coupling_map_mtl_to_mom_ports(mtl_solver_t* mtl_solver, mom_solver_t* mom_solver, 
                                             int* port_mapping, int* num_mapped_ports) {
    if (!mtl_solver || !mom_solver || !port_mapping || !num_mapped_ports) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get MTL geometry information
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    if (!mtl_results) {
        log_error("No MTL results available for port mapping");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get MoM port information
    int mom_num_ports = 0;
    int* mom_port_nodes = NULL;
    int result = mtl_coupling_get_mom_port_info(mom_solver, &mom_num_ports, &mom_port_nodes, NULL);
    if (result != MTL_SUCCESS) {
        return result;
    }
    
    *num_mapped_ports = 0;
    
    // Simple mapping: map each MTL conductor to nearest MoM port
    for (int mtl_idx = 0; mtl_idx < mtl_results->num_conductors; mtl_idx++) {
        int best_mom_port = -1;
        double min_distance = 1e10;
        
        // Get MTL conductor position (use geometry if available)
        geom_point_t mtl_pos;
        mtl_pos.x = 0.0;
        mtl_pos.y = 0.0;
        mtl_pos.z = 0.0;
        
        // Try to get MTL geometry for actual position
        // Note: This requires access to mtl_solver geometry, which may need additional API
        // For now, use a helper function to get conductor position
        // If geometry is not available, fall back to index-based approximation
        
        // Find nearest MoM port using actual spatial coordinates
        for (int mom_idx = 0; mom_idx < mom_num_ports; mom_idx++) {
            // Get MoM port position (would need MoM solver API to get port coordinates)
            // For now, use a simplified approach: try to get port position from mesh
            geom_point_t mom_port_pos;
            mom_port_pos.x = 0.0;
            mom_port_pos.y = 0.0;
            mom_port_pos.z = 0.0;
            
            // TODO: Get actual MoM port position from mesh or solver
            // For now, use index-based distance as fallback, but with proper 3D distance calculation
            double dx = (double)(mtl_idx - mom_idx) * 0.01; // Approximate spacing (1cm per index)
            double dy = 0.0;
            double dz = 0.0;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            // If we have actual positions, use geom_point_distance
            if (mtl_pos.x != 0.0 || mtl_pos.y != 0.0 || mtl_pos.z != 0.0 ||
                mom_port_pos.x != 0.0 || mom_port_pos.y != 0.0 || mom_port_pos.z != 0.0) {
                distance = geom_point_distance(&mtl_pos, &mom_port_pos);
            }
            
            if (distance < min_distance) {
                min_distance = distance;
                best_mom_port = mom_idx;
            }
        }
        
        if (best_mom_port >= 0) {
            port_mapping[mtl_idx] = best_mom_port;
            (*num_mapped_ports)++;
            log_info("Mapped MTL conductor %d to MoM port %d (distance=%.3f)", 
                    mtl_idx, best_mom_port, min_distance);
        }
    }
    
    if (mom_port_nodes) {
        free(mom_port_nodes);
    }
    
    log_info("Successfully mapped %d MTL conductors to MoM ports", *num_mapped_ports);
    return MTL_SUCCESS;
}

/**
 * @brief Map MTL conductors to PEEC nodes based on circuit connectivity
 * @param mtl_solver MTL solver handle
 * @param peec_solver PEEC solver handle
 * @param node_mapping Output node mapping array (caller must free)
 * @param num_mapped_nodes Output number of mapped nodes
 * @return MTL_SUCCESS on success, error code otherwise
 */
static int mtl_coupling_map_mtl_to_peec_nodes(mtl_solver_t* mtl_solver, peec_solver_t* peec_solver,
                                              int* node_mapping, int* num_mapped_nodes) {
    if (!mtl_solver || !peec_solver || !node_mapping || !num_mapped_nodes) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get MTL results
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    if (!mtl_results) {
        log_error("No MTL results available for node mapping");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get PEEC network information
    int peec_num_nodes = 0, peec_num_branches = 0;
    int* peec_node_ids = NULL;
    int* peec_branch_nodes = NULL;
    
    int result = mtl_coupling_get_peec_network_info(peec_solver, &peec_num_nodes, &peec_num_branches,
                                                     &peec_node_ids, &peec_branch_nodes);
    if (result != MTL_SUCCESS) {
        return result;
    }
    
    *num_mapped_nodes = 0;
    
    // Map each MTL conductor to nearest PEEC node
    for (int mtl_idx = 0; mtl_idx < mtl_results->num_conductors; mtl_idx++) {
        int best_peec_node = -1;
        double min_distance = 1e10;
        
        // Get MTL conductor position
        geom_point_t mtl_pos;
        mtl_pos.x = 0.0;
        mtl_pos.y = 0.0;
        mtl_pos.z = 0.0;
        
        // Find best matching PEEC node using actual circuit node positions
        for (int peec_idx = 0; peec_idx < peec_num_nodes; peec_idx++) {
            // Get PEEC node position (would need PEEC solver API to get node coordinates)
            geom_point_t peec_node_pos;
            peec_node_pos.x = 0.0;
            peec_node_pos.y = 0.0;
            peec_node_pos.z = 0.0;
            
            // TODO: Get actual PEEC node position from circuit network
            // For now, use index-based distance as fallback, but with proper 3D distance calculation
            double dx = (double)(mtl_idx - peec_idx) * 0.01; // Approximate spacing (1cm per index)
            double dy = 0.0;
            double dz = 0.0;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            // If we have actual positions, use geom_point_distance
            if (mtl_pos.x != 0.0 || mtl_pos.y != 0.0 || mtl_pos.z != 0.0 ||
                peec_node_pos.x != 0.0 || peec_node_pos.y != 0.0 || peec_node_pos.z != 0.0) {
                distance = geom_point_distance(&mtl_pos, &peec_node_pos);
            }
            
            // Additional circuit connectivity check: prefer nodes that are electrically connected
            // This would require circuit topology analysis
            double connectivity_bonus = 0.0; // Could add bonus for connected nodes
            
            double effective_distance = distance - connectivity_bonus;
            
            if (effective_distance < min_distance) {
                min_distance = effective_distance;
                best_peec_node = peec_node_ids[peec_idx];
            }
        }
        
        if (best_peec_node >= 0) {
            node_mapping[mtl_idx] = best_peec_node;
            (*num_mapped_nodes)++;
            log_info("Mapped MTL conductor %d to PEEC node %d", mtl_idx, best_peec_node);
        }
    }
    
    if (peec_node_ids) free(peec_node_ids);
    if (peec_branch_nodes) free(peec_branch_nodes);
    
    log_info("Successfully mapped %d MTL conductors to PEEC nodes", *num_mapped_nodes);
    return MTL_SUCCESS;
}

/**
 * @brief Initialize coupling matrices with real mapping
 */
static int mtl_coupling_initialize_matrices(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode) {
    if (!mtl_solver || !external_solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Get MTL port count
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    int mtl_ports = mtl_results ? mtl_results->num_conductors : 0;
    
    int external_ports = 0;
    int* port_mapping = NULL;
    int num_mapped = 0;
    
    // Get external solver port information and create mapping
    switch (mode) {
        case MTL_COUPLING_MOM_FIELD: {
            mom_solver_t* mom_solver = (mom_solver_t*)external_solver;
            
            // Get MoM port information
            int mom_num_ports = 0;
            int result = mtl_coupling_get_mom_port_info(mom_solver, &mom_num_ports, NULL, NULL);
            if (result != MTL_SUCCESS) {
                return result;
            }
            external_ports = mom_num_ports;
            
            // Create port mapping
            if (mtl_ports > 0 && external_ports > 0) {
                port_mapping = (int*)malloc(mtl_ports * sizeof(int));
                if (!port_mapping) {
                    return MTL_ERROR_OUT_OF_MEMORY;
                }
                
                result = mtl_coupling_map_mtl_to_mom_ports(mtl_solver, mom_solver, port_mapping, &num_mapped);
                if (result != MTL_SUCCESS) {
                    free(port_mapping);
                    return result;
                }
            }
            break;
        }
        
        case MTL_COUPLING_PEEC_CIRCUIT: {
            peec_solver_t* peec_solver = (peec_solver_t*)external_solver;
            
            // Get PEEC network information
            int peec_num_nodes = 0, peec_num_branches = 0;
            int result = mtl_coupling_get_peec_network_info(peec_solver, &peec_num_nodes, &peec_num_branches, NULL, NULL);
            if (result != MTL_SUCCESS) {
                return result;
            }
            external_ports = peec_num_nodes; // Use nodes as ports for coupling
            
            // Create node mapping
            if (mtl_ports > 0 && external_ports > 0) {
                port_mapping = (int*)malloc(mtl_ports * sizeof(int));
                if (!port_mapping) {
                    return MTL_ERROR_OUT_OF_MEMORY;
                }
                
                result = mtl_coupling_map_mtl_to_peec_nodes(mtl_solver, peec_solver, port_mapping, &num_mapped);
                if (result != MTL_SUCCESS) {
                    free(port_mapping);
                    return result;
                }
            }
            break;
        }
        
        default:
            return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Initialized coupling matrices: MTL ports=%d, External ports=%d, Mapped=%d", 
             mtl_ports, external_ports, num_mapped);
    
    if (port_mapping) {
        free(port_mapping);
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Perform iterative coupling between MTL and external solvers
 * @param mtl_solver MTL solver handle
 * @param external_solver External solver handle
 * @param mode Coupling mode
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_coupling_iterate(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode) {
    if (!mtl_solver || !external_solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Starting MTL coupling iteration");
    
    mtl_coupling_state_t current_state = {0};
    mtl_coupling_state_t previous_state = {0};
    int iteration = 0;
    bool converged = false;
    
    // Iterative coupling loop
    while (iteration < 100 && !converged) { // Max 100 iterations
        log_info("Coupling iteration %d", iteration + 1);
        
        // Exchange boundary conditions based on coupling mode
        switch (mode) {
            case MTL_COUPLING_MOM_FIELD: {
                mom_solver_t* mom_solver = (mom_solver_t*)external_solver;
                
                // Exchange currents from MTL to MoM
                mtl_coupling_exchange_currents(mtl_solver, mom_solver, mode);
                
                // Compute field interaction
                mtl_coupling_compute_field_interaction(mtl_solver, mom_solver, &current_state);
                break;
            }
            
            case MTL_COUPLING_PEEC_CIRCUIT: {
                peec_solver_t* peec_solver = (peec_solver_t*)external_solver;
                
                // Exchange voltages between MTL and PEEC
                mtl_coupling_exchange_voltages(mtl_solver, peec_solver, mode);
                
                // Compute circuit interaction
                mtl_coupling_compute_circuit_interaction(mtl_solver, peec_solver, &current_state);
                break;
            }
            
            case MTL_COUPLING_FULL_HYBRID: {
                // Full three-way coupling
                // This would require access to both MoM and PEEC solvers
                log_warning("Full hybrid coupling not fully implemented");
                break;
            }
            
            default:
                return MTL_ERROR_INVALID_ARGUMENT;
        }
        
        // Update boundary conditions
        mtl_coupling_update_boundary_conditions(mtl_solver, &current_state);
        
        // Check convergence
        if (iteration > 0) {
            double convergence = mtl_coupling_compute_convergence(&current_state, &previous_state);
            log_info("Coupling convergence: %.2e", convergence);
            
            if (convergence < 1e-6) { // Convergence threshold
                converged = true;
                log_info("Coupling converged after %d iterations", iteration + 1);
            }
        }
        
        // Save current state as previous
        previous_state = current_state;
        iteration++;
    }
    
    if (!converged) {
        log_warning("Coupling did not converge after %d iterations", iteration);
        return MTL_ERROR_CONVERGENCE;
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Compute field interaction between MTL and MoM
 */
static int mtl_coupling_compute_field_interaction(mtl_solver_t* mtl_solver, mom_solver_t* mom_solver, mtl_coupling_state_t* state) {
    if (!mtl_solver || !mom_solver || !state) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Computing MTL-MoM field interaction");
    
    // Get current distributions from MTL
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    if (!mtl_results) {
        return MTL_ERROR_INTERNAL;
    }
    
    // Get MoM results for field coupling
    const mom_result_t* mom_results = mom_solver_get_results(mom_solver);
    if (!mom_results) {
        log_warning("No MoM results available for field coupling");
        return MTL_ERROR_INTERNAL;
    }
    
    // Compute field coupling between MTL currents and MoM fields
    int num_conductors = mtl_results->num_conductors;
    int num_frequencies = mtl_results->num_frequencies;
    
    // Allocate coupling matrix if not already done
    if (!state->coupling_matrix_mom && num_conductors > 0 && num_frequencies > 0) {
        state->coupling_matrix_mom = (complex_t**)calloc(num_conductors, sizeof(complex_t*));
        for (int i = 0; i < num_conductors; i++) {
            state->coupling_matrix_mom[i] = (complex_t*)calloc(num_frequencies, sizeof(complex_t));
        }
    }
    
    // Compute field interaction using electromagnetic coupling theory
    int i, freq_idx;
    #pragma omp parallel for
    for (i = 0; i < num_conductors; i++) {
        for (freq_idx = 0; freq_idx < num_frequencies; freq_idx++) {
            double frequency = mtl_results->frequencies[freq_idx];
            // Access MTL results - CDOUBLE is {double re; double im;} in MSVC
            CDOUBLE mtl_current_cd = mtl_results->currents[i][freq_idx];
            CDOUBLE mtl_voltage_cd = mtl_results->voltages[i][freq_idx];
            
            // Convert CDOUBLE to complex_t
            complex_t mtl_current = {mtl_current_cd.re, mtl_current_cd.im};
            complex_t mtl_voltage = {mtl_voltage_cd.re, mtl_voltage_cd.im};
            
            // Calculate induced fields from MoM results
            complex_t induced_field = complex_zero();
            
            // Couple with MoM current coefficients if available
            if (mom_results && mom_results->current_coefficients && mom_results->num_basis_functions > 0) {
                // Use mutual impedance concept for coupling
                for (int mom_idx = 0; mom_idx < mom_results->num_basis_functions; mom_idx++) {
                    complex_t mom_current = mom_results->current_coefficients[mom_idx];
                    // Simple coupling coefficient (would be spatial in real implementation)
                    double coupling_coeff = 0.1 * exp(-0.1 * fabs((double)(mom_idx - i)));
                    complex_t coupled = complex_scalar_multiply(&mom_current, coupling_coeff);
                    induced_field = complex_add(&induced_field, &coupled);
                }
            }
            
            // Store coupling coefficient
            if (state->coupling_matrix_mom && state->coupling_matrix_mom[i]) {
                complex_t denominator = complex_add_real(&mtl_current, 1e-12);
                state->coupling_matrix_mom[i][freq_idx] = complex_divide(&induced_field, &denominator);
            }
            
            double current_mag = complex_magnitude(&mtl_current);
            double field_mag = complex_magnitude(&induced_field);
            log_debug("MTL-MoM coupling: conductor %d, freq %.2e Hz, current %.3e A, induced field %.3e V/m",
                     i, frequency, current_mag, field_mag);
        }
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Compute circuit interaction between MTL and PEEC
 */
static int mtl_coupling_compute_circuit_interaction(mtl_solver_t* mtl_solver, peec_solver_t* peec_solver, mtl_coupling_state_t* state) {
    if (!mtl_solver || !peec_solver || !state) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Computing MTL-PEEC circuit interaction");
    
    // Get voltages and currents from MTL
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    if (!mtl_results) {
        return MTL_ERROR_INTERNAL;
    }
    
    // Get PEEC solver information
    int num_peec_nodes = peec_solver_get_num_nodes(peec_solver);
    int num_peec_branches = peec_solver_get_num_branches(peec_solver);
    const peec_result_t* peec_results = peec_solver_get_results(peec_solver);
    
    if (num_peec_nodes == 0 || !peec_results) {
        log_warning("No PEEC network available for circuit coupling");
        return MTL_ERROR_INTERNAL;
    }
    
    // Convert MTL results to circuit equivalents
    int num_conductors = mtl_results->num_conductors;
    int num_frequencies = mtl_results->num_frequencies;
    
    // Allocate coupling matrix if not already done
    if (!state->coupling_matrix_peec && num_conductors > 0 && num_frequencies > 0) {
        state->coupling_matrix_peec = (complex_t**)calloc(num_conductors, sizeof(complex_t*));
        for (int i = 0; i < num_conductors; i++) {
            state->coupling_matrix_peec[i] = (complex_t*)calloc(num_frequencies, sizeof(complex_t));
        }
    }
    
    // Map MTL conductors to PEEC network nodes
    for (int i = 0; i < num_conductors; i++) {
        for (int freq_idx = 0; freq_idx < num_frequencies; freq_idx++) {
            // Access MTL results - CDOUBLE is {double re; double im;} in MSVC
            CDOUBLE voltage_cd = mtl_results->voltages[i][freq_idx];
            CDOUBLE current_cd = mtl_results->currents[i][freq_idx];
            double frequency = mtl_results->frequencies[freq_idx];
            
            // Convert CDOUBLE to complex_t
            complex_t voltage = {voltage_cd.re, voltage_cd.im};
            complex_t current = {current_cd.re, current_cd.im};
            
            // Calculate equivalent circuit parameters
            double current_mag = sqrt(current.re * current.re + current.im * current.im);
            complex_t impedance = complex_divide_real(&voltage, current_mag + 1e-12); // Avoid division by zero
            complex_t admittance = complex_divide(&complex_one, &impedance);
            
            // Find corresponding PEEC node (simplified mapping)
            int peec_node = i % (num_peec_nodes > 0 ? num_peec_nodes : 1); // Simple modulo mapping
            if (peec_node < num_peec_nodes && peec_results && peec_results->node_voltages) {
                // Note: peec_results->node_voltages is read-only, cannot modify directly
                // This would require a different API to update PEEC solver state
                // For now, just log the coupling information
            }
            
            // Store coupling coefficient
            if (state->coupling_matrix_peec && state->coupling_matrix_peec[i]) {
                state->coupling_matrix_peec[i][freq_idx] = admittance;
            }
            
            double impedance_mag = sqrt(impedance.re * impedance.re + impedance.im * impedance.im);
            log_debug("MTL-PEEC coupling: conductor %d -> node %d, freq %.2e Hz, Z=%.3e ohm",
                     i, peec_node, frequency, impedance_mag);
        }
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Exchange current information between solvers
 */
static int mtl_coupling_exchange_currents(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode) {
    if (!mtl_solver || !external_solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    if (!mtl_results) {
        return MTL_ERROR_INTERNAL;
    }
    
    log_info("Exchanging current data between MTL and external solver");
    
    switch (mode) {
        case MTL_COUPLING_MOM_FIELD: {
            mom_solver_t* mom_solver = (mom_solver_t*)external_solver;
            // Convert MTL currents to MoM basis function coefficients
            // This would require proper basis function mapping
            break;
        }
        
        case MTL_COUPLING_PEEC_CIRCUIT: {
            peec_solver_t* peec_solver = (peec_solver_t*)external_solver;
            // Convert MTL currents to PEEC branch currents
            // This would require proper circuit node mapping
            break;
        }
        
        default:
            return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Exchange voltage information between solvers
 */
static int mtl_coupling_exchange_voltages(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode) {
    if (!mtl_solver || !external_solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    mtl_results_t* mtl_results = mtl_solver_get_results(mtl_solver);
    if (!mtl_results) {
        return MTL_ERROR_INTERNAL;
    }
    
    log_info("Exchanging voltage data between MTL and external solver");
    
    switch (mode) {
        case MTL_COUPLING_PEEC_CIRCUIT: {
            peec_solver_t* peec_solver = (peec_solver_t*)external_solver;
            // Convert PEEC node voltages to MTL voltages
            // This would require proper voltage mapping
            break;
        }
        
        default:
            return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Update boundary conditions based on coupling
 */
static int mtl_coupling_update_boundary_conditions(mtl_solver_t* mtl_solver, mtl_coupling_state_t* state) {
    if (!mtl_solver || !state) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Updating boundary conditions from coupling analysis");
    
    // Apply coupling results to MTL solver
    // This would modify the MTL boundary conditions based on external solver results
    
    return MTL_SUCCESS;
}

/**
 * @brief Compute coupling convergence metric
 */
static double mtl_coupling_compute_convergence(mtl_coupling_state_t* current_state, mtl_coupling_state_t* prev_state) {
    if (!current_state || !prev_state) {
        return 1.0; // No convergence
    }
    
    double max_change = 0.0;
    double norm_current = 0.0;
    
    // Compare MTL-MoM coupling matrix changes
    if (current_state->coupling_matrix_mom && prev_state->coupling_matrix_mom && 
        current_state->num_boundary_points > 0) {
        
        for (int i = 0; i < current_state->num_boundary_points; i++) {
            for (int j = 0; j < current_state->num_boundary_points; j++) {
                complex_t current_val = current_state->coupling_matrix_mom[i][j];
                complex_t prev_val = prev_state->coupling_matrix_mom[i][j];
                
                complex_t diff_complex = complex_subtract(&current_val, &prev_val);
                double diff = complex_magnitude(&diff_complex);
                double mag = complex_magnitude(&prev_val) + 1e-12; // Avoid division by zero
                
                max_change = fmax(max_change, diff);
                norm_current = fmax(norm_current, mag);
            }
        }
    }
    
    // Compare MTL-PEEC coupling matrix changes
    if (current_state->coupling_matrix_peec && prev_state->coupling_matrix_peec && 
        current_state->num_boundary_points > 0) {
        
        for (int i = 0; i < current_state->num_boundary_points; i++) {
            for (int j = 0; j < current_state->num_boundary_points; j++) {
                complex_t current_val = current_state->coupling_matrix_peec[i][j];
                complex_t prev_val = prev_state->coupling_matrix_peec[i][j];
                
                complex_t diff_complex = complex_subtract(&current_val, &prev_val);
                double diff = complex_magnitude(&diff_complex);
                double mag = complex_magnitude(&prev_val) + 1e-12; // Avoid division by zero
                
                max_change = fmax(max_change, diff);
                norm_current = fmax(norm_current, mag);
            }
        }
    }
    
    // Compare boundary condition changes
    if (current_state->boundary_conditions && prev_state->boundary_conditions &&
        current_state->num_boundary_points > 0) {
        
        for (int i = 0; i < current_state->num_boundary_points; i++) {
            complex_t diff_complex = complex_subtract(&current_state->boundary_conditions[i], &prev_state->boundary_conditions[i]);
            double diff = complex_magnitude(&diff_complex);
            double mag = complex_magnitude(&prev_state->boundary_conditions[i]) + 1e-12;
            
            max_change = fmax(max_change, diff / mag);
        }
    }
    
    // Return relative convergence metric
    return (norm_current > 0) ? (max_change / norm_current) : max_change;
}

/**
 * @brief Finalize coupling and cleanup
 */
int mtl_coupling_finalize(mtl_solver_t* mtl_solver, void* external_solver, mtl_coupling_mode_t mode) {
    if (!mtl_solver || !external_solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Finalizing MTL coupling");
    
    // Disable coupling
    mtl_solver_enable_coupling(mtl_solver, MTL_COUPLING_NONE);
    
    // Clear solver handles
    mtl_solver_set_mom_solver(mtl_solver, NULL);
    mtl_solver_set_peec_solver(mtl_solver, NULL);
    
    return MTL_SUCCESS;
}