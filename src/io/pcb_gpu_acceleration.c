/******************************************************************************
 * PCB GPU Acceleration Implementation
 * 
 * This file implements GPU-accelerated kernels specifically optimized for
 * PCB electromagnetic calculations, including:
 * - Layer geometry processing
 * - Triangle mesh operations
 * - Impedance matrix assembly
 * - S-parameter extraction
 * - Multi-layer coupling analysis
 * - Frequency sweep optimization
 ******************************************************************************/

#include "pcb_gpu_acceleration.h"
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cuComplex.h>
#include <stdio.h>
#include <math.h>

#define BLOCK_SIZE 256
#define WARP_SIZE 32
#define MAX_LAYERS 32
#define MAX_TRIANGLES_PER_BLOCK 64

/******************************************************************************
 * PCB Layer Geometry Processing Kernels
 ******************************************************************************/

__global__ void pcb_layer_geometry_kernel(
    const double* layer_vertices, const int* layer_triangles,
    const int* layer_offsets, const double* layer_thickness,
    const double* layer_conductivity, const double* layer_permittivity,
    double* processed_geometry, int num_layers, int total_triangles) {
    
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= total_triangles) return;
    
    // Find which layer this triangle belongs to
    int layer_idx = 0;
    for (int i = 0; i < num_layers; i++) {
        if (tid < layer_offsets[i + 1]) {
            layer_idx = i;
            break;
        }
    }
    
    int triangle_idx = tid - layer_offsets[layer_idx];
    int vertex_offset = layer_triangles[tid * 3];
    
    // Process triangle geometry for PCB layer
    double3 v0, v1, v2;
    v0.x = layer_vertices[vertex_offset * 3];
    v0.y = layer_vertices[vertex_offset * 3 + 1];
    v0.z = layer_vertices[vertex_offset * 3 + 2];
    
    v1.x = layer_vertices[(vertex_offset + 1) * 3];
    v1.y = layer_vertices[(vertex_offset + 1) * 3 + 1];
    v1.z = layer_vertices[(vertex_offset + 1) * 3 + 2];
    
    v2.x = layer_vertices[(vertex_offset + 2) * 3];
    v2.y = layer_vertices[(vertex_offset + 2) * 3 + 1];
    v2.z = layer_vertices[(vertex_offset + 2) * 3 + 2];
    
    // Calculate triangle normal and area for PCB layer
    double3 edge1 = make_double3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
    double3 edge2 = make_double3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
    
    double3 normal;
    normal.x = edge1.y * edge2.z - edge1.z * edge2.y;
    normal.y = edge1.z * edge2.x - edge1.x * edge2.z;
    normal.z = edge1.x * edge2.y - edge1.y * edge2.x;
    
    double area = 0.5 * sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    
    // Store processed geometry data
    processed_geometry[tid * 8] = area;
    processed_geometry[tid * 8 + 1] = normal.x;
    processed_geometry[tid * 8 + 2] = normal.y;
    processed_geometry[tid * 8 + 3] = normal.z;
    processed_geometry[tid * 8 + 4] = layer_thickness[layer_idx];
    processed_geometry[tid * 8 + 5] = layer_conductivity[layer_idx];
    processed_geometry[tid * 8 + 6] = layer_permittivity[layer_idx];
    processed_geometry[tid * 8 + 7] = (double)layer_idx;
}

__global__ void pcb_triangle_area_kernel(
    const double* vertices, const int* triangles,
    double* areas, int num_triangles) {
    
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_triangles) return;
    
    int tri_offset = tid * 3;
    int v0_idx = triangles[tri_offset] * 3;
    int v1_idx = triangles[tri_offset + 1] * 3;
    int v2_idx = triangles[tri_offset + 2] * 3;
    
    // Get triangle vertices
    double3 v0 = make_double3(vertices[v0_idx], vertices[v0_idx + 1], vertices[v0_idx + 2]);
    double3 v1 = make_double3(vertices[v1_idx], vertices[v1_idx + 1], vertices[v1_idx + 2]);
    double3 v2 = make_double3(vertices[v2_idx], vertices[v2_idx + 1], vertices[v2_idx + 2]);
    
    // Calculate triangle area using cross product
    double3 u = make_double3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
    double3 v = make_double3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
    
    double3 cross;
    cross.x = u.y * v.z - u.z * v.y;
    cross.y = u.z * v.x - u.x * v.z;
    cross.z = u.x * v.y - u.y * v.x;
    
    areas[tid] = 0.5 * sqrt(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
}

/******************************************************************************
 * PCB Edge Detection and Via Processing Kernels
 ******************************************************************************/

__global__ void pcb_edge_detection_kernel(
    const double* vertices, const int* triangles, const int* triangle_neighbors,
    int* edge_flags, int num_triangles) {
    
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_triangles) return;
    
    int tri_offset = tid * 3;
    int neighbor_offset = tid * 3;
    
    // Check each edge of the triangle
    for (int edge = 0; edge < 3; edge++) {
        int neighbor_tri = triangle_neighbors[neighbor_offset + edge];
        
        // If neighbor is -1 or has different material properties, mark as edge
        if (neighbor_tri == -1) {
            edge_flags[tri_offset + edge] = 1;
        } else {
            // Additional checks for PCB-specific edge detection
            edge_flags[tri_offset + edge] = 0;
        }
    }
}

__global__ void pcb_via_connection_kernel(
    const double* via_positions, const double* via_radii,
    const double* layer_elevations, const int* via_connections,
    cuDoubleComplex* via_impedances, int num_vias, int num_layers) {
    
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_vias) return;
    
    double via_x = via_positions[tid * 3];
    double via_y = via_positions[tid * 3 + 1];
    double via_z = via_positions[tid * 3 + 2];
    double radius = via_radii[tid];
    
    // Calculate via inductance and capacitance
    double via_length = 0.0;
    int start_layer = -1, end_layer = -1;
    
    // Find via connection layers
    for (int i = 0; i < num_layers - 1; i++) {
        if (via_connections[tid * num_layers + i] == 1) {
            if (start_layer == -1) start_layer = i;
            end_layer = i + 1;
            via_length += fabs(layer_elevations[i + 1] - layer_elevations[i]);
        }
    }
    
    if (start_layer == -1 || end_layer == -1) return;
    
    // Calculate via inductance (simplified model)
    double mu0 = 4.0 * M_PI * 1e-7;
    double via_inductance = (mu0 * via_length) / (2.0 * M_PI) * log(via_length / radius + sqrt(1.0 + (via_length * via_length) / (radius * radius)));
    
    // Calculate via capacitance (simplified model)
    double epsilon0 = 8.854e-12;
    double via_capacitance = epsilon0 * M_PI * radius * radius / via_length;
    
    // Store via impedance
    via_impedances[tid].x = via_inductance;
    via_impedances[tid].y = 1.0 / (2.0 * M_PI * 1e9 * via_capacitance); // At 1GHz
}

/******************************************************************************
 * PCB Layered Green's Function Kernels
 ******************************************************************************/

__global__ void pcb_layered_green_function_kernel(
    const double* source_points, const double* field_points,
    const double* layer_elevations, const double* layer_thickness,
    const double* layer_conductivity, const double* layer_permittivity,
    const double frequency, cuDoubleComplex* green_function,
    int num_sources, int num_fields, int num_layers) {
    
    int source_idx = blockIdx.x * blockDim.x + threadIdx.x;
    int field_idx = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (source_idx >= num_sources || field_idx >= num_fields) return;
    
    int source_offset = source_idx * 3;
    int field_offset = field_idx * 3;
    int result_offset = source_idx * num_fields + field_idx;
    
    double sx = source_points[source_offset];
    double sy = source_points[source_offset + 1];
    double sz = source_points[source_offset + 2];
    
    double fx = field_points[field_offset];
    double fy = field_points[field_offset + 1];
    double fz = field_points[field_offset + 2];
    
    double dx = fx - sx;
    double dy = fy - sy;
    double dz = fz - sz;
    double r = sqrt(dx * dx + dy * dy + dz * dz);
    
    double omega = 2.0 * M_PI * frequency;
    double k0 = omega / 3e8; // Free space wave number
    
    // Find source and field layers
    int source_layer = 0, field_layer = 0;
    for (int i = 0; i < num_layers; i++) {
        if (sz >= layer_elevations[i] && sz < layer_elevations[i] + layer_thickness[i]) {
            source_layer = i;
        }
        if (fz >= layer_elevations[i] && fz < layer_elevations[i] + layer_thickness[i]) {
            field_layer = i;
        }
    }
    
    // Calculate layered Green's function for PCB
    cuDoubleComplex result;
    
    if (source_layer == field_layer) {
        // Same layer - direct term + reflected term
        double k = k0 * sqrt(layer_permittivity[source_layer]);
        double alpha = layer_conductivity[source_layer] * 377.0 / (2.0 * k0);
        
        double complex_k = k + I * alpha;
        cuDoubleComplex exp_term;
        exp_term.x = cos(-creal(complex_k) * r);
        exp_term.y = sin(-creal(complex_k) * r);
        
        double attenuation = exp(-cimag(complex_k) * r);
        result.x = attenuation * exp_term.x / (4.0 * M_PI * r);
        result.y = attenuation * exp_term.y / (4.0 * M_PI * r);
    } else {
        // Different layers - transmission term
        double transmission_coeff = 1.0;
        
        // Simple transmission model (can be improved)
        for (int i = min(source_layer, field_layer); i < max(source_layer, field_layer); i++) {
            double eta1 = sqrt(layer_permittivity[i]);
            double eta2 = sqrt(layer_permittivity[i + 1]);
            transmission_coeff *= (2.0 * eta1) / (eta1 + eta2);
        }
        
        result.x = transmission_coeff * cos(k0 * r) / (4.0 * M_PI * r);
        result.y = transmission_coeff * sin(k0 * r) / (4.0 * M_PI * r);
    }
    
    green_function[result_offset] = result;
}

/******************************************************************************
 * PCB Impedance Matrix Assembly Kernels
 ******************************************************************************/

__global__ void pcb_impedance_matrix_assembly_kernel(
    const cuDoubleComplex* green_function_matrix,
    const double* basis_functions, const double* test_functions,
    const double* triangle_areas, const int* triangle_layers,
    cuDoubleComplex* impedance_matrix, int num_triangles) {
    
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    int col = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (row >= num_triangles || col >= num_triangles) return;
    
    int matrix_offset = row * num_triangles + col;
    
    // Check if triangles are on compatible layers (simplified)
    if (abs(triangle_layers[row] - triangle_layers[col]) > 1) {
        impedance_matrix[matrix_offset].x = 0.0;
        impedance_matrix[matrix_offset].y = 0.0;
        return;
    }
    
    // Integrate Green's function with basis and test functions
    double area_row = triangle_areas[row];
    double area_col = triangle_areas[col];
    
    // Simplified integration (can be improved with numerical quadrature)
    cuDoubleComplex green_val = green_function_matrix[matrix_offset];
    
    double integration_weight = area_row * area_col;
    
    impedance_matrix[matrix_offset].x = green_val.x * integration_weight;
    impedance_matrix[matrix_offset].y = green_val.y * integration_weight;
}

/******************************************************************************
 * PCB S-Parameter Extraction Kernels
 ******************************************************************************/

__global__ void pcb_sparameter_extraction_kernel(
    const cuDoubleComplex* impedance_matrix,
    const cuDoubleComplex* port_excitations,
    const int* port_triangles, const int* num_port_triangles,
    cuDoubleComplex* s_parameters, int num_ports, int frequency_idx) {
    
    int port_i = blockIdx.x * blockDim.x + threadIdx.x;
    int port_j = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (port_i >= num_ports || port_j >= num_ports) return;
    
    int sparam_offset = frequency_idx * num_ports * num_ports + port_i * num_ports + port_j;
    
    // Extract S-parameter using MoM solution
    cuDoubleComplex numerator = make_cuDoubleComplex(0.0, 0.0);
    cuDoubleComplex denominator = make_cuDoubleComplex(0.0, 0.0);
    
    // Sum over port triangles
    for (int i = 0; i < num_port_triangles[port_i]; i++) {
        for (int j = 0; j < num_port_triangles[port_j]; j++) {
            int tri_i = port_triangles[port_i * MAX_TRIANGLES_PER_BLOCK + i];
            int tri_j = port_triangles[port_j * MAX_TRIANGLES_PER_BLOCK + j];
            
            if (tri_i >= 0 && tri_j >= 0) {
                int z_offset = tri_i * gridDim.x * blockDim.x + tri_j;
                cuDoubleComplex z_val = impedance_matrix[z_offset];
                cuDoubleComplex exc_i = port_excitations[port_i * MAX_TRIANGLES_PER_BLOCK + i];
                cuDoubleComplex exc_j = port_excitations[port_j * MAX_TRIANGLES_PER_BLOCK + j];
                
                cuDoubleComplex term;
                term.x = z_val.x * exc_i.x - z_val.y * exc_i.y;
                term.y = z_val.x * exc_i.y + z_val.y * exc_i.x;
                
                numerator.x += term.x * exc_j.x - term.y * exc_j.y;
                numerator.y += term.x * exc_j.y + term.y * exc_j.x;
                
                denominator.x += exc_i.x * exc_i.x + exc_i.y * exc_i.y;
                denominator.y += 0.0;
            }
        }
    }
    
    // Calculate S-parameter
    double denom_mag = denominator.x * denominator.x + denominator.y * denominator.y;
    if (denom_mag > 0.0) {
        s_parameters[sparam_offset].x = (numerator.x * denominator.x + numerator.y * denominator.y) / denom_mag;
        s_parameters[sparam_offset].y = (numerator.y * denominator.x - numerator.x * denominator.y) / denom_mag;
    } else {
        s_parameters[sparam_offset].x = 0.0;
        s_parameters[sparam_offset].y = 0.0;
    }
}

/******************************************************************************
 * PCB Current Distribution Kernels
 ******************************************************************************/

__global__ void pcb_current_distribution_kernel(
    const cuDoubleComplex* impedance_matrix,
    const cuDoubleComplex* voltage_vector,
    cuDoubleComplex* current_distribution, int num_triangles) {
    
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_triangles) return;
    
    // Solve Z * I = V for current distribution
    // This is a simplified implementation - real implementation would use
    // a sparse linear solver
    
    cuDoubleComplex sum = make_cuDoubleComplex(0.0, 0.0);
    
    for (int j = 0; j < num_triangles; j++) {
        int z_offset = tid * num_triangles + j;
        cuDoubleComplex z_val = impedance_matrix[z_offset];
        cuDoubleComplex v_val = voltage_vector[j];
        
        // Accumulate Z * I product (simplified)
        sum.x += z_val.x * v_val.x - z_val.y * v_val.y;
        sum.y += z_val.x * v_val.y + z_val.y * v_val.x;
    }
    
    // Simplified current calculation (should be I = Z^(-1) * V)
    double mag = sqrt(sum.x * sum.x + sum.y * sum.y);
    if (mag > 0.0) {
        current_distribution[tid].x = (sum.x * voltage_vector[tid].x + sum.y * voltage_vector[tid].y) / mag;
        current_distribution[tid].y = (sum.y * voltage_vector[tid].x - sum.x * voltage_vector[tid].y) / mag;
    } else {
        current_distribution[tid].x = 0.0;
        current_distribution[tid].y = 0.0;
    }
}

/******************************************************************************
 * PCB Multi-layer Coupling Kernels
 ******************************************************************************/

__global__ void pcb_multilayer_coupling_kernel(
    const cuDoubleComplex* layer_currents, const double* layer_positions,
    const double* layer_thickness, const double frequency,
    cuDoubleComplex* coupling_matrix, int num_layers) {
    
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (i >= num_layers || j >= num_layers) return;
    
    int offset = i * num_layers + j;
    
    if (i == j) {
        // Self coupling
        coupling_matrix[offset].x = 1.0;
        coupling_matrix[offset].y = 0.0;
        return;
    }
    
    // Calculate mutual coupling between layers
    double distance = fabs(layer_positions[i] - layer_positions[j]);
    double avg_thickness = 0.5 * (layer_thickness[i] + layer_thickness[j]);
    
    double omega = 2.0 * M_PI * frequency;
    double k0 = omega / 3e8;
    
    // Simplified coupling model
    double coupling_strength = exp(-distance / avg_thickness);
    double phase_shift = k0 * distance;
    
    coupling_matrix[offset].x = coupling_strength * cos(phase_shift);
    coupling_matrix[offset].y = coupling_strength * sin(phase_shift);
}

/******************************************************************************
 * PCB Frequency Sweep Optimization Kernels
 ******************************************************************************/

__global__ void pcb_frequency_sweep_kernel(
    const double* frequencies, int num_frequencies,
    const double* pcb_geometry, const double* material_properties,
    cuDoubleComplex* frequency_response, int geometry_size) {
    
    int freq_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (freq_idx >= num_frequencies) return;
    
    double frequency = frequencies[freq_idx];
    double omega = 2.0 * M_PI * frequency;
    
    // Calculate frequency-dependent material properties
    double conductivity = material_properties[0];
    double permittivity = material_properties[1];
    double permeability = material_properties[2];
    double loss_tangent = material_properties[3];
    
    // Frequency-dependent conductivity (simplified model)
    double freq_conductivity = conductivity * (1.0 + 0.1 * log(frequency / 1e9));
    
    // Frequency-dependent permittivity (Debye model)
    double freq_permittivity = permittivity - (permittivity - 1.0) / (1.0 + (omega * 1e-12) * (omega * 1e-12));
    
    // Calculate complex propagation constant
    double complex_permittivity = freq_permittivity - I * freq_conductivity / (omega * 8.854e-12);
    double propagation_constant = omega * sqrt(permeability * 4e-7 * M_PI * complex_permittivity * 8.854e-12);
    
    // Store frequency response
    int response_offset = freq_idx * geometry_size;
    for (int i = 0; i < geometry_size; i++) {
        double geometric_factor = pcb_geometry[i];
        
        frequency_response[response_offset + i].x = geometric_factor * creal(propagation_constant);
        frequency_response[response_offset + i].y = geometric_factor * cimag(propagation_constant);
    }
}

/******************************************************************************
 * Host Interface Functions
 ******************************************************************************/

int launch_pcb_layer_geometry_processing(
    const double* layer_vertices, const int* layer_triangles,
    const int* layer_offsets, const double* layer_thickness,
    const double* layer_conductivity, const double* layer_permittivity,
    double* processed_geometry, int num_layers, int total_triangles) {
    
    // Allocate device memory
    double *d_vertices, *d_thickness, *d_conductivity, *d_permittivity, *d_processed_geometry;
    int *d_triangles, *d_offsets;
    
    size_t vertex_size = total_triangles * 9 * sizeof(double);
    size_t triangle_size = total_triangles * 3 * sizeof(int);
    size_t layer_size = (num_layers + 1) * sizeof(int);
    size_t processed_size = total_triangles * 8 * sizeof(double);
    
    cudaMalloc(&d_vertices, vertex_size);
    cudaMalloc(&d_triangles, triangle_size);
    cudaMalloc(&d_offsets, layer_size);
    cudaMalloc(&d_thickness, num_layers * sizeof(double));
    cudaMalloc(&d_conductivity, num_layers * sizeof(double));
    cudaMalloc(&d_permittivity, num_layers * sizeof(double));
    cudaMalloc(&d_processed_geometry, processed_size);
    
    // Copy data to device
    cudaMemcpy(d_vertices, layer_vertices, vertex_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_triangles, layer_triangles, triangle_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_offsets, layer_offsets, layer_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_thickness, layer_thickness, num_layers * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_conductivity, layer_conductivity, num_layers * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_permittivity, layer_permittivity, num_layers * sizeof(double), cudaMemcpyHostToDevice);
    
    // Launch kernel
    int block_size = 256;
    int grid_size = (total_triangles + block_size - 1) / block_size;
    
    pcb_layer_geometry_kernel<<<grid_size, block_size>>>(
        d_vertices, d_triangles, d_offsets, d_thickness,
        d_conductivity, d_permittivity, d_processed_geometry,
        num_layers, total_triangles);
    
    // Copy result back
    cudaMemcpy(processed_geometry, d_processed_geometry, processed_size, cudaMemcpyDeviceToHost);
    
    // Cleanup
    cudaFree(d_vertices);
    cudaFree(d_triangles);
    cudaFree(d_offsets);
    cudaFree(d_thickness);
    cudaFree(d_conductivity);
    cudaFree(d_permittivity);
    cudaFree(d_processed_geometry);
    
    return 0;
}

int launch_pcb_impedance_matrix_assembly(
    const cuDoubleComplex* green_function_matrix,
    const double* basis_functions, const double* test_functions,
    const double* triangle_areas, const int* triangle_layers,
    cuDoubleComplex* impedance_matrix, int num_triangles) {
    
    // Allocate device memory
    cuDoubleComplex *d_green_matrix, *d_impedance_matrix;
    double *d_basis_functions, *d_test_functions, *d_areas;
    int *d_triangle_layers;
    
    size_t matrix_size = num_triangles * num_triangles * sizeof(cuDoubleComplex);
    size_t function_size = num_triangles * sizeof(double);
    
    cudaMalloc(&d_green_matrix, matrix_size);
    cudaMalloc(&d_basis_functions, function_size);
    cudaMalloc(&d_test_functions, function_size);
    cudaMalloc(&d_areas, function_size);
    cudaMalloc(&d_triangle_layers, num_triangles * sizeof(int));
    cudaMalloc(&d_impedance_matrix, matrix_size);
    
    // Copy data to device
    cudaMemcpy(d_green_matrix, green_function_matrix, matrix_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_basis_functions, basis_functions, function_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_test_functions, test_functions, function_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_areas, triangle_areas, function_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_triangle_layers, triangle_layers, num_triangles * sizeof(int), cudaMemcpyHostToDevice);
    
    // Launch kernel with 2D grid
    dim3 block_size(16, 16);
    dim3 grid_size((num_triangles + block_size.x - 1) / block_size.x,
                   (num_triangles + block_size.y - 1) / block_size.y);
    
    pcb_impedance_matrix_assembly_kernel<<<grid_size, block_size>>>(
        d_green_matrix, d_basis_functions, d_test_functions,
        d_areas, d_triangle_layers, d_impedance_matrix, num_triangles);
    
    // Copy result back
    cudaMemcpy(impedance_matrix, d_impedance_matrix, matrix_size, cudaMemcpyDeviceToHost);
    
    // Cleanup
    cudaFree(d_green_matrix);
    cudaFree(d_basis_functions);
    cudaFree(d_test_functions);
    cudaFree(d_areas);
    cudaFree(d_triangle_layers);
    cudaFree(d_impedance_matrix);
    
    return 0;
}

int launch_pcb_sparameter_extraction(
    const cuDoubleComplex* impedance_matrix,
    const cuDoubleComplex* port_excitations,
    const int* port_triangles, const int* num_port_triangles,
    cuDoubleComplex* s_parameters, int num_ports, int num_frequencies) {
    
    // Allocate device memory
    cuDoubleComplex *d_impedance_matrix, *d_port_excitations, *d_s_parameters;
    int *d_port_triangles, *d_num_port_triangles;
    
    size_t matrix_size = num_ports * MAX_TRIANGLES_PER_BLOCK * num_ports * MAX_TRIANGLES_PER_BLOCK * sizeof(cuDoubleComplex);
    size_t excitation_size = num_ports * MAX_TRIANGLES_PER_BLOCK * sizeof(cuDoubleComplex);
    size_t sparam_size = num_frequencies * num_ports * num_ports * sizeof(cuDoubleComplex);
    
    cudaMalloc(&d_impedance_matrix, matrix_size);
    cudaMalloc(&d_port_excitations, excitation_size);
    cudaMalloc(&d_port_triangles, num_ports * MAX_TRIANGLES_PER_BLOCK * sizeof(int));
    cudaMalloc(&d_num_port_triangles, num_ports * sizeof(int));
    cudaMalloc(&d_s_parameters, sparam_size);
    
    // Copy data to device
    cudaMemcpy(d_impedance_matrix, impedance_matrix, matrix_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_port_excitations, port_excitations, excitation_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_port_triangles, port_triangles, num_ports * MAX_TRIANGLES_PER_BLOCK * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_num_port_triangles, num_port_triangles, num_ports * sizeof(int), cudaMemcpyHostToDevice);
    
    // Launch kernel for each frequency
    dim3 block_size(8, 8);
    dim3 grid_size((num_ports + block_size.x - 1) / block_size.x,
                   (num_ports + block_size.y - 1) / block_size.y);
    
    for (int freq = 0; freq < num_frequencies; freq++) {
        pcb_sparameter_extraction_kernel<<<grid_size, block_size>>>(
            d_impedance_matrix, d_port_excitations,
            d_port_triangles, d_num_port_triangles,
            d_s_parameters, num_ports, freq);
    }
    
    // Copy result back
    cudaMemcpy(s_parameters, d_s_parameters, sparam_size, cudaMemcpyDeviceToHost);
    
    // Cleanup
    cudaFree(d_impedance_matrix);
    cudaFree(d_port_excitations);
    cudaFree(d_port_triangles);
    cudaFree(d_num_port_triangles);
    cudaFree(d_s_parameters);
    
    return 0;
}

/******************************************************************************
 * PCB GPU Memory Management Functions
 ******************************************************************************/

PCBGPUContext* create_pcb_gpu_context(int max_triangles, int max_ports, int max_frequencies) {
    PCBGPUContext* context = (PCBGPUContext*)malloc(sizeof(PCBGPUContext));
    
    context->max_triangles = max_triangles;
    context->max_ports = max_ports;
    context->max_frequencies = max_frequencies;
    
    // Allocate GPU memory pools
    size_t triangle_size = max_triangles * sizeof(double) * 3;
    size_t matrix_size = max_triangles * max_triangles * sizeof(cuDoubleComplex);
    size_t sparam_size = max_ports * max_ports * max_frequencies * sizeof(cuDoubleComplex);
    
    cudaMalloc(&context->d_triangle_vertices, triangle_size);
    cudaMalloc(&context->d_impedance_matrix, matrix_size);
    cudaMalloc(&context->d_s_parameters, sparam_size);
    
    // Create cuBLAS handle
    cublasCreate(&context->cublas_handle);
    
    return context;
}

void destroy_pcb_gpu_context(PCBGPUContext* context) {
    if (context) {
        cudaFree(context->d_triangle_vertices);
        cudaFree(context->d_impedance_matrix);
        cudaFree(context->d_s_parameters);
        
        cublasDestroy(context->cublas_handle);
        
        free(context);
    }
}

int optimize_pcb_gpu_performance(PCBGPUContext* context) {
    // Set optimal CUDA device parameters
    cudaDeviceSetCacheConfig(cudaFuncCachePreferShared);
    cudaDeviceSetSharedMemConfig(cudaSharedMemBankSizeEightByte);
    
    // Optimize memory access patterns
    size_t total_mem, free_mem;
    cudaMemGetInfo(&free_mem, &total_mem);
    
    // Adjust batch sizes based on available memory
    if (free_mem > 4ULL * 1024 * 1024 * 1024) { // > 4GB
        context->optimal_batch_size = 1024;
    } else if (free_mem > 2ULL * 1024 * 1024 * 1024) { // > 2GB
        context->optimal_batch_size = 512;
    } else {
        context->optimal_batch_size = 256;
    }
    
    return 0;
}