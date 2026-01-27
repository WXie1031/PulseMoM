/*****************************************************************************************
 * PEEC Triangular Mesh Support
 * 
 * Enhanced support for triangular meshes in PEEC solver
 * Supports: surface triangles, volume tetrahedra, mixed meshes
 *****************************************************************************************/

#include "../../solvers/peec/peec_solver.h"  // This includes layered_greens_function.h
#include "../geometry/core_geometry.h"
#include "core_mesh.h"
#include "../../common/core_common.h"
// layered_greens_function.h is already included via peec_solver.h
#include "../../operators/kernels/windowed_greens_function.h"  // For WGF support in PEEC
#include "../../solvers/peec/peec_non_manhattan_geometry.h"
#include "../../solvers/peec/peec_integration.h"
#include "../../solvers/peec/peec_via_modeling.h"  // For enhanced via modeling
#include <math.h>
#include <stdlib.h>
#include <string.h>

// MSVC-compatible complex number handling
#if defined(_MSC_VER)
typedef complex_t peec_complex_t;
#else
#include <complex.h>
typedef double complex peec_complex_t;
#endif

// Physical constants
#define MU0 (4.0 * M_PI * 1e-7)
#ifndef EPS0
#define EPS0 (8.854187817e-12)
#endif

/********************************************************************************
 * Extract Partial Elements from Triangular Mesh
 ********************************************************************************/
int peec_extract_partial_elements_triangular(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix) {
    
    if (!mesh || !resistance_matrix || !inductance_matrix || !capacitance_matrix) {
        return -1;
    }
    
    int num_triangles = 0;
    int* triangle_indices = NULL;
    
    // Count triangles
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == MESH_ELEMENT_TRIANGLE) {
            num_triangles++;
        }
    }
    
    if (num_triangles == 0) {
        return 0; // No triangles found
    }
    
    // Allocate triangle index array
    triangle_indices = (int*)malloc(num_triangles * sizeof(int));
    if (!triangle_indices) return -1;
    
    // Collect triangle indices
    int tri_idx = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == MESH_ELEMENT_TRIANGLE) {
            triangle_indices[tri_idx++] = i;
        }
    }
    
    // Extract partial elements for each triangle pair
    for (int i = 0; i < num_triangles; i++) {
        mesh_element_t* tri_i = &mesh->elements[triangle_indices[i]];
        
        for (int j = 0; j < num_triangles; j++) {
            mesh_element_t* tri_j = &mesh->elements[triangle_indices[j]];
            
            // Compute triangle centroids
            point3d_t centroid_i = {tri_i->centroid.x, tri_i->centroid.y, tri_i->centroid.z};
            point3d_t centroid_j = {tri_j->centroid.x, tri_j->centroid.y, tri_j->centroid.z};
            
            // Compute distance between centroids
            double dx = centroid_i.x - centroid_j.x;
            double dy = centroid_i.y - centroid_j.y;
            double dz = centroid_i.z - centroid_j.z;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (i == j) {
                // Self terms
                // Resistance: R = ρ * l / A
                // For triangle, use equivalent length and area
                double resistivity = 1.7e-8; // Copper
                double area = tri_i->area;
                double perimeter = 0.0;
                
                // Compute perimeter (simplified)
                if (tri_i->num_vertices >= 3) {
                    for (int k = 0; k < 3; k++) {
                        int v1 = tri_i->vertices[k];
                        int v2 = tri_i->vertices[(k+1) % 3];
                        if (v1 < mesh->num_vertices && v2 < mesh->num_vertices) {
                            point3d_t p1 = {mesh->vertices[v1].position.x, mesh->vertices[v1].position.y, mesh->vertices[v1].position.z};
                            point3d_t p2 = {mesh->vertices[v2].position.x, mesh->vertices[v2].position.y, mesh->vertices[v2].position.z};
                            double edge_len = sqrt((p2.x-p1.x)*(p2.x-p1.x) + 
                                                  (p2.y-p1.y)*(p2.y-p1.y) + 
                                                  (p2.z-p1.z)*(p2.z-p1.z));
                            perimeter += edge_len;
                        }
                    }
                }
                
                double equivalent_length = perimeter / 3.0;
                double equivalent_thickness = sqrt(area) * 0.1; // Simplified
                double cross_section = equivalent_length * equivalent_thickness;
                
                if (cross_section > 0) {
                    resistance_matrix[i * num_triangles + j] = 
                        resistivity * equivalent_length / cross_section;
                }
                
                // Self inductance (simplified formula for triangular loop)
                double mu = MU0;
                double characteristic_length = sqrt(area);
                if (characteristic_length > 0 && equivalent_thickness > 0) {
                    #if defined(_MSC_VER)
                    complex_t L_self = {
                        (mu * characteristic_length / (2.0 * M_PI)) * 
                        log(2.0 * characteristic_length / equivalent_thickness),
                        0.0
                    };
                    inductance_matrix[i * num_triangles + j] = L_self;
                    #else
                    inductance_matrix[i * num_triangles + j] = 
                        (mu * characteristic_length / (2.0 * M_PI)) * 
                        log(2.0 * characteristic_length / equivalent_thickness);
                    #endif
                }
                
                // Self capacitance
                double eps = EPS0;
                double separation = equivalent_thickness; // Simplified
                if (separation > 0) {
                    capacitance_matrix[i * num_triangles + j] = 
                        eps * area / separation;
                }
            } else {
                // Mutual terms
                if (distance > 0) {
                    // Mutual inductance
                    double mu = MU0;
                    double area_avg = 0.5 * (tri_i->area + tri_j->area);
                    double length_avg = sqrt(area_avg);
                    
                    #if defined(_MSC_VER)
                    complex_t L_mutual = {
                        (mu / (2.0 * M_PI)) * log(length_avg / distance),
                        0.0
                    };
                    inductance_matrix[i * num_triangles + j] = L_mutual;
                    #else
                    inductance_matrix[i * num_triangles + j] = 
                        (mu / (2.0 * M_PI)) * log(length_avg / distance);
                    #endif
                    
                    // Mutual capacitance
                    double eps = EPS0;
                    double area_avg_cap = 0.5 * (tri_i->area + tri_j->area);
                    capacitance_matrix[i * num_triangles + j] = 
                        eps * area_avg_cap / distance;
                }
            }
        }
    }
    
    free(triangle_indices);
    return 0;
}

/********************************************************************************
 * Convert Triangular Mesh to PEEC Circuit
 ********************************************************************************/
int peec_convert_triangular_mesh_to_circuit(
    mesh_t* mesh,
    int* num_nodes,
    int* num_elements) {
    
    if (!mesh || !num_nodes || !num_elements) return -1;
    
    // Count triangles
    int num_triangles = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == MESH_ELEMENT_TRIANGLE) {
            num_triangles++;
        }
    }
    
    // Each triangle becomes a circuit element
    // Nodes are created at triangle vertices
    *num_elements = num_triangles;
    
    // Count unique vertices (simplified - would need proper vertex mapping)
    *num_nodes = mesh->num_vertices;
    
    return 0;
}

/********************************************************************************
 * Build Circuit Network from Triangular Mesh
 ********************************************************************************/
int peec_build_circuit_from_triangular_mesh(
    mesh_t* mesh,
    int* node_map,
    int* element_to_nodes) {
    
    if (!mesh || !node_map || !element_to_nodes) return -1;
    
    // Map triangle elements to circuit nodes
    int tri_idx = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == MESH_ELEMENT_TRIANGLE) {
            mesh_element_t* tri = &mesh->elements[i];
            
            // Each triangle has 3 vertices -> 3 nodes
            if (tri->num_vertices >= 3) {
                element_to_nodes[tri_idx * 3 + 0] = tri->vertices[0];
                element_to_nodes[tri_idx * 3 + 1] = tri->vertices[1];
                element_to_nodes[tri_idx * 3 + 2] = tri->vertices[2];
            }
            
            tri_idx++;
        }
    }
    
    return 0;
}

