/********************************************************************************
 * PulseEM - Unified Electromagnetic Simulation Platform
 *
 * Copyright (C) 2024-2025 PulseEM Technologies
 *
 * Commercial License - All Rights Reserved
 * Unauthorized copying, modification, or distribution is strictly prohibited
 * Proprietary and confidential - see LICENSE file for details
 *
 * File: basis_functions.h
 * Description: Basis functions for electromagnetic field calculations
 ********************************************************************************/

#ifndef BASIS_FUNCTIONS_H
#define BASIS_FUNCTIONS_H

#include <stdbool.h>
#include "../../operators/greens/layered_greens_function.h"

// CDOUBLE is defined in layered_greens_function.h

typedef struct {
    double x[3];           // Triangle vertices
    double y[3];
    double z[3];
    int layer;            // Layer index
    int material_id;      // Material property index
} Triangle;

typedef struct {
    double x_min, x_max;   // Bounding box
    double y_min, y_max;
    double z;
    int layer;
    int nx, ny;           // Discretization
    double dx, dy;        // Cell sizes
} RooftopGrid;

typedef struct {
    int type;             // 0: RWG, 1: Rooftop
    int edge_index;       // Edge index for RWG
    int cell_index;       // Cell index for Rooftop
    int direction;        // 0: x, 1: y for Rooftop
    double area;          // Basis function support area
    double *coefficients;   // Expansion coefficients
    int n_coeffs;
} BasisFunction;

typedef struct {
    Triangle *triangles;
    int n_triangles;
    
    RooftopGrid *rooftop_grids;
    int n_rooftop_grids;
    
    BasisFunction *basis_functions;
    int n_basis_functions;
    
    int *edge_to_triangle;     // Edge connectivity
    int *triangle_to_edges;      // Triangle connectivity
    int n_edges;
} MeshData;

typedef struct {
    double x, y, z;        // Position
    double length;         // Port length
    double width;          // Port width
    int layer;            // Layer index
    int type;             // 0: Lumped, 1: Edge, 2: Differential
    int reference_ground; // Reference ground layer
} PortDefinition;

typedef struct {
    double frequency;      // Frequency [Hz]
    CDOUBLE *Z_matrix;  // Impedance matrix
    CDOUBLE *Y_matrix;  // Admittance matrix
    CDOUBLE *S_matrix;  // Scattering matrix
    int matrix_size;      // Size of matrices
} NetworkParameters;

// RWG basis function creation
void create_rwg_basis_functions(
    MeshData *mesh,
    const Triangle *triangles,
    int n_triangles,
    const LayeredMedium *medium
);

// Rooftop basis function creation
void create_rooftop_basis_functions(
    MeshData *mesh,
    const RooftopGrid *grid,
    const LayeredMedium *medium
);

// Mixed basis function approach (optimal for PCB)
void create_mixed_basis_functions(
    MeshData *mesh,
    const Triangle *triangles,
    int n_triangles,
    const RooftopGrid *rooftop_grids,
    int n_rooftop_grids,
    const LayeredMedium *medium
);

// Basis function evaluation
CDOUBLE evaluate_basis_function(
    const BasisFunction *bf,
    double x, double y, double z,
    int component  // 0: x, 1: y, 2: z
);

// Surface current expansion
typedef struct {
    CDOUBLE *coefficients;  // Basis function coefficients
    int n_coefficients;
    double frequency;
} CurrentExpansion;

CurrentExpansion* expand_surface_current(
    const MeshData *mesh,
    const CDOUBLE *current_values,
    const PortDefinition *ports,
    int n_ports
);

// Impedance matrix assembly using layered medium Green's function
NetworkParameters* assemble_impedance_matrix(
    const MeshData *mesh,
    const GreensFunctionDyadic *greens_func,
    const FrequencyDomain *freq,
    const PortDefinition *ports,
    int n_ports
);

// Surface roughness modeling using Huray model
CDOUBLE surface_roughness_correction(
    double frequency,
    double roughness_rms,
    double roughness_correlation_length,
    const CDOUBLE *surface_impedance
);

// Edge basis functions for improved accuracy
void create_edge_enhanced_basis(
    MeshData *mesh,
    const Triangle *triangles,
    int n_triangles,
    double edge_length_threshold
);

// Port definition and excitation
void define_ports(
    PortDefinition *ports,
    int n_ports,
    const MeshData *mesh,
    const LayeredMedium *medium
);

// Differential port handling
void create_differential_ports(
    PortDefinition *positive_port,
    PortDefinition *negative_port,
    PortDefinition *differential_port,
    double separation_distance
);

// Memory management
void free_mesh_data(MeshData *mesh);
void free_current_expansion(CurrentExpansion *expansion);
void free_network_parameters(NetworkParameters *net);

#endif // BASIS_FUNCTIONS_H
