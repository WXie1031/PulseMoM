/********************************************************************************
 * Matrix Assembler Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements matrix assembly operators.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 assembles operator matrix, not solver matrix.
 ********************************************************************************/

#include "matrix_assembler.h"
#include "../kernels/mom_kernel.h"
#include "../kernels/peec_kernel.h"
#include "../integration/integration_utils.h"
#include "../integration/singular_integration.h"
#include "../../common/types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

operator_matrix_t* matrix_assembler_create_matrix(
    matrix_type_t type,
    int num_rows,
    int num_cols) {
    
    if (num_rows <= 0 || num_cols <= 0) return NULL;
    
    operator_matrix_t* matrix = (operator_matrix_t*)calloc(1, sizeof(operator_matrix_t));
    if (!matrix) return NULL;
    
    matrix->type = type;
    matrix->num_rows = num_rows;
    matrix->num_cols = num_cols;
    matrix->is_symmetric = false;
    matrix->is_hermitian = false;
    
    if (type == MATRIX_TYPE_DENSE) {
        matrix->data.dense = (complex_t*)calloc(num_rows * num_cols, sizeof(complex_t));
        if (!matrix->data.dense) {
            free(matrix);
            return NULL;
        }
    } else {
        // Sparse and compressed matrices would be handled by L4 backend
        matrix->data.sparse = NULL;
    }
    
    return matrix;
}

void matrix_assembler_destroy_matrix(operator_matrix_t* matrix) {
    if (!matrix) return;
    
    if (matrix->type == MATRIX_TYPE_DENSE && matrix->data.dense) {
        free(matrix->data.dense);
    }
    // Sparse and compressed would be handled by L4 backend
    
    free(matrix);
}

int matrix_assembler_assemble_mom(
    const mesh_t* mesh,
    const rwg_basis_set_t* basis_set,
    const matrix_assembly_spec_t* spec,
    operator_matrix_t* matrix) {
    
    if (!mesh || !basis_set || !spec || !matrix) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer assembles operator matrix, not solver matrix
    // Create MoM kernel
    mom_kernel_t* kernel = mom_kernel_create(
        (mom_formulation_t)spec->formulation,
        spec->frequency
    );
    if (!kernel) return STATUS_ERROR_MEMORY_ALLOCATION;
    
    int num_basis = basis_set->num_basis_functions;
    
    // Allocate matrix if not already allocated
    if (!matrix->data.dense) {
        operator_matrix_t* new_matrix = matrix_assembler_create_matrix(
            MATRIX_TYPE_DENSE, num_basis, num_basis);
        if (!new_matrix) {
            mom_kernel_destroy(kernel);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        *matrix = *new_matrix;
        free(new_matrix);
    }
    
    // Assemble impedance matrix: Z_ij = <f_i, operator(f_j)>
    int i;
    #ifdef _OPENMP
    #pragma omp parallel for if(spec->use_parallel)
    #endif
    for (i = 0; i < num_basis; i++) {
        const rwg_basis_t* test_bf = rwg_basis_get(basis_set, i);
        if (!test_bf) continue;
        
        for (int j = 0; j < num_basis; j++) {
            const rwg_basis_t* source_bf = rwg_basis_get(basis_set, j);
            if (!source_bf) continue;
            
            // Get triangle elements
            const mesh_element_t* test_tri = &mesh->elements[test_bf->triangle_plus];
            const mesh_element_t* source_tri = &mesh->elements[source_bf->triangle_plus];
            
            // Get source and test points (centroids)
            real_t source_point[3] = {
                source_tri->centroid.x,
                source_tri->centroid.y,
                source_tri->centroid.z
            };
            real_t test_point[3] = {
                test_tri->centroid.x,
                test_tri->centroid.y,
                test_tri->centroid.z
            };
            
            // Evaluate kernel
            complex_t Z_ij;
            if (spec->formulation == ASSEMBLY_FORMULATION_EFIE) {
                Z_ij = mom_kernel_evaluate_efie(kernel, 
                    (mom_triangle_element_t*)source_tri,
                    (mom_triangle_element_t*)test_tri,
                    source_point, test_point);
            } else if (spec->formulation == ASSEMBLY_FORMULATION_MFIE) {
                Z_ij = mom_kernel_evaluate_mfie(kernel,
                    (mom_triangle_element_t*)source_tri,
                    (mom_triangle_element_t*)test_tri,
                    source_point, test_point);
            } else {
                Z_ij = mom_kernel_evaluate_cfie(kernel,
                    (mom_triangle_element_t*)source_tri,
                    (mom_triangle_element_t*)test_tri,
                    source_point, test_point, 0.5);  // Default alpha
            }
            
            // Store in matrix
            int idx = i * num_basis + j;
            matrix->data.dense[idx] = Z_ij;
        }
    }
    
    mom_kernel_destroy(kernel);
    return STATUS_SUCCESS;
}

int matrix_assembler_assemble_peec(
    const mesh_t* mesh,
    const matrix_assembly_spec_t* spec,
    operator_matrix_t* matrix) {
    
    if (!mesh || !spec || !matrix) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer assembles operator matrix for PEEC
    // Create PEEC kernel
    peec_kernel_t* kernel = peec_kernel_create(
        PEEC_FORMULATION_CLASSICAL,  // Use peec_formulation_t from physics layer
        spec->frequency
    );
    if (!kernel) return STATUS_ERROR_MEMORY_ALLOCATION;
    
    // Assemble PEEC circuit matrices: R, L, C, P
    // R: Resistance matrix (Ohms)
    // L: Inductance matrix (Henries)
    // C: Capacitance matrix (Farads)
    // P: Potential coefficient matrix (1/Farads)
    
    int num_elements = mesh->num_elements;
    if (matrix->num_rows != num_elements || matrix->num_cols != num_elements) {
        peec_kernel_destroy(kernel);
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Initialize matrix to zero
    if (matrix->type == MATRIX_TYPE_DENSE && matrix->data.dense) {
        complex_t* mat_data = matrix->data.dense;
        for (int i = 0; i < num_elements * num_elements; i++) {
            #if defined(_MSC_VER)
            mat_data[i].re = 0.0;
            mat_data[i].im = 0.0;
            #else
            mat_data[i] = 0.0 + 0.0 * I;
            #endif
        }
    }
    
    // Assemble R, L, C, P matrices
    // For PEEC, the system matrix is: Z = R + jωL + (1/jωC) = R + jωL - j/(ωC)
    // Or in terms of potential coefficients: Z = R + jωL + P/(jω)
    
    real_t omega = 2.0 * M_PI * spec->frequency;
    
    // Loop over all element pairs
    for (int i = 0; i < num_elements; i++) {
        const mesh_element_t* elem_i = &mesh->elements[i];
        if (elem_i->type != MESH_ELEMENT_TRIANGLE && 
            elem_i->type != MESH_ELEMENT_QUADRILATERAL) continue;
        
        // Get PEEC element properties (would come from material properties)
        peec_rectangle_element_t peec_elem_i;
        // Fill vertices (rectangle needs 4 vertices, triangle uses 3 + duplicate last)
        int num_verts = (elem_i->num_vertices < 4) ? elem_i->num_vertices : 4;
        for (int v = 0; v < num_verts; v++) {
            int vidx = (v < elem_i->num_vertices) ? elem_i->vertices[v] : elem_i->vertices[elem_i->num_vertices - 1];
            peec_elem_i.vertices[v][0] = mesh->vertices[vidx].position.x;
            peec_elem_i.vertices[v][1] = mesh->vertices[vidx].position.y;
            peec_elem_i.vertices[v][2] = mesh->vertices[vidx].position.z;
        }
        // For triangles, duplicate last vertex to make rectangle
        if (elem_i->num_vertices == 3) {
            peec_elem_i.vertices[3][0] = peec_elem_i.vertices[2][0];
            peec_elem_i.vertices[3][1] = peec_elem_i.vertices[2][1];
            peec_elem_i.vertices[3][2] = peec_elem_i.vertices[2][2];
        }
        peec_elem_i.area = elem_i->area;
        // Compute normal (simplified: use element normal if available)
        if (elem_i->normal.x != 0.0 || elem_i->normal.y != 0.0 || elem_i->normal.z != 0.0) {
            peec_elem_i.normal[0] = elem_i->normal.x;
            peec_elem_i.normal[1] = elem_i->normal.y;
            peec_elem_i.normal[2] = elem_i->normal.z;
        } else {
            // Compute normal from first two edges
            real_t v0v1[3] = {
                peec_elem_i.vertices[1][0] - peec_elem_i.vertices[0][0],
                peec_elem_i.vertices[1][1] - peec_elem_i.vertices[0][1],
                peec_elem_i.vertices[1][2] - peec_elem_i.vertices[0][2]
            };
            real_t v0v2[3] = {
                peec_elem_i.vertices[2][0] - peec_elem_i.vertices[0][0],
                peec_elem_i.vertices[2][1] - peec_elem_i.vertices[0][1],
                peec_elem_i.vertices[2][2] - peec_elem_i.vertices[0][2]
            };
            peec_elem_i.normal[0] = v0v1[1] * v0v2[2] - v0v1[2] * v0v2[1];
            peec_elem_i.normal[1] = v0v1[2] * v0v2[0] - v0v1[0] * v0v2[2];
            peec_elem_i.normal[2] = v0v1[0] * v0v2[1] - v0v1[1] * v0v2[0];
            // Normalize
            real_t norm_len = sqrt(peec_elem_i.normal[0] * peec_elem_i.normal[0] +
                                   peec_elem_i.normal[1] * peec_elem_i.normal[1] +
                                   peec_elem_i.normal[2] * peec_elem_i.normal[2]);
            if (norm_len > 1e-10) {
                peec_elem_i.normal[0] /= norm_len;
                peec_elem_i.normal[1] /= norm_len;
                peec_elem_i.normal[2] /= norm_len;
            }
        }
        
        for (int j = 0; j < num_elements; j++) {
            const mesh_element_t* elem_j = &mesh->elements[j];
            if (elem_j->type != MESH_ELEMENT_TRIANGLE && 
                elem_j->type != MESH_ELEMENT_QUADRILATERAL) continue;
            
            peec_rectangle_element_t peec_elem_j;
            // Fill vertices (rectangle needs 4 vertices, triangle uses 3 + duplicate last)
            int num_verts_j = (elem_j->num_vertices < 4) ? elem_j->num_vertices : 4;
            for (int v = 0; v < num_verts_j; v++) {
                int vidx = (v < elem_j->num_vertices) ? elem_j->vertices[v] : elem_j->vertices[elem_j->num_vertices - 1];
                peec_elem_j.vertices[v][0] = mesh->vertices[vidx].position.x;
                peec_elem_j.vertices[v][1] = mesh->vertices[vidx].position.y;
                peec_elem_j.vertices[v][2] = mesh->vertices[vidx].position.z;
            }
            // For triangles, duplicate last vertex to make rectangle
            if (elem_j->num_vertices == 3) {
                peec_elem_j.vertices[3][0] = peec_elem_j.vertices[2][0];
                peec_elem_j.vertices[3][1] = peec_elem_j.vertices[2][1];
                peec_elem_j.vertices[3][2] = peec_elem_j.vertices[2][2];
            }
            peec_elem_j.area = elem_j->area;
            // Compute normal (simplified: use element normal if available)
            if (elem_j->normal.x != 0.0 || elem_j->normal.y != 0.0 || elem_j->normal.z != 0.0) {
                peec_elem_j.normal[0] = elem_j->normal.x;
                peec_elem_j.normal[1] = elem_j->normal.y;
                peec_elem_j.normal[2] = elem_j->normal.z;
            } else {
                // Compute normal from first two edges
                real_t v0v1[3] = {
                    peec_elem_j.vertices[1][0] - peec_elem_j.vertices[0][0],
                    peec_elem_j.vertices[1][1] - peec_elem_j.vertices[0][1],
                    peec_elem_j.vertices[1][2] - peec_elem_j.vertices[0][2]
                };
                real_t v0v2[3] = {
                    peec_elem_j.vertices[2][0] - peec_elem_j.vertices[0][0],
                    peec_elem_j.vertices[2][1] - peec_elem_j.vertices[0][1],
                    peec_elem_j.vertices[2][2] - peec_elem_j.vertices[0][2]
                };
                peec_elem_j.normal[0] = v0v1[1] * v0v2[2] - v0v1[2] * v0v2[1];
                peec_elem_j.normal[1] = v0v1[2] * v0v2[0] - v0v1[0] * v0v2[2];
                peec_elem_j.normal[2] = v0v1[0] * v0v2[1] - v0v1[1] * v0v2[0];
                // Normalize
                real_t norm_len = sqrt(peec_elem_j.normal[0] * peec_elem_j.normal[0] +
                                       peec_elem_j.normal[1] * peec_elem_j.normal[1] +
                                       peec_elem_j.normal[2] * peec_elem_j.normal[2]);
                if (norm_len > 1e-10) {
                    peec_elem_j.normal[0] /= norm_len;
                    peec_elem_j.normal[1] /= norm_len;
                    peec_elem_j.normal[2] /= norm_len;
                }
            }
            
            // Compute partial elements
            real_t R_ij = 0.0;
            if (i == j) {
                // Self-resistance: R_ii = ρ * l / A
                // R = (resistivity * length) / (area * thickness)
                // For rectangular element: R = ρ * (perimeter / 4) / (area * thickness)
                real_t conductivity = 5.8e7;  // Copper (default, would get from material properties)
                real_t resistivity = 1.0 / conductivity;
                real_t thickness = 1e-6;  // 1μm (default, would get from material properties)
                
                // Compute effective length from area (assuming square)
                real_t effective_length = sqrt(elem_i->area);
                
                // Compute resistance: R = ρ * l / (w * t) where w*t = area
                // For square: R = ρ * sqrt(area) / (area * thickness)
                R_ij = resistivity * effective_length / (elem_i->area * thickness);
            }
            
            // Inductance: L_ij
            real_t L_ij = peec_kernel_evaluate_inductance(kernel, &peec_elem_i, &peec_elem_j);
            
            // Potential coefficient: P_ij
            real_t P_ij = peec_kernel_evaluate_potential_coefficient(kernel, &peec_elem_i, &peec_elem_j);
            
            // Assemble into system matrix: Z_ij = R_ij + jωL_ij - j/(ω*P_ij)
            // Note: P matrix is inverse of C matrix, so 1/C = P
            complex_t Z_ij;
            #if defined(_MSC_VER)
            Z_ij.re = R_ij;
            Z_ij.im = omega * L_ij - 1.0 / (omega * P_ij);
            #else
            Z_ij = R_ij + I * (omega * L_ij - 1.0 / (omega * P_ij));
            #endif
            
            // Store in matrix
            if (matrix->type == MATRIX_TYPE_DENSE && matrix->data.dense) {
                int idx = i * num_elements + j;
                matrix->data.dense[idx] = Z_ij;
            }
        }
    }
    
    peec_kernel_destroy(kernel);
    return STATUS_SUCCESS;
}

int matrix_assembler_assemble_excitation(
    const mesh_t* mesh,
    const rwg_basis_set_t* basis_set,
    const mom_plane_wave_t* excitation,
    operator_vector_t* rhs) {
    
    if (!mesh || !basis_set || !excitation || !rhs) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer assembles RHS vector from excitation
    // V_i = <f_i, E_inc>
    int num_basis = basis_set->num_basis_functions;
    
    if (rhs->size != num_basis) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Initialize RHS
    for (int i = 0; i < num_basis; i++) {
        #if defined(_MSC_VER)
        rhs->data[i].re = 0.0;
        rhs->data[i].im = 0.0;
        #else
        rhs->data[i] = 0.0 + 0.0 * I;
        #endif
    }
    
    // Compute incident field on each basis function
    for (int i = 0; i < num_basis; i++) {
        const rwg_basis_t* bf = rwg_basis_get(basis_set, i);
        if (!bf) continue;
        
        // Get basis function center and edges
        real_t center[3] = {
            bf->support_center.x,
            bf->support_center.y,
            bf->support_center.z
        };
        
        // Compute incident field: E_inc = E0 * exp(-jk·r)
        // For RWG basis functions: V_i = <f_i, E_inc> = ∫ f_i(r) · E_inc(r) dS
        real_t k_dot_r = excitation->k_vector.x * center[0] +
                         excitation->k_vector.y * center[1] +
                         excitation->k_vector.z * center[2];
        real_t phase = -k_dot_r;
        
        // Compute incident field vector at basis function center
        real_t k_mag = sqrt(excitation->k_vector.x * excitation->k_vector.x +
                           excitation->k_vector.y * excitation->k_vector.y +
                           excitation->k_vector.z * excitation->k_vector.z);
        
        // Normalize k vector for phase calculation
        real_t k_norm[3];
        if (k_mag > NUMERICAL_EPSILON) {
            k_norm[0] = excitation->k_vector.x / k_mag;
            k_norm[1] = excitation->k_vector.y / k_mag;
            k_norm[2] = excitation->k_vector.z / k_mag;
        } else {
            k_norm[0] = 0.0;
            k_norm[1] = 0.0;
            k_norm[2] = 1.0;
        }
        
        // Incident field vector: E_inc = E0 * polarization * exp(-jk·r)
        real_t E_inc[3];
        real_t exp_re = cos(phase);
        real_t exp_im = sin(phase);
        E_inc[0] = excitation->amplitude * excitation->polarization.x * exp_re;
        E_inc[1] = excitation->amplitude * excitation->polarization.y * exp_re;
        E_inc[2] = excitation->amplitude * excitation->polarization.z * exp_re;
        
        // Compute RHS: V_i = <f_i, E_inc> = ∫ f_i(r) · E_inc(r) dS
        // For RWG basis function, this is approximately:
        // V_i ≈ (f_i_center · E_inc_center) * area_i
        // where f_i_center is the basis function value at center
        
        // Get basis function direction from RWG basis function
        // RWG basis function points along the edge from minus to plus triangle
        real_t basis_dir[3] = {0.0, 0.0, 0.0};
        if (bf->edge_length > NUMERICAL_EPSILON) {
            // Normalize edge vector to get direction
            basis_dir[0] = bf->edge_vector.x / bf->edge_length;
            basis_dir[1] = bf->edge_vector.y / bf->edge_length;
            basis_dir[2] = bf->edge_vector.z / bf->edge_length;
        } else {
            // Fallback: use default direction
            basis_dir[0] = 1.0;
            basis_dir[1] = 0.0;
            basis_dir[2] = 0.0;
        }
        
        // Dot product: f_i · E_inc
        real_t dot_product = basis_dir[0] * E_inc[0] +
                            basis_dir[1] * E_inc[1] +
                            basis_dir[2] * E_inc[2];
        
        // Get basis function support area (from RWG basis function)
        // RWG basis function spans two triangles: plus and minus
        real_t basis_area = 0.0;
        if (bf->triangle_plus >= 0 && bf->triangle_minus >= 0) {
            // Support area is the sum of plus and minus triangle areas
            basis_area = bf->area_plus + bf->area_minus;
        } else if (bf->triangle_plus >= 0) {
            // Only plus triangle (boundary edge)
            basis_area = bf->area_plus;
        } else if (bf->triangle_minus >= 0) {
            // Only minus triangle (boundary edge)
            basis_area = bf->area_minus;
        } else {
            // Fallback: use default area
            basis_area = 1.0;
        }
        
        // RHS value: V_i = <f_i, E_inc> ≈ (f_i · E_inc) * area
        real_t rhs_magnitude = dot_product * basis_area;
        
        #if defined(_MSC_VER)
        rhs->data[i].re = rhs_magnitude * exp_re;
        rhs->data[i].im = rhs_magnitude * exp_im;
        #else
        rhs->data[i] = rhs_magnitude * (exp_re + I * exp_im);
        #endif
    }
    
    return STATUS_SUCCESS;
}
