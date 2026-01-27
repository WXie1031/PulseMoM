/********************************************************************************
 * Coupling Operators Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements coupling operators between different domains.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines coupling operators, not execution orchestration.
 ********************************************************************************/

#include "coupling_operator.h"
#include "../../common/types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

coupling_operator_t* coupling_operator_create(
    hybrid_coupling_type_t coupling_type,
    int source_domain,
    int target_domain) {
    
    coupling_operator_t* op = (coupling_operator_t*)calloc(1, sizeof(coupling_operator_t));
    if (!op) return NULL;
    
    op->coupling_type = coupling_type;
    op->source_domain = source_domain;
    op->target_domain = target_domain;
    op->coupling_strength = 1.0;  // Default
    
    return op;
}

void coupling_operator_destroy(coupling_operator_t* op) {
    if (!op) return;
    free(op);
}

int coupling_operator_assemble_matrix(
    const coupling_operator_t* op,
    const hybrid_interface_point_t* interface_points,
    int num_points,
    coupling_matrix_t* matrix) {
    
    if (!op || !interface_points || !matrix || num_points <= 0) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer assembles coupling operator matrix
    // This defines HOW to compute coupling, not WHEN to execute it
    
    // Determine source and target sizes from interface points
    // For most coupling types, source and target sizes match interface points
    int source_size = num_points;
    int target_size = num_points;
    
    // For different coupling types, sizes might differ
    // This would be determined by the coupling physics (L1 layer)
    // L3 layer just assembles the operator matrix based on the coupling type
    
    // Allocate coupling matrix
    matrix->source_size = source_size;
    matrix->target_size = target_size;
    matrix->matrix = (complex_t*)calloc(source_size * target_size, sizeof(complex_t));
    if (!matrix->matrix) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Assemble coupling matrix based on coupling type
    for (int i = 0; i < source_size; i++) {
        for (int j = 0; j < target_size; j++) {
            int idx = i * target_size + j;
            
            // Compute coupling coefficient based on physical coupling type
            // L3 layer defines the operator form, not physics (physics is in L1)
            real_t distance = 0.0;
            if (i < num_points && j < num_points) {
                real_t dx = interface_points[i].position.x - interface_points[j].position.x;
                real_t dy = interface_points[i].position.y - interface_points[j].position.y;
                real_t dz = interface_points[i].position.z - interface_points[j].position.z;
                distance = sqrt(dx * dx + dy * dy + dz * dz);
            }
            
            // Compute coupling coefficient based on coupling type and distance
            // L3 layer defines the operator form, physics is in L1
            real_t coupling = 0.0;
            
            // Different coupling types have different distance dependencies
            // L3 layer defines operator forms based on L1 physics definitions
            switch (op->coupling_type) {
                case HYBRID_COUPLING_ELECTRIC_FIELD:
                    // Electric field coupling: decreases as 1/r² (capacitive)
                    if (distance > NUMERICAL_EPSILON) {
                        coupling = op->coupling_strength / (distance * distance);
                    } else {
                        coupling = op->coupling_strength;  // Self-coupling
                    }
                    break;
                    
                case HYBRID_COUPLING_MAGNETIC_FIELD:
                    // Magnetic field coupling: decreases as 1/r³ (inductive)
                    if (distance > NUMERICAL_EPSILON) {
                        coupling = op->coupling_strength / (distance * distance * distance);
                    } else {
                        coupling = op->coupling_strength;  // Self-coupling
                    }
                    break;
                    
                case HYBRID_COUPLING_CURRENT_DENSITY:
                case HYBRID_COUPLING_VOLTAGE_POTENTIAL:
                case HYBRID_COUPLING_POWER_FLOW:
                case HYBRID_COUPLING_MIXED:
                    // Electromagnetic coupling: Green's function form
                    // G(r) = exp(-jk*r) / (4*π*r)
                    // Used for full-wave coupling
                    if (distance > NUMERICAL_EPSILON) {
                        real_t k = 2.0 * M_PI * op->frequency / C0;
                        real_t kr = k * distance;
                        real_t G_magnitude = 1.0 / (4.0 * M_PI * distance);
                        real_t phase = -kr;
                        coupling = op->coupling_strength * G_magnitude;
                        
                        #if defined(_MSC_VER)
                        matrix->matrix[idx].re = coupling * cos(phase);
                        matrix->matrix[idx].im = coupling * sin(phase);
                        #else
                        matrix->matrix[idx] = coupling * cexp(I * phase);
                        #endif
                        continue;  // Skip default assignment
                    } else {
                        coupling = op->coupling_strength;  // Self-coupling
                    }
                    break;
                    
                default:
                    // Default: distance-dependent coupling
                    if (distance > NUMERICAL_EPSILON) {
                        coupling = op->coupling_strength / (1.0 + distance);
                    } else {
                        coupling = op->coupling_strength;  // Self-coupling
                    }
                    break;
            }
            
            #if defined(_MSC_VER)
            matrix->matrix[idx].re = coupling;
            matrix->matrix[idx].im = 0.0;
            #else
            matrix->matrix[idx] = coupling + 0.0 * I;
            #endif
        }
    }
    
    return STATUS_SUCCESS;
}

int coupling_operator_apply(
    const coupling_matrix_t* coupling_matrix,
    const operator_vector_t* source_vector,
    operator_vector_t* target_vector) {
    
    if (!coupling_matrix || !source_vector || !target_vector) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (source_vector->size != coupling_matrix->source_size ||
        target_vector->size != coupling_matrix->target_size) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer defines the operator: target = coupling_matrix * source
    // Initialize target vector
    for (int i = 0; i < target_vector->size; i++) {
        #if defined(_MSC_VER)
        target_vector->data[i].re = 0.0;
        target_vector->data[i].im = 0.0;
        #else
        target_vector->data[i] = 0.0 + 0.0 * I;
        #endif
    }
    
    // Apply coupling matrix
    for (int i = 0; i < coupling_matrix->target_size; i++) {
        complex_t sum;
        #if defined(_MSC_VER)
        sum.re = 0.0;
        sum.im = 0.0;
        #else
        sum = 0.0 + 0.0 * I;
        #endif
        
        for (int j = 0; j < coupling_matrix->source_size; j++) {
            int idx = i * coupling_matrix->source_size + j;
            #if defined(_MSC_VER)
            complex_t prod;
            prod.re = coupling_matrix->matrix[idx].re * source_vector->data[j].re -
                      coupling_matrix->matrix[idx].im * source_vector->data[j].im;
            prod.im = coupling_matrix->matrix[idx].re * source_vector->data[j].im +
                      coupling_matrix->matrix[idx].im * source_vector->data[j].re;
            sum.re += prod.re;
            sum.im += prod.im;
            #else
            sum += coupling_matrix->matrix[idx] * source_vector->data[j];
            #endif
        }
        
        target_vector->data[i] = sum;
    }
    
    return STATUS_SUCCESS;
}

coupling_matrix_t* coupling_operator_create_matrix(
    int source_size,
    int target_size) {
    
    if (source_size <= 0 || target_size <= 0) return NULL;
    
    coupling_matrix_t* matrix = (coupling_matrix_t*)calloc(1, sizeof(coupling_matrix_t));
    if (!matrix) return NULL;
    
    matrix->source_size = source_size;
    matrix->target_size = target_size;
    matrix->matrix = (complex_t*)calloc(source_size * target_size, sizeof(complex_t));
    if (!matrix->matrix) {
        free(matrix);
        return NULL;
    }
    
    return matrix;
}

void coupling_operator_destroy_matrix(coupling_matrix_t* matrix) {
    if (!matrix) return;
    
    if (matrix->matrix) {
        free(matrix->matrix);
    }
    free(matrix);
}
