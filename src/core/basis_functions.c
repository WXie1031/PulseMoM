#include "basis_functions.h"
#include "layered_greens_function.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BASIS_FUNCTIONS 100000
#define ROOFTOP_THRESHOLD 0.1  // Threshold for choosing rooftop vs RWG
#define EDGE_ENHANCEMENT_FACTOR 1.5

// Helper function: Calculate triangle area
double triangle_area(const Triangle *tri) {
    double v1x = tri->x[1] - tri->x[0];
    double v1y = tri->y[1] - tri->y[0];
    double v1z = tri->z[1] - tri->z[0];
    
    double v2x = tri->x[2] - tri->x[0];
    double v2y = tri->y[2] - tri->y[0];
    double v2z = tri->z[2] - tri->z[0];
    
    double cross_x = v1y * v2z - v1z * v2y;
    double cross_y = v1z * v2x - v1x * v2z;
    double cross_z = v1x * v2y - v1y * v2x;
    
    return 0.5 * sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
}

// Helper function: Calculate edge length
double edge_length(const Triangle *tri, int edge) {
    int i1 = edge;
    int i2 = (edge + 1) % 3;
    
    double dx = tri->x[i2] - tri->x[i1];
    double dy = tri->y[i2] - tri->y[i1];
    double dz = tri->z[i2] - tri->z[i1];
    
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// RWG basis function creation
void create_rwg_basis_functions(
    MeshData *mesh,
    const Triangle *triangles,
    int n_triangles,
    const LayeredMedium *medium) {
    
    mesh->triangles = (Triangle*)malloc(n_triangles * sizeof(Triangle));
    memcpy(mesh->triangles, triangles, n_triangles * sizeof(Triangle));
    mesh->n_triangles = n_triangles;
    
    // Count edges (simplified - in practice use edge connectivity)
    mesh->n_edges = 3 * n_triangles; // Overestimate
    mesh->edge_to_triangle = (int*)malloc(mesh->n_edges * 2 * sizeof(int));
    mesh->triangle_to_edges = (int*)malloc(3 * n_triangles * sizeof(int));
    
    // Create edge connectivity
    int edge_count = 0;
    for (int i = 0; i < n_triangles; i++) {
        for (int j = 0; j < 3; j++) {
            mesh->triangle_to_edges[3 * i + j] = edge_count;
            mesh->edge_to_triangle[2 * edge_count] = i;
            mesh->edge_to_triangle[2 * edge_count + 1] = -1; // No neighbor yet
            edge_count++;
        }
    }
    
    // Allocate basis functions
    mesh->basis_functions = (BasisFunction*)malloc(mesh->n_edges * sizeof(BasisFunction));
    mesh->n_basis_functions = 0;
    
    // Create RWG basis functions
    for (int edge = 0; edge < edge_count; edge++) {
        int tri_plus = mesh->edge_to_triangle[2 * edge];
        int tri_minus = mesh->edge_to_triangle[2 * edge + 1];
        
        if (tri_minus >= 0) { // Interior edge
            BasisFunction *bf = &mesh->basis_functions[mesh->n_basis_functions];
            bf->type = 0; // RWG
            bf->edge_index = edge;
            bf->area = triangle_area(&mesh->triangles[tri_plus]) + 
                      triangle_area(&mesh->triangles[tri_minus]);
            bf->n_coeffs = 1;
            bf->coefficients = (double*)malloc(sizeof(double));
            bf->coefficients[0] = 1.0; // Normalization
            
            mesh->n_basis_functions++;
        }
    }
}

// Rooftop basis function creation
void create_rooftop_basis_functions(
    MeshData *mesh,
    const RooftopGrid *grid,
    const LayeredMedium *medium) {
    
    // Allocate rooftop grids
    mesh->rooftop_grids = (RooftopGrid*)malloc(sizeof(RooftopGrid));
    memcpy(mesh->rooftop_grids, grid, sizeof(RooftopGrid));
    mesh->n_rooftop_grids = 1;
    
    // Calculate number of rooftop functions
    int nx = grid->nx;
    int ny = grid->ny;
    int n_rooftop_x = (nx - 1) * ny; // x-directed rooftops
    int n_rooftop_y = nx * (ny - 1); // y-directed rooftops
    int total_rooftop = n_rooftop_x + n_rooftop_y;
    
    // Reallocate basis functions array
    int current_n_bf = mesh->n_basis_functions;
    mesh->basis_functions = (BasisFunction*)realloc(mesh->basis_functions,
                                                 (current_n_bf + total_rooftop) * sizeof(BasisFunction));
    
    // Create x-directed rooftop functions
    for (int i = 0; i < nx - 1; i++) {
        for (int j = 0; j < ny; j++) {
            BasisFunction *bf = &mesh->basis_functions[current_n_bf];
            bf->type = 1; // Rooftop
            bf->cell_index = i * ny + j;
            bf->direction = 0; // x-direction
            bf->area = grid->dx * grid->dy;
            bf->n_coeffs = 1;
            bf->coefficients = (double*)malloc(sizeof(double));
            bf->coefficients[0] = 1.0 / grid->dx; // Normalization
            
            current_n_bf++;
        }
    }
    
    // Create y-directed rooftop functions
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny - 1; j++) {
            BasisFunction *bf = &mesh->basis_functions[current_n_bf];
            bf->type = 1; // Rooftop
            bf->cell_index = i * ny + j;
            bf->direction = 1; // y-direction
            bf->area = grid->dx * grid->dy;
            bf->n_coeffs = 1;
            bf->coefficients = (double*)malloc(sizeof(double));
            bf->coefficients[0] = 1.0 / grid->dy; // Normalization
            
            current_n_bf++;
        }
    }
    
    mesh->n_basis_functions = current_n_bf;
}

// Mixed basis function approach (optimal for PCB)
void create_mixed_basis_functions(
    MeshData *mesh,
    const Triangle *triangles,
    int n_triangles,
    const RooftopGrid *rooftop_grids,
    int n_rooftop_grids,
    const LayeredMedium *medium) {
    
    // Initialize mesh data
    memset(mesh, 0, sizeof(MeshData));
    
    // Create RWG basis functions for irregular geometries
    create_rwg_basis_functions(mesh, triangles, n_triangles, medium);
    
    // Create rooftop basis functions for regular geometries
    for (int i = 0; i < n_rooftop_grids; i++) {
        create_rooftop_basis_functions(mesh, &rooftop_grids[i], medium);
    }
    
    // Optimize basis function selection based on geometry
    for (int i = 0; i < mesh->n_basis_functions; i++) {
        BasisFunction *bf = &mesh->basis_functions[i];
        
        // Apply selection criteria based on geometry regularity
        if (bf->type == 0 && bf->area < ROOFTOP_THRESHOLD) {
            // Consider converting small RWG to rooftop if possible
            // This would require geometric analysis
        }
    }
}

// Basis function evaluation
double complex evaluate_basis_function(
    const BasisFunction *bf,
    double x, double y, double z,
    int component) {
    
    if (bf->type == 0) { // RWG
        // Simplified RWG evaluation
        // In practice, implement proper triangle interpolation
        return (component < 2) ? bf->coefficients[0] : 0.0;
    } else if (bf->type == 1) { // Rooftop
        // Rooftop function evaluation
        if (component == bf->direction) {
            // Linear interpolation over rooftop support
            return bf->coefficients[0];
        }
    }
    
    return 0.0 + 0.0 * I;
}

// Surface current expansion
CurrentExpansion* expand_surface_current(
    const MeshData *mesh,
    const double complex *current_values,
    const PortDefinition *ports,
    int n_ports) {
    
    CurrentExpansion *exp = (CurrentExpansion*)malloc(sizeof(CurrentExpansion));
    exp->n_coefficients = mesh->n_basis_functions;
    exp->coefficients = (double complex*)malloc(exp->n_coefficients * sizeof(double complex));
    
    // Simple expansion - in practice, solve MoM system
    for (int i = 0; i < mesh->n_basis_functions; i++) {
        exp->coefficients[i] = current_values[i]; // Placeholder
    }
    
    return exp;
}

// Impedance matrix assembly
NetworkParameters* assemble_impedance_matrix(
    const MeshData *mesh,
    const GreensFunctionDyadic *greens_func,
    const FrequencyDomain *freq,
    const PortDefinition *ports,
    int n_ports) {
    
    NetworkParameters *net = (NetworkParameters*)malloc(sizeof(NetworkParameters));
    net->frequency = freq->freq;
    net->matrix_size = mesh->n_basis_functions;
    
    int matrix_size = net->matrix_size;
    net->Z_matrix = (double complex*)malloc(matrix_size * matrix_size * sizeof(double complex));
    net->Y_matrix = (double complex*)malloc(matrix_size * matrix_size * sizeof(double complex));
    net->S_matrix = (double complex*)malloc(matrix_size * matrix_size * sizeof(double complex));
    
    // Assemble impedance matrix using Green's function
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            double complex Z_ij = 0.0 + 0.0 * I;
            
            // Integrate Green's function over basis function supports
            // This is simplified - in practice, use numerical integration
            BasisFunction *bf_i = &mesh->basis_functions[i];
            BasisFunction *bf_j = &mesh->basis_functions[j];
            
            // Approximate integration
            double area_i = bf_i->area;
            double area_j = bf_j->area;
            
            // Use Green's function dyadic
            for (int comp1 = 0; comp1 < 3; comp1++) {
                for (int comp2 = 0; comp2 < 3; comp2++) {
                    Z_ij += greens_func->G_ee[comp1][comp2] * area_i * area_j;
                }
            }
            
            net->Z_matrix[i * matrix_size + j] = Z_ij;
        }
    }
    
    // Calculate admittance matrix (inverse of impedance)
    // In practice, use LU decomposition
    memcpy(net->Y_matrix, net->Z_matrix, matrix_size * matrix_size * sizeof(double complex));
    
    return net;
}

// Surface roughness modeling using Huray model
double complex surface_roughness_correction(
    double frequency,
    double roughness_rms,
    double roughness_correlation_length,
    const double complex *surface_impedance) {
    
    // Huray model parameters
    double k0 = 2.0 * M_PI * frequency / C_0;
    double delta = roughness_rms;
    double lc = roughness_correlation_length;
    
    // Surface roughness correction factor
    double complex correction = 1.0 + I * 2.0 * k0 * delta * delta / lc;
    
    return (*surface_impedance) * correction;
}

// Memory management
void free_mesh_data(MeshData *mesh) {
    if (mesh) {
        free(mesh->triangles);
        free(mesh->rooftop_grids);
        free(mesh->edge_to_triangle);
        free(mesh->triangle_to_edges);
        
        for (int i = 0; i < mesh->n_basis_functions; i++) {
            free(mesh->basis_functions[i].coefficients);
        }
        free(mesh->basis_functions);
        
        memset(mesh, 0, sizeof(MeshData));
    }
}

void free_current_expansion(CurrentExpansion *exp) {
    if (exp) {
        free(exp->coefficients);
        free(exp);
    }
}

void free_network_parameters(NetworkParameters *net) {
    if (net) {
        free(net->Z_matrix);
        free(net->Y_matrix);
        free(net->S_matrix);
        free(net);
    }
}