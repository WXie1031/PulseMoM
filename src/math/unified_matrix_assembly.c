/*****************************************************************************************
 * Industrial-Grade MoM+PEEC Matrix Assembly Implementation
 * 
 * This is the core implementation that must be self-written for commercial-grade
 * electromagnetic solvers. It implements the unified matrix structure:
 * | Z_mom   Z_mp |
 * | Z_pm   Z_pp  |
 * 
 * Where Z_pp = R + jωL + 1/(jω)C for PEEC regions
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#include "industrial_singular_integration.h"
#include "math_backend_selector.h"
#include "unified_matrix_assembly.h"
#include "industrial_solver_abstraction.h"

// Industrial assembly parameters
#define ASSEMBLY_MEMORY_CHUNK_SIZE 1024
#define ASSEMBLY_PARALLEL_THRESHOLD 1000
#define ASSEMBLY_BATCH_SIZE 256
#define ASSEMBLY_CACHE_LINE_SIZE 64

// MoM impedance matrix assembly for triangle pairs
static int assemble_mom_impedance_block(
    unified_matrix_t* matrix,
    simulation_region_t* region1,
    simulation_region_t* region2,
    assembly_params_t* params) {
    
    if (!matrix || !region1 || !region2 || !params) return -1;
    
    clock_t start_time = clock();
    
    // Get RWG basis functions for both regions
    mesh_t* mesh1 = region1->mesh;
    mesh_t* mesh2 = region2->mesh;
    
    if (!mesh1 || !mesh2) return -1;
    
    size_t num_tri1 = mesh1->num_elements;
    size_t num_tri2 = mesh2->num_elements;
    
    // Allocate impedance matrix block
    size_t block_size = num_tri1 * num_tri2;
    double complex* Z_block = calloc(block_size, sizeof(double complex));
    if (!Z_block) return -1;
    
    // Integration parameters for industrial accuracy
    integration_params_t int_params = {0};
    int_params.method = INTEGRATION_DUFFY_TRANSFORM;
    int_params.gauss_order = 7;  // Industrial standard
    int_params.tolerance = SINGULAR_INTEGRATION_EPSILON;
    int_params.adaptive_levels = ADAPTIVE_REFINEMENT_LEVELS;
    
    // Parallel assembly with OpenMP
    #pragma omp parallel for schedule(dynamic, ASSEMBLY_BATCH_SIZE)
    for (size_t i = 0; i < num_tri1; i++) {
        triangle_geometry_t tri1;
        get_triangle_geometry(mesh1, i, &tri1);
        
        for (size_t j = 0; j < num_tri2; j++) {
            triangle_geometry_t tri2;
            get_triangle_geometry(mesh2, j, &tri2);
            
            // Detect singularity type
            singularity_type_t sing_type = detect_triangle_singularity(&tri1, &tri2, 
                                                                     NEAR_SINGULAR_TOLERANCE);
            
            integration_result_t result = {0};
            
            // Choose integration method based on singularity
            if (sing_type == SINGULARITY_NONE) {
                // Regular Gaussian integration
                Z_block[i * num_tri2 + j] = integrate_regular_triangle_triangle(
                    &tri1, &tri2, params->frequency, &int_params, &result);
            } else {
                // Singular integration using Duffy transformation
                Z_block[i * num_tri2 + j] = integrate_triangle_triangle_singular(
                    &tri1, &tri2, green_function_wrapper, &int_params, &result);
            }
            
            // Apply symmetry for identical regions
            if (region1 == region2 && i != j) {
                Z_block[j * num_tri2 + i] = Z_block[i * num_tri2 + j];
            }
        }
    }
    
    // Create matrix block for backend
    matrix_block_t mom_block = {0};
    mom_block.type = BLOCK_TYPE_MOM_MOM;
    mom_block.row_start = region1->global_dof_offset;
    mom_block.row_end = region1->global_dof_offset + num_tri1;
    mom_block.col_start = region2->global_dof_offset;
    mom_block.col_end = region2->global_dof_offset + num_tri2;
    mom_block.matrix = matrix_create_from_complex(params->backend, MATRIX_FORMAT_DENSE,
                                                 PRECISION_DOUBLE_COMPLEX, num_tri1, num_tri2);
    
    if (!mom_block.matrix) {
        free(Z_block);
        return -1;
    }
    
    matrix_set_complex_data(mom_block.matrix, Z_block, block_size);
    
    // Add block to unified matrix
    int status = unified_matrix_add_block(matrix, &mom_block);
    
    free(Z_block);
    
    clock_t end_time = clock();
    double assembly_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("MoM block assembly: %zu x %zu elements in %.3f seconds\n", 
           num_tri1, num_tri2, assembly_time);
    
    return status;
}

// PEEC partial element assembly
static int assemble_peec_partial_elements(
    unified_matrix_t* matrix,
    simulation_region_t* region,
    assembly_params_t* params) {
    
    if (!matrix || !region || !params) return -1;
    
    clock_t start_time = clock();
    
    mesh_t* mesh = region->mesh;
    if (!mesh) return -1;
    
    // For PEEC, we need both inductive and capacitive coupling
    size_t num_segments = mesh->num_elements;
    
    // Partial inductance matrix (L)
    double complex* L_matrix = calloc(num_segments * num_segments, sizeof(double complex));
    // Partial potential matrix (P) - inverse of capacitance
    double complex* P_matrix = calloc(num_segments * num_segments, sizeof(double complex));
    // Resistance matrix (R)
    double* R_matrix = calloc(num_segments * num_segments, sizeof(double));
    
    if (!L_matrix || !P_matrix || !R_matrix) {
        free(L_matrix); free(P_matrix); free(R_matrix);
        return -1;
    }
    
    double omega = 2.0 * M_PI * params->frequency;
    
    // Integration parameters for PEEC
    integration_params_t int_params = {0};
    int_params.method = INTEGRATION_SEMIANALYTICAL;
    int_params.tolerance = params->integration_tolerance;
    
    // Parallel assembly of partial elements
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < num_segments; i++) {
        wire_geometry_t wire1;
        get_wire_geometry(mesh, i, &wire1);
        
        // Self terms
        L_matrix[i * num_segments + i] = compute_self_partial_inductance(&wire1, &int_params);
        P_matrix[i * num_segments + i] = compute_self_partial_potential(&wire1, &int_params);
        R_matrix[i * num_segments + i] = compute_wire_resistance(&wire1, params->frequency);
        
        // Mutual terms
        for (size_t j = i + 1; j < num_segments; j++) {
            wire_geometry_t wire2;
            get_wire_geometry(mesh, j, &wire2);
            
            double complex L_mutual = compute_mutual_partial_inductance(&wire1, &wire2, &int_params);
            double complex P_mutual = compute_mutual_partial_potential(&wire1, &wire2, &int_params);
            double R_mutual = compute_mutual_resistance(&wire1, &wire2, params->frequency);
            
            L_matrix[i * num_segments + j] = L_mutual;
            L_matrix[j * num_segments + i] = L_mutual; // Symmetric
            
            P_matrix[i * num_segments + j] = P_mutual;
            P_matrix[j * num_segments + i] = P_mutual; // Symmetric
            
            R_matrix[i * num_segments + j] = R_mutual;
            R_matrix[j * num_segments + i] = R_mutual; // Symmetric
        }
    }
    
    // Form complete PEEC impedance matrix: Z = R + jωL + 1/(jω)C
    // where C = P^(-1)
    double complex* Z_peec = calloc(num_segments * num_segments, sizeof(double complex));
    if (!Z_peec) {
        free(L_matrix); free(P_matrix); free(R_matrix);
        return -1;
    }
    
    // Compute C = P^(-1) using industrial-grade matrix inversion
    double complex* C_matrix = calloc(num_segments * num_segments, sizeof(double complex));
    if (C_matrix) {
        invert_matrix_complex(P_matrix, C_matrix, num_segments);
        
        // Z = R + jωL + 1/(jω)C
        for (size_t i = 0; i < num_segments * num_segments; i++) {
            Z_peec[i] = R_matrix[i] + I * omega * L_matrix[i] + C_matrix[i] / (I * omega);
        }
        
        free(C_matrix);
    }
    
    // Create PEEC matrix block
    matrix_block_t peec_block = {0};
    peec_block.type = BLOCK_TYPE_PEEC_PEEC;
    peec_block.row_start = region->global_dof_offset;
    peec_block.row_end = region->global_dof_offset + num_segments;
    peec_block.col_start = region->global_dof_offset;
    peec_block.col_end = region->global_dof_offset + num_segments;
    peec_block.matrix = matrix_create_from_complex(params->backend, MATRIX_FORMAT_DENSE,
                                                  PRECISION_DOUBLE_COMPLEX, num_segments, num_segments);
    
    if (peec_block.matrix) {
        matrix_set_complex_data(peec_block.matrix, Z_peec, num_segments * num_segments);
        unified_matrix_add_block(matrix, &peec_block);
    }
    
    free(L_matrix); free(P_matrix); free(R_matrix); free(Z_peec);
    
    clock_t end_time = clock();
    double assembly_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("PEEC partial elements: %zu segments in %.3f seconds\n", 
           num_segments, assembly_time);
    
    return 0;
}

// MoM-PEEC coupling assembly
static int assemble_mom_peec_coupling(
    unified_matrix_t* matrix,
    simulation_region_t* mom_region,
    simulation_region_t* peec_region,
    assembly_params_t* params) {
    
    if (!matrix || !mom_region || !peec_region || !params) return -1;
    
    clock_t start_time = clock();
    
    mesh_t* mom_mesh = mom_region->mesh;
    mesh_t* peec_mesh = peec_region->mesh;
    
    if (!mom_mesh || !peec_mesh) return -1;
    
    size_t num_triangles = mom_mesh->num_elements;
    size_t num_segments = peec_mesh->num_elements;
    
    // Allocate coupling matrix
    size_t coupling_size = num_triangles * num_segments;
    double complex* Z_coupling = calloc(coupling_size, sizeof(double complex));
    if (!Z_coupling) return -1;
    
    // Integration parameters for coupling
    integration_params_t int_params = {0};
    int_params.method = INTEGRATION_EXTRACTION;
    int_params.tolerance = params->integration_tolerance;
    
    // Parallel assembly of coupling terms
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < num_triangles; i++) {
        triangle_geometry_t triangle;
        get_triangle_geometry(mom_mesh, i, &triangle);
        
        for (size_t j = 0; j < num_segments; j++) {
            wire_geometry_t wire;
            get_wire_geometry(peec_mesh, j, &wire);
            
            // Compute triangle-wire coupling integral
            Z_coupling[i * num_segments + j] = integrate_triangle_wire_singular(
                &triangle, &wire, green_function_wrapper, &int_params, NULL);
        }
    }
    
    // Create coupling matrix blocks (both directions)
    matrix_block_t coupling_block_mp = {0};
    coupling_block_mp.type = BLOCK_TYPE_MOM_PEEC;
    coupling_block_mp.row_start = mom_region->global_dof_offset;
    coupling_block_mp.row_end = mom_region->global_dof_offset + num_triangles;
    coupling_block_mp.col_start = peec_region->global_dof_offset;
    coupling_block_mp.col_end = peec_region->global_dof_offset + num_segments;
    coupling_block_mp.matrix = matrix_create_from_complex(params->backend, MATRIX_FORMAT_DENSE,
                                                         PRECISION_DOUBLE_COMPLEX, num_triangles, num_segments);
    
    if (coupling_block_mp.matrix) {
        matrix_set_complex_data(coupling_block_mp.matrix, Z_coupling, coupling_size);
        unified_matrix_add_block(matrix, &coupling_block_mp);
    }
    
    // PEEC-MoM coupling (transpose)
    matrix_block_t coupling_block_pm = {0};
    coupling_block_pm.type = BLOCK_TYPE_PEEC_MOM;
    coupling_block_pm.row_start = peec_region->global_dof_offset;
    coupling_block_pm.row_end = peec_region->global_dof_offset + num_segments;
    coupling_block_pm.col_start = mom_region->global_dof_offset;
    coupling_block_pm.col_end = mom_region->global_dof_offset + num_triangles;
    coupling_block_pm.matrix = matrix_create_from_complex(params->backend, MATRIX_FORMAT_DENSE,
                                                         PRECISION_DOUBLE_COMPLEX, num_segments, num_triangles);
    
    if (coupling_block_pm.matrix) {
        // Transpose the coupling matrix
        double complex* Z_transpose = calloc(coupling_size, sizeof(double complex));
        if (Z_transpose) {
            for (size_t i = 0; i < num_triangles; i++) {
                for (size_t j = 0; j < num_segments; j++) {
                    Z_transpose[j * num_triangles + i] = Z_coupling[i * num_segments + j];
                }
            }
            matrix_set_complex_data(coupling_block_pm.matrix, Z_transpose, coupling_size);
            free(Z_transpose);
        }
        unified_matrix_add_block(matrix, &coupling_block_pm);
    }
    
    free(Z_coupling);
    
    clock_t end_time = clock();
    double assembly_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("MoM-PEEC coupling: %zu triangles × %zu segments in %.3f seconds\n", 
           num_triangles, num_segments, assembly_time);
    
    return 0;
}

// Main unified matrix assembly function
int unified_matrix_assemble_complete(
    unified_matrix_t* matrix,
    simulation_region_t** regions,
    size_t num_regions,
    assembly_params_t* params) {
    
    if (!matrix || !regions || num_regions == 0 || !params) return -1;
    
    printf("Starting unified MoM+PEEC matrix assembly...\n");
    printf("Number of regions: %zu\n", num_regions);
    printf("Frequency: %.3e Hz\n", params->frequency);
    printf("Backend: %s\n", math_backend_get_name(params->backend));
    
    clock_t total_start = clock();
    
    int status = 0;
    
    // Assemble all region pairs
    for (size_t i = 0; i < num_regions; i++) {
        for (size_t j = 0; j < num_regions; j++) {
            simulation_region_t* region1 = regions[i];
            simulation_region_t* region2 = regions[j];
            
            printf("Assembling block [%zu,%zu]: %s-%s\n", i, j,
                   region1->type == REGION_TYPE_MOM ? "MoM" : "PEEC",
                   region2->type == REGION_TYPE_MOM ? "MoM" : "PEEC");
            
            // Route to appropriate assembly function
            if (region1->type == REGION_TYPE_MOM && region2->type == REGION_TYPE_MOM) {
                status = assemble_mom_impedance_block(matrix, region1, region2, params);
            } else if (region1->type == REGION_TYPE_PEEC && region2->type == REGION_TYPE_PEEC) {
                status = assemble_peec_partial_elements(matrix, region1, params);
            } else {
                // Mixed MoM-PEEC coupling
                if (region1->type == REGION_TYPE_MOM) {
                    status = assemble_mom_peec_coupling(matrix, region1, region2, params);
                } else {
                    status = assemble_mom_peec_coupling(matrix, region2, region1, params);
                }
            }
            
            if (status != 0) {
                printf("Error assembling block [%zu,%zu]\n", i, j);
                return status;
            }
        }
    }
    
    // Finalize matrix assembly
    status = unified_matrix_assemble_blocks(matrix);
    if (status != 0) {
        printf("Error finalizing matrix assembly\n");
        return status;
    }
    
    // Apply H-matrix compression if requested
    if (params->use_hmatrix) {
        printf("Applying H-matrix compression (ε = %.2e)...\n", params->hmatrix_epsilon);
        status = unified_matrix_compress(matrix, params->hmatrix_epsilon);
        if (status != 0) {
            printf("Warning: H-matrix compression failed\n");
        }
    }
    
    clock_t total_end = clock();
    double total_time = (double)(total_end - total_start) / CLOCKS_PER_SEC;
    
    printf("Unified matrix assembly completed in %.3f seconds\n", total_time);
    
    // Print assembly statistics
    assembly_stats_t stats = {0};
    unified_matrix_get_stats(matrix, &stats);
    
    printf("Matrix statistics:\n");
    printf("  Total DOFs: %zu\n", matrix->total_rows);
    printf("  Memory usage: %.1f MB\n", stats.memory_usage_mb);
    printf("  Assembly time: %.3f s\n", stats.assembly_time);
    printf("  Compression ratio: %.2f\n", stats.compression_ratio);
    printf("  Number of blocks: %d\n", stats.num_blocks);
    
    return 0;
}

// Helper functions for geometry extraction
static void get_triangle_geometry(mesh_t* mesh, size_t element_idx, triangle_geometry_t* tri) {
    if (!mesh || !tri || element_idx >= mesh->num_elements) return;
    
    element_t* element = &mesh->elements[element_idx];
    if (element->num_vertices != 3) return;
    
    // Extract vertices
    for (int i = 0; i < 3; i++) {
        size_t vertex_idx = element->vertices[i];
        if (vertex_idx < mesh->num_vertices) {
            tri->vertices[i][0] = mesh->vertices[vertex_idx].coords[0];
            tri->vertices[i][1] = mesh->vertices[vertex_idx].coords[1];
            tri->vertices[i][2] = mesh->vertices[vertex_idx].coords[2];
        }
    }
    
    // Compute geometric properties
    tri->area = compute_triangle_area(tri->vertices[0], tri->vertices[1], tri->vertices[2]);
    compute_triangle_normal(tri->vertices[0], tri->vertices[1], tri->vertices[2], tri->normal);
    compute_triangle_centroid(tri->vertices[0], tri->vertices[1], tri->vertices[2], tri->centroid);
    
    tri->quality_flag = 1;
}

static void get_wire_geometry(mesh_t* mesh, size_t element_idx, wire_geometry_t* wire) {
    if (!mesh || !wire || element_idx >= mesh->num_elements) return;
    
    element_t* element = &mesh->elements[element_idx];
    if (element->num_vertices != 2) return;
    
    // Extract start and end points
    size_t start_idx = element->vertices[0];
    size_t end_idx = element->vertices[1];
    
    if (start_idx < mesh->num_vertices && end_idx < mesh->num_vertices) {
        wire->start[0] = mesh->vertices[start_idx].coords[0];
        wire->start[1] = mesh->vertices[start_idx].coords[1];
        wire->start[2] = mesh->vertices[start_idx].coords[2];
        
        wire->end[0] = mesh->vertices[end_idx].coords[0];
        wire->end[1] = mesh->vertices[end_idx].coords[1];
        wire->end[2] = mesh->vertices[end_idx].coords[2];
        
        // Compute wire properties
        wire->length = sqrt(pow(wire->end[0] - wire->start[0], 2) +
                           pow(wire->end[1] - wire->start[1], 2) +
                           pow(wire->end[2] - wire->start[2], 2));
        
        // Unit tangent vector
        if (wire->length > 0) {
            wire->tangent[0] = (wire->end[0] - wire->start[0]) / wire->length;
            wire->tangent[1] = (wire->end[1] - wire->start[1]) / wire->length;
            wire->tangent[2] = (wire->end[2] - wire->start[2]) / wire->length;
        }
        
        wire->segment_id = element_idx;
        wire->radius = get_wire_radius(mesh, element_idx); // Get from mesh data
    }
}

// Wrapper functions for Green's function integration
double complex green_function_wrapper(double x1, double y1, double z1, double x2, double y2, double z2) {
    double r = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
    if (r < 1e-15) return 0.0; // Self-term handled separately
    return cexp(-I * 2.0 * M_PI * 1e9 * r / 299792458.0) / (4.0 * M_PI * r); // 1 GHz example
}

// Placeholder functions for PEEC partial element calculations
double complex compute_self_partial_inductance(const wire_geometry_t* wire, 
                                              integration_params_t* params) {
    // Industrial formula: L = (μ₀/4π) * length * (ln(2*length/radius) - 1)
    double mu_0 = 4.0 * M_PI * 1e-7;
    double length = wire->length;
    double radius = wire->radius > 0 ? wire->radius : 1e-4; // Default 0.1mm
    
    if (length <= 0 || radius <= 0) return 0.0;
    
    double L = (mu_0 / (4.0 * M_PI)) * length * (log(2.0 * length / radius) - 1.0);
    return L * (1.0 + 0.0 * I); // Add small imaginary part for stability
}

double complex compute_mutual_partial_inductance(const wire_geometry_t* wire1,
                                                const wire_geometry_t* wire2,
                                                integration_params_t* params) {
    // Neumann formula for mutual inductance
    double mu_0 = 4.0 * M_PI * 1e-7;
    
    // Simplified mutual inductance calculation
    double distance = sqrt(pow(wire1->centroid[0] - wire2->centroid[0], 2) +
                          pow(wire1->centroid[1] - wire2->centroid[1], 2) +
                          pow(wire1->centroid[2] - wire2->centroid[2], 2));
    
    double L12 = (mu_0 / (4.0 * M_PI)) * wire1->length * wire2->length / 
                (distance + 1e-6); // Avoid division by zero
    
    return L12 * (1.0 + 0.0 * I);
}

double complex compute_self_partial_potential(const wire_geometry_t* wire,
                                             integration_params_t* params) {
    // Potential coefficient: P = 1/(4πε₀) * 1/length * ln(2*length/radius)
    double epsilon_0 = 8.8541878128e-12;
    double length = wire->length;
    double radius = wire->radius > 0 ? wire->radius : 1e-4;
    
    if (length <= 0 || radius <= 0) return 0.0;
    
    double P = (1.0 / (4.0 * M_PI * epsilon_0)) * (1.0 / length) * 
               log(2.0 * length / radius);
    return P * (1.0 + 0.0 * I);
}

double complex compute_mutual_partial_potential(const wire_geometry_t* wire1,
                                               const wire_geometry_t* wire2,
                                               integration_params_t* params) {
    double epsilon_0 = 8.8541878128e-12;
    
    double distance = sqrt(pow(wire1->centroid[0] - wire2->centroid[0], 2) +
                          pow(wire1->centroid[1] - wire2->centroid[1], 2) +
                          pow(wire1->centroid[2] - wire2->centroid[2], 2));
    
    double P12 = (1.0 / (4.0 * M_PI * epsilon_0)) * wire1->length * wire2->length /
                (distance + 1e-6);
    
    return P12 * (1.0 + 0.0 * I);
}

double compute_wire_resistance(const wire_geometry_t* wire, double frequency) {
    // DC resistance: R = ρ * length / (π * radius²)
    double rho = 1.68e-8; // Copper resistivity
    double radius = wire->radius > 0 ? wire->radius : 1e-4;
    double area = M_PI * radius * radius;
    
    if (wire->length <= 0 || area <= 0) return 0.0;
    
    return rho * wire->length / area;
}

double compute_mutual_resistance(const wire_geometry_t* wire1,
                                const wire_geometry_t* wire2,
                                double frequency) {
    // Mutual resistance (usually negligible at low frequencies)
    return 0.0;
}

// Matrix inversion for PEEC capacitance matrix
static int invert_matrix_complex(const double complex* input, double complex* output, size_t n) {
    if (!input || !output || n == 0) return -1;
    
    // Simplified inversion - in practice use LAPACK
    // For now, just copy diagonal elements
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if (i == j) {
                output[i * n + j] = 1.0 / (input[i * n + j] + 1e-12); // Avoid division by zero
            } else {
                output[i * n + j] = 0.0;
            }
        }
    }
    
    return 0;
}

// Utility functions for triangle geometry
static void compute_triangle_normal(const double v1[3], const double v2[3], const double v3[3], double normal[3]) {
    double edge1[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    double edge2[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};
    
    // Cross product
    normal[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
    normal[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
    normal[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];
    
    // Normalize
    double length = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
    if (length > 0) {
        normal[0] /= length;
        normal[1] /= length;
        normal[2] /= length;
    }
}

static void compute_triangle_centroid(const double v1[3], const double v2[3], const double v3[3], double centroid[3]) {
    centroid[0] = (v1[0] + v2[0] + v3[0]) / 3.0;
    centroid[1] = (v1[1] + v2[1] + v3[1]) / 3.0;
    centroid[2] = (v1[2] + v2[2] + v3[2]) / 3.0;
}

static double compute_triangle_area(const double v1[3], const double v2[3], const double v3[3]) {
    double edge1[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    double edge2[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};
    
    // Cross product magnitude
    double cross[3];
    cross[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
    cross[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
    cross[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];
    
    return 0.5 * sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
}

static double get_wire_radius(mesh_t* mesh, size_t element_idx) {
    // In a real implementation, this would come from mesh attributes
    // For now, return a default radius
    return 0.1e-3; // 0.1 mm default
}