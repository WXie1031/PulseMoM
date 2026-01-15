/*****************************************************************************************
 * PulseEM - MLFMM (Multilevel Fast Multipole Method) for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_mlfmm.h
 * Description: Header for MLFMM acceleration
 *****************************************************************************************/

#ifndef MOM_MLFMM_H
#define MOM_MLFMM_H

#include "../core/core_common.h"
#include "../core/core_mesh.h"
#include <stdbool.h>

// Compiler optimization hints
#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT_PTR __restrict__
#elif defined(_MSC_VER)
#define RESTRICT_PTR __restrict
#else
#define RESTRICT_PTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct mlfmm_box_s mlfmm_box_t;

// MLFMM box structure
typedef struct mlfmm_box_s {
    int level;              // Tree level (0 = root)
    int box_index;          // Box index at this level
    double center[3];       // Box center coordinates
    double size;            // Box size (edge length)
    int* element_indices;   // Elements in this box
    int num_elements;       // Number of elements
    mlfmm_box_t* children[8];  // 8 children for octree
    complex_t* multipole;   // Multipole expansion coefficients
    complex_t* local;       // Local expansion coefficients
    int p_max;              // Maximum expansion order
} mlfmm_box_t;

// MLFMM tree structure
typedef struct {
    mlfmm_box_t* root;      // Root of octree
    int max_levels;         // Maximum tree depth
    int box_size;           // Minimum elements per box
    double frequency;       // Operating frequency
    double k;               // Wavenumber
    int p_max;              // Maximum multipole expansion order
    mlfmm_box_t** boxes_by_level;  // Array of box arrays per level
    int* num_boxes_per_level;      // Number of boxes per level
} mlfmm_tree_t;

// MLFMM parameters
typedef struct {
    int max_levels;         // Maximum tree depth
    int min_box_size;       // Minimum elements per box
    double tolerance;      // Expansion tolerance
    bool use_adaptive_p;    // Adaptive expansion order
} mlfmm_params_t;

// MLFMM functions
int compute_multipole_order(double box_size, double k, double tolerance);

mlfmm_tree_t* mlfmm_build_tree(
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    int num_elements,
    const mlfmm_params_t* params
);

void mlfmm_free_tree(mlfmm_tree_t* tree);

// MLFMM matrix-vector product
void mlfmm_matrix_vector_product(
    const mlfmm_tree_t* RESTRICT_PTR tree,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    double freq
);

#ifdef __cplusplus
}
#endif

#endif // MOM_MLFMM_H
