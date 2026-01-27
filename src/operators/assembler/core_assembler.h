/**
 * @file core_assembler.h
 * @brief Unified assembler interface for MoM and PEEC solvers
 * @details Matrix assembly and network assembly with shared infrastructure
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_ASSEMBLER_H
#define CORE_ASSEMBLER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/core_common.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../kernels/core_kernels.h"
#include "../../backend/solvers/sparse_direct_solver.h"

typedef enum {
    MATRIX_TYPE_DENSE = 0,
    MATRIX_TYPE_SPARSE = 1,
    MATRIX_TYPE_COMPRESSED = 2
} matrix_type_t;

typedef enum {
    ELEMENT_TYPE_TRIANGLE = 0,
    ELEMENT_TYPE_RECTANGLE = 1,
    ELEMENT_TYPE_WIRE = 2,
    ELEMENT_TYPE_VIA = 3
} element_type_t;

#ifdef __cplusplus
extern "C" {
#endif

// Assembly types
typedef enum {
    ASSEMBLY_TYPE_DENSE,          // Dense matrix (MoM)
    ASSEMBLY_TYPE_SPARSE,         // Sparse matrix (PEEC)
    ASSEMBLY_TYPE_BLOCK_SPARSE,    // Block sparse (Hybrid)
    ASSEMBLY_TYPE_NETWORK,         // Circuit network (PEEC)
    ASSEMBLY_TYPE_COMPRESSED        // Compressed representation (ACA)
} assembly_type_t;

typedef enum {
    ASSEMBLY_FORMULATION_EFIE,     // Electric Field Integral Equation
    ASSEMBLY_FORMULATION_MFIE,     // Magnetic Field Integral Equation
    ASSEMBLY_FORMULATION_CFIE,     // Combined Field Integral Equation
    ASSEMBLY_FORMULATION_POTENTIAL, // Scalar/Vector Potential
    ASSEMBLY_FORMULATION_CIRCUIT   // Circuit network
} assembly_formulation_t;

// Matrix assembly specification
typedef struct {
    assembly_type_t type;
    assembly_formulation_t formulation;
    
    // Problem dimensions
    int num_unknowns;
    int num_rhs;
    int num_ports;
    
    // Matrix properties
    bool is_symmetric;
    bool is_hermitian;
    bool is_positive_definite;
    bool use_compression;
    
    // Frequency information
    double frequency;
    bool is_frequency_dependent;
    
    // Parallel processing
    bool use_parallel;
    int num_threads;
    int num_partitions;
    
    // Accuracy control
    double tolerance;
    int max_iterations;
    
    // Compression parameters (for ACA/MLFMM)
    double compression_tolerance;
    int max_rank;
    bool use_adaptive_rank;
} matrix_assembly_spec_t;

// Network assembly specification (PEEC)
typedef struct {
    assembly_type_t type;           // Always ASSEMBLY_TYPE_NETWORK
    
    // Network dimensions
    int num_nodes;
    int num_branches;
    int num_elements;
    int num_ports;
    
    // Element types
    bool include_resistance;
    bool include_inductance;
    bool include_capacitance;
    bool include_conductance;
    
    // Frequency dependence
    bool is_frequency_dependent;
    double frequency;
    
    // Skin effect and proximity effect
    bool include_skin_effect;
    bool include_proximity_effect;
    double skin_depth;
    
    // Retardation
    bool include_retardation;
    double retardation_tolerance;
    
    // Coupling
    bool include_mutual_coupling;
    double coupling_threshold;
    
    // Output format
    char spice_format[64];          // "SPICE", "SPECTRE", "HSPICE"
    bool export_touchstone;
    bool export_netlist;
} network_assembly_spec_t;

// Element assembly data
typedef struct {
    int element_id;
    int basis_function_id_i;
    int basis_function_id_j;
    
    // Physical properties
    int material_id;
    int layer_id;
    double area;
    double length;
    double volume;
    
    // Geometric data
    void* geometry_data;            // Pointer to element geometry
    void* basis_data_i;            // Basis function data for test function
    void* basis_data_j;            // Basis function data for source function
    
    // Integration data
    void* integration_data;         // Quadrature points and weights
    int num_integration_points;
    
    // Results
    double* impedance_matrix_entry; // Complex impedance value
    double* network_element_values; // R, L, C, G values
    
    // Flags
    bool is_self_term;
    bool is_near_field;
    bool is_far_field;
    bool is_singular;
    
    // Extended fields for unified assembler implementation
    int element_type;
    double* coordinates;
    double dimensions[3];
    double material_properties[4];
    complex_t* local_matrix;
    int* dof_indices;
    int num_dofs;
} element_assembly_data_t;

// Matrix assembly result
typedef struct {
    assembly_type_t type;
    int num_rows;
    int num_cols;
    int num_nonzeros;             // For sparse matrices
    
    // Dense matrix storage
    double* dense_matrix;           // Complex values (real, imag pairs)
    
    // Sparse matrix storage (CSR format)
    int* row_ptr;
    int* col_ind;
    double* sparse_values;
    
    // Block matrix storage (for ACA)
    int num_blocks;
    int* block_rows;
    int* block_cols;
    double** block_matrices;
    
    // Compressed representation (ACA)
    double** U_matrices;            // Left singular vectors
    double** V_matrices;            // Right singular vectors
    int* ranks;
    
    // Network representation (PEEC)
    void* network_data;              // Circuit network data
    char* spice_netlist;             // SPICE netlist string
    
    // Port information
    int* port_indices;
    int num_ports;
    double* port_impedances;
    
    // Metadata
    double assembly_time;
    double memory_usage;
    int compression_ratio;
    bool is_compressed;
} matrix_assembly_result_t;

// Network assembly result (PEEC)
typedef struct {
    int num_nodes;
    int num_branches;
    int num_elements;
    
    // Node data
    int* node_ids;
    double* node_voltages;          // Complex
    double* node_currents;          // Complex
    
    // Branch data
    int* branch_from_nodes;
    int* branch_to_nodes;
    double* branch_resistances;
    double* branch_inductances;
    double* branch_capacitances;
    double* branch_conductances;
    
    // Element data
    double* element_values[4];      // R, L, C, G arrays
    int* element_from_nodes;
    int* element_to_nodes;
    
    // Port data
    int* port_nodes;
    double* port_impedances;
    int num_ports;
    
    // SPICE netlist
    char* spice_netlist;
    char* touchstone_data;
    
    // Frequency domain data
    double* frequency_points;
    double** s_parameters;          // S-matrix
    double** z_parameters;          // Z-matrix
    double** y_parameters;          // Y-matrix
    int num_frequencies;
    
    // Metadata
    double assembly_time;
    double memory_usage;
    bool is_passive;
    bool is_stable;
} network_assembly_result_t;

/*********************************************************************
 * Unified Assembler Interface
 *********************************************************************/

typedef struct compressed_matrix compressed_matrix_t;

typedef struct matrix_assembler {
    int matrix_type;
    int matrix_size;
    int num_elements;
    int element_capacity;
    element_assembly_data_t* elements;
    complex_t* dense_matrix;
    sparse_matrix_t* sparse_matrix;
    compressed_matrix_t* compressed_matrix;
} matrix_assembler_t;
typedef struct network_assembler network_assembler_t;

// Matrix assembler (MoM)
matrix_assembler_t* matrix_assembler_create(const matrix_assembly_spec_t* spec);
void matrix_assembler_destroy(matrix_assembler_t* assembler);

// Convenience low-level APIs used by internal implementations
matrix_assembler_t* matrix_assembler_create_with_type(int matrix_type, int size);
void matrix_assembler_free(matrix_assembler_t* assembler);

int matrix_assembler_set_mesh(matrix_assembler_t* assembler, const mesh_t* mesh);
int matrix_assembler_set_kernel(matrix_assembler_t* assembler, kernel_engine_t* kernel);
int matrix_assembler_set_frequency(matrix_assembler_t* assembler, double frequency);

int matrix_assembler_add_element(matrix_assembler_t* assembler, 
                               const element_assembly_data_t* element_data);

matrix_assembly_result_t* matrix_assembler_build(matrix_assembler_t* assembler);
void matrix_assembly_result_destroy(matrix_assembly_result_t* result);

// Network assembler (PEEC)
network_assembler_t* network_assembler_create(const network_assembly_spec_t* spec);
void network_assembler_destroy(network_assembler_t* assembler);

int network_assembler_set_mesh(network_assembler_t* assembler, const mesh_t* mesh);
int network_assembler_set_kernel(network_assembler_t* assembler, kernel_engine_t* kernel);
int network_assembler_set_frequency(network_assembler_t* assembler, double frequency);

int network_assembler_add_element(network_assembler_t* assembler,
                                 const element_assembly_data_t* element_data);

network_assembly_result_t* network_assembler_build(network_assembler_t* assembler);
void network_assembly_result_destroy(network_assembly_result_t* result);

/*********************************************************************
 * Specialized Assemblers
 *********************************************************************/

// MoM-specific assemblers
matrix_assembler_t* mom_assembler_efie_create(const matrix_assembly_spec_t* spec);
matrix_assembler_t* mom_assembler_mfie_create(const matrix_assembly_spec_t* spec);
matrix_assembler_t* mom_assembler_cfie_create(const matrix_assembly_spec_t* spec);

// PEEC-specific assemblers  
network_assembler_t* peec_assembler_rlgc_create(const network_assembly_spec_t* spec);
network_assembler_t* peec_assembler_mna_create(const network_assembly_spec_t* spec);

// Hybrid assemblers
matrix_assembler_t* hybrid_assembler_schur_create(const matrix_assembly_spec_t* spec);
matrix_assembler_t* hybrid_assembler_ddm_create(const matrix_assembly_spec_t* spec);

/*********************************************************************
 * Assembly Utilities
 *********************************************************************/

// Element data creation
element_assembly_data_t* element_assembly_data_create(int element_id,
                                                      int basis_i, int basis_j);
void element_assembly_data_destroy(element_assembly_data_t* data);

// Basis function utilities
int assembler_compute_rwg_basis(const mesh_t* mesh, int element_id, 
                              void** basis_data, int* num_basis);
int assembler_compute_pulse_basis(const mesh_t* mesh, int element_id,
                                void** basis_data, int* num_basis);

// Integration utilities
int assembler_setup_integration_points(const mesh_t* mesh,
                                     int element_id_i, int element_id_j,
                                     void** integration_data, int* num_points);

// Matrix utilities
int assembler_compress_matrix(matrix_assembly_result_t* result, 
                            double tolerance, int max_rank);
int assembler_sparsify_matrix(matrix_assembly_result_t* result, double threshold);

// Network utilities
int assembler_check_network_passivity(network_assembly_result_t* result);
int assembler_check_network_stability(network_assembly_result_t* result);
int assembler_enforce_passivity(network_assembly_result_t* result, double tolerance);

// Performance monitoring
typedef struct {
    double assembly_time;
    double kernel_evaluation_time;
    double integration_time;
    double compression_time;
    double memory_usage;
    int num_kernel_evaluations;
    int num_integration_points;
    int compression_ratio;
} assembler_performance_t;

assembler_performance_t assembler_get_performance(const matrix_assembler_t* assembler);
assembler_performance_t network_assembler_get_performance(const network_assembler_t* assembler);

// Triangle-to-triangle integration for MoM
// Note: kernel_formulation_t is defined in core_kernels.h (already included)
// gauss_order: Optional Gauss quadrature order (1, 4, 7, or 8). If <= 0, uses default (4)
// Updated signature to include gauss_order parameter for adaptive integration
complex_t integrate_triangle_triangle(
    const geom_triangle_t* tri_i,
    const geom_triangle_t* tri_j,
    double frequency,
    kernel_formulation_t formulation,
    int gauss_order
);

#ifdef __cplusplus
}
#endif

#endif // CORE_ASSEMBLER_H
