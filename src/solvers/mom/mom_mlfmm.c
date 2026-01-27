/*****************************************************************************************
 * PulseEM - MLFMM (Multilevel Fast Multipole Method) for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_mlfmm.c
 * Description: Implementation of MLFMM acceleration
 * 
 * References:
 * - Puma-EM: MLFMM implementation for large-scale problems
 * - Standard FMM/MLFMM algorithms for electromagnetic problems
 *****************************************************************************************/

#include "mom_mlfmm.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Compute multipole expansion order based on box size and wavenumber
int compute_multipole_order(double box_size, double k, double tolerance) {
    // Standard FMM formula: p ≈ k * box_size + log(1/tolerance) / log(2)
    // Reference: Puma-EM and standard FMM literature
    int p = (int)(k * box_size + 5.0 * log(1.0 / tolerance) / log(2.0));
    if (p < 3) p = 3;  // Minimum order
    if (p > 30) p = 30;  // Limit expansion order for efficiency
    return p;
}

// Build octree for MLFMM
mlfmm_tree_t* mlfmm_build_tree(
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    int num_elements,
    const mlfmm_params_t* params
) {
    if (!elements || !vertices || num_elements == 0 || !params) return NULL;
    
    mlfmm_tree_t* tree = (mlfmm_tree_t*)calloc(1, sizeof(mlfmm_tree_t));
    if (!tree) return NULL;
    
    tree->max_levels = params->max_levels > 0 ? params->max_levels : 5;
    tree->box_size = params->min_box_size > 0 ? params->min_box_size : 10;
    tree->frequency = 1e9;  // Will be set by caller
    tree->k = 2.0 * M_PI * tree->frequency / C0;
    tree->p_max = compute_multipole_order(1.0, tree->k, params->tolerance);
    
    // Compute bounding box
    double bbox_min[3] = {1e30, 1e30, 1e30};
    double bbox_max[3] = {-1e30, -1e30, -1e30};
    
    for (int i = 0; i < num_elements; i++) {
        if (elements[i].num_vertices >= 3) {
            for (int v = 0; v < 3; v++) {
                int vidx = elements[i].vertices[v];
                geom_point_t pos = vertices[vidx].position;
                if (pos.x < bbox_min[0]) bbox_min[0] = pos.x;
                if (pos.x > bbox_max[0]) bbox_max[0] = pos.x;
                if (pos.y < bbox_min[1]) bbox_min[1] = pos.y;
                if (pos.y > bbox_max[1]) bbox_max[1] = pos.y;
                if (pos.z < bbox_min[2]) bbox_min[2] = pos.z;
                if (pos.z > bbox_max[2]) bbox_max[2] = pos.z;
            }
        }
    }
    
    // Create root box
    tree->root = (mlfmm_box_t*)calloc(1, sizeof(mlfmm_box_t));
    if (!tree->root) {
        free(tree);
        return NULL;
    }
    
    tree->root->level = 0;
    tree->root->box_index = 0;
    tree->root->center[0] = (bbox_min[0] + bbox_max[0]) * 0.5;
    tree->root->center[1] = (bbox_min[1] + bbox_max[1]) * 0.5;
    tree->root->center[2] = (bbox_min[2] + bbox_max[2]) * 0.5;
    double max_size = fmax(fmax(bbox_max[0] - bbox_min[0],
                               bbox_max[1] - bbox_min[1]),
                          bbox_max[2] - bbox_min[2]);
    tree->root->size = max_size;
    tree->root->p_max = tree->p_max;
    
    // Allocate element indices for root
    tree->root->element_indices = (int*)malloc(num_elements * sizeof(int));
    if (!tree->root->element_indices) {
        free(tree->root);
        free(tree);
        return NULL;
    }
    
    for (int i = 0; i < num_elements; i++) {
        tree->root->element_indices[i] = i;
    }
    tree->root->num_elements = num_elements;
    
    // Note: Full octree subdivision would be implemented here
    // For now, we use a simplified single-level structure
    // In a full implementation, would recursively subdivide boxes
    
    printf("MoM MLFMM: Tree built (levels=%d, p_max=%d, k=%.6e)\n", 
           tree->max_levels, tree->p_max, tree->k);
    
    return tree;
}

// Free MLFMM tree
void mlfmm_free_tree(mlfmm_tree_t* tree) {
    if (!tree) return;
    
    // Recursively free boxes (simplified - would need full traversal)
    if (tree->root) {
        if (tree->root->element_indices) free(tree->root->element_indices);
        if (tree->root->multipole) free(tree->root->multipole);
        if (tree->root->local) free(tree->root->local);
        free(tree->root);
    }
    
    if (tree->boxes_by_level) {
        for (int level = 0; level < tree->max_levels; level++) {
            if (tree->boxes_by_level[level]) {
                if (tree->num_boxes_per_level) {
                    for (int b = 0; b < tree->num_boxes_per_level[level]; b++) {
                        // Note: boxes_by_level structure needs proper cleanup
                        // Simplified for now
                    }
                }
                free(tree->boxes_by_level[level]);
            }
        }
        free(tree->boxes_by_level);
    }
    
    if (tree->num_boxes_per_level) free(tree->num_boxes_per_level);
    free(tree);
}

// Compute multipole expansion coefficients for a box
// Reference: Puma-EM FMM_translation.py and standard FMM literature
static void compute_multipole_expansion(
    mlfmm_box_t* box,
    const complex_t* RESTRICT_PTR sources,
    const double* RESTRICT_PTR source_positions,
    int num_sources,
    double k
) {
    if (!box || !sources || !source_positions || num_sources == 0) return;
    
    int p_max = box->p_max;
    int num_coeffs = (p_max + 1) * (p_max + 1);  // Spherical harmonics: (L+1)^2
    
    if (!box->multipole) {
        box->multipole = (complex_t*)calloc(num_coeffs, sizeof(complex_t));
        if (!box->multipole) return;
    }
    
    // Initialize multipole coefficients
    for (int i = 0; i < num_coeffs; i++) {
        box->multipole[i] = complex_zero();
    }
    
    // Compute multipole expansion: M_n^m = Σ_j q_j * j_n(k*r_j) * Y_n^m*(θ_j, φ_j)
    // Simplified: use point source approximation
    for (int j = 0; j < num_sources; j++) {
        double dx = source_positions[3*j] - box->center[0];
        double dy = source_positions[3*j+1] - box->center[1];
        double dz = source_positions[3*j+2] - box->center[2];
        double r = sqrt(dx*dx + dy*dy + dz*dz) + 1e-12;
        
        if (r < 1e-12) continue;
        
        // Spherical coordinates
        double theta = acos(dz / r);
        double phi = atan2(dy, dx);
        
        // Compute spherical harmonics and add contribution
        // Simplified: use low-order expansion (n=0,1)
        complex_t source_val = sources[j];
        double kr = k * r;
        
        // n=0 term (monopole)
        if (num_coeffs > 0) {
            // j_0(kr) ≈ sin(kr)/(kr) for small kr, or use asymptotic
            double j0 = (kr < 1e-6) ? 1.0 : sin(kr) / kr;
            box->multipole[0].re += source_val.re * j0;
            box->multipole[0].im += source_val.im * j0;
        }
        
        // n=1 terms (dipole) - simplified
        if (num_coeffs > 4) {
            double j1 = (kr < 1e-6) ? kr/3.0 : (sin(kr)/(kr*kr) - cos(kr)/kr);
            double Y10 = sqrt(3.0/(4.0*M_PI)) * cos(theta);
            double Y11_re = -sqrt(3.0/(8.0*M_PI)) * sin(theta) * cos(phi);
            double Y11_im = -sqrt(3.0/(8.0*M_PI)) * sin(theta) * sin(phi);
            
            box->multipole[1].re += source_val.re * j1 * Y10;
            box->multipole[1].im += source_val.im * j1 * Y10;
            box->multipole[2].re += source_val.re * j1 * Y11_re;
            box->multipole[2].im += source_val.im * j1 * Y11_re;
            box->multipole[3].re += source_val.re * j1 * Y11_im;
            box->multipole[3].im += source_val.im * j1 * Y11_im;
        }
    }
}

// Multipole-to-multipole (M2M) translation
// Translates multipole expansion from child to parent box
static void mlfmm_m2m_translation(
    mlfmm_box_t* parent,
    mlfmm_box_t* child,
    double k
) {
    if (!parent || !child || !child->multipole) return;
    
    // Compute translation vector from child center to parent center
    double dx = child->center[0] - parent->center[0];
    double dy = child->center[1] - parent->center[1];
    double dz = child->center[2] - parent->center[2];
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (r < 1e-12) {
        // Child and parent have same center - just copy coefficients
        if (!parent->multipole) {
            int num_coeffs = (parent->p_max + 1) * (parent->p_max + 1);
            parent->multipole = (complex_t*)calloc(num_coeffs, sizeof(complex_t));
        }
        if (parent->multipole && child->multipole) {
            int num_coeffs = (child->p_max + 1) * (child->p_max + 1);
            for (int i = 0; i < num_coeffs; i++) {
                parent->multipole[i].re += child->multipole[i].re;
                parent->multipole[i].im += child->multipole[i].im;
            }
        }
        return;
    }
    
    // M2M translation using addition theorem for spherical harmonics
    // Simplified: use low-order translation
    // Full implementation would use Wigner 3-j symbols and rotation matrices
    
    if (!parent->multipole) {
        int num_coeffs = (parent->p_max + 1) * (parent->p_max + 1);
        parent->multipole = (complex_t*)calloc(num_coeffs, sizeof(complex_t));
    }
    
    if (parent->multipole && child->multipole) {
        // Simplified translation: add child multipole to parent
        // Full implementation would apply translation operator
        int num_coeffs = (child->p_max + 1) * (child->p_max + 1);
        for (int i = 0; i < num_coeffs && i < (parent->p_max + 1) * (parent->p_max + 1); i++) {
            parent->multipole[i].re += child->multipole[i].re;
            parent->multipole[i].im += child->multipole[i].im;
        }
    }
}

// Multipole-to-local (M2L) translation
// Translates multipole expansion to local expansion
static void mlfmm_m2l_translation(
    mlfmm_box_t* target,
    mlfmm_box_t* source,
    double k
) {
    if (!target || !source || !source->multipole) return;
    
    // Compute translation vector from source to target center
    double dx = target->center[0] - source->center[0];
    double dy = target->center[1] - source->center[1];
    double dz = target->center[2] - source->center[2];
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (r < 1e-12) return;  // Same box - skip
    
    // Initialize local expansion if needed
    if (!target->local) {
        int num_coeffs = (target->p_max + 1) * (target->p_max + 1);
        target->local = (complex_t*)calloc(num_coeffs, sizeof(complex_t));
        if (!target->local) return;
    }
    
    // M2L translation using addition theorem
    // Simplified: use Green's function translation
    // Full implementation would use Wigner 3-j symbols and rotation matrices
    
    // For now, simplified contribution from source multipole
    // Full M2L would compute: L_n^m = Σ_n' Σ_m' M_n'^m' * T_n'n^m'm(r)
    // where T is the M2L translation operator
    
    complex_t source_multipole_0 = source->multipole[0];
    double kr = k * r;
    
    // Simplified: add contribution from monopole term
    if (kr > 1e-12) {
        // Green's function: exp(-ikr)/(4πr)
        double phase = -kr;
        complex_t green = {(cos(phase) / (4.0 * M_PI * r)), (sin(phase) / (4.0 * M_PI * r))};
        
        target->local[0].re += source_multipole_0.re * green.re - source_multipole_0.im * green.im;
        target->local[0].im += source_multipole_0.re * green.im + source_multipole_0.im * green.re;
    }
}

// Local-to-local (L2L) translation
// Translates local expansion from parent to child box
static void mlfmm_l2l_translation(
    mlfmm_box_t* child,
    mlfmm_box_t* parent,
    double k
) {
    if (!child || !parent || !parent->local) return;
    
    // Compute translation vector from parent to child center
    double dx = child->center[0] - parent->center[0];
    double dy = child->center[1] - parent->center[1];
    double dz = child->center[2] - parent->center[2];
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (r < 1e-12) {
        // Same center - copy coefficients
        if (!child->local) {
            int num_coeffs = (child->p_max + 1) * (child->p_max + 1);
            child->local = (complex_t*)calloc(num_coeffs, sizeof(complex_t));
        }
        if (child->local && parent->local) {
            int num_coeffs = (parent->p_max + 1) * (parent->p_max + 1);
            for (int i = 0; i < num_coeffs && i < (child->p_max + 1) * (child->p_max + 1); i++) {
                child->local[i] = parent->local[i];
            }
        }
        return;
    }
    
    // L2L translation using addition theorem
    // Simplified: use low-order translation
    // Full implementation would use Wigner 3-j symbols and rotation matrices
    
    if (!child->local) {
        int num_coeffs = (child->p_max + 1) * (child->p_max + 1);
        child->local = (complex_t*)calloc(num_coeffs, sizeof(complex_t));
    }
    
    if (child->local && parent->local) {
        // Simplified: copy parent local expansion to child
        // Full L2L would apply translation operator
        int num_coeffs = (parent->p_max + 1) * (parent->p_max + 1);
        for (int i = 0; i < num_coeffs && i < (child->p_max + 1) * (child->p_max + 1); i++) {
            child->local[i] = parent->local[i];
        }
    }
}

// MLFMM matrix-vector product
void mlfmm_matrix_vector_product(
    const mlfmm_tree_t* RESTRICT_PTR tree,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    double freq
) {
    if (!tree || !x || !y) return;
    
    // Initialize output
    int i;
    #pragma omp parallel for if(n > 1000) \
                             shared(y, n) \
                             schedule(static)
    for (i = 0; i < n; i++) {
        y[i] = complex_zero();
    }
    
    double k = 2.0 * M_PI * freq / C0;
    
    // MLFMM algorithm:
    // 1. Upward pass: Compute multipole expansions (P2M, M2M)
    // 2. Downward pass: Translate and accumulate local expansions (M2L, L2L)
    // 3. Near-field: Direct computation
    // 4. Evaluation: Local-to-field (L2F) and direct evaluation
    
    // For now, simplified implementation
    // Full MLFMM would include:
    // - Complete octree traversal
    // - Proper M2M/M2L/L2L translation operators
    // - L2F evaluation at observation points
    
    // Note: This is a placeholder - full MLFMM implementation would be much more complex
    // Reference: Puma-EM MLFMA.py and standard FMM literature
}
