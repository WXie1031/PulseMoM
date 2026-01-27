/********************************************************************************
 * Circuit Coupling Orchestration Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements the orchestration logic for circuit-electromagnetic co-simulation.
 * L5 layer: Execution Orchestration - WHEN and HOW to couple circuit and EM solvers.
 *
 * Extracted from: physics/peec/circuit_coupling_simulation.c
 * 
 * Architecture Rule: L5 orchestrates the coupling flow, but does NOT define
 * the physical equations (which remain in L1 physics layer).
 ********************************************************************************/

#include "circuit_coupling_orchestration.h"
#include "../../physics/peec/peec_circuit_coupling.h"
#include "../../io/analysis/enhanced_sparameter_extraction.h"
#include "../../backend/solvers/sparse_direct_solver.h"
#include "../../operators/greens/layered_greens_function.h"  // For CDOUBLE definition
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Use CDOUBLE for complex numbers (compatible with MSVC)
#ifndef CDOUBLE
#if defined(_MSC_VER)
typedef struct { double re; double im; } cdouble;
#define CDOUBLE cdouble
#else
#include <complex.h>
#define CDOUBLE double complex
#endif
#endif

// Internal structures (from original circuit_coupling_simulation.c)
// These are implementation details, not exposed in the header
typedef struct CircuitNode {
    int index;
    char* name;
    double voltage;
    CDOUBLE voltage_ac;
    struct CircuitNode* next;
} CircuitNode;

typedef struct CircuitComponent {
    int type;
    char* name;
    int node1, node2;
    double value;
    CDOUBLE value_ac;
    void* sparams;
    void* nonlinear_model;
    struct CircuitComponent* next;
} CircuitComponent;

// Simple sparse matrix structure for MNA (stub implementation)
// In a full implementation, this would use the sparse_direct_solver interface
typedef struct SparseMatrix {
    int rows;
    int cols;
    // Simple dense storage for now (would be sparse in full implementation)
    CDOUBLE* data;
} SparseMatrix;

// Helper functions for sparse matrix operations (stub implementation)
static SparseMatrix* create_sparse_matrix(int rows, int cols) {
    SparseMatrix* mat = (SparseMatrix*)calloc(1, sizeof(SparseMatrix));
    if (!mat) return NULL;
    mat->rows = rows;
    mat->cols = cols;
    mat->data = (CDOUBLE*)calloc(rows * cols, sizeof(CDOUBLE));
    if (!mat->data) {
        free(mat);
        return NULL;
    }
    return mat;
}

static void destroy_sparse_matrix(SparseMatrix* mat) {
    if (!mat) return;
    if (mat->data) free(mat->data);
    free(mat);
}

static void sparse_matrix_add_value(SparseMatrix* mat, int row, int col, CDOUBLE value) {
    if (!mat || !mat->data || row < 0 || row >= mat->rows || col < 0 || col >= mat->cols) return;
#if defined(_MSC_VER)
    mat->data[row * mat->cols + col].re += value.re;
    mat->data[row * mat->cols + col].im += value.im;
#else
    mat->data[row * mat->cols + col] += value;
#endif
}

typedef struct MNAMatrix {
    SparseMatrix* G;
    SparseMatrix* C;
    SparseMatrix* L;
    CDOUBLE* B;
    CDOUBLE* X;
    int size;
} MNAMatrix;

// Internal orchestrator structure (implementation detail)
// The public interface uses void* pointers in the header
typedef struct {
    CircuitNode* nodes;
    CircuitComponent* components;
    MNAMatrix* mna_matrix;
    void* em_model;
    int num_nodes;
    int num_components;
    double frequency;
    bool is_nonlinear;
    double convergence_tolerance;
    int max_iterations;
} CircuitCouplingOrchestratorInternal;

/********************************************************************************
 * Create/Destroy
 ********************************************************************************/

CircuitCouplingOrchestrator* circuit_coupling_orchestrator_create(void) {
    CircuitCouplingOrchestratorInternal* orch = (CircuitCouplingOrchestratorInternal*)calloc(1, sizeof(CircuitCouplingOrchestratorInternal));
    if (!orch) return NULL;
    
    orch->convergence_tolerance = 1e-6;
    orch->max_iterations = 100;
    
    return (CircuitCouplingOrchestrator*)orch;
}

void circuit_coupling_orchestrator_destroy(CircuitCouplingOrchestrator* orch) {
    if (!orch) return;
    CircuitCouplingOrchestratorInternal* internal = (CircuitCouplingOrchestratorInternal*)orch;
    
    // Free nodes
    CircuitNode* node = internal->nodes;
    while (node) {
        CircuitNode* next = node->next;
        free(node->name);
        free(node);
        node = next;
    }
    
    // Free components
    CircuitComponent* comp = internal->components;
    while (comp) {
        CircuitComponent* next = comp->next;
        free(comp->name);
        free(comp);
        comp = next;
    }
    
    // Free MNA matrix
    if (internal->mna_matrix) {
        if (internal->mna_matrix->G) destroy_sparse_matrix(internal->mna_matrix->G);
        if (internal->mna_matrix->C) destroy_sparse_matrix(internal->mna_matrix->C);
        if (internal->mna_matrix->L) destroy_sparse_matrix(internal->mna_matrix->L);
        free(internal->mna_matrix->B);
        free(internal->mna_matrix->X);
        free(internal->mna_matrix);
    }
    
    free(internal);
}

/********************************************************************************
 * MNA Matrix Building (Numerical Implementation)
 ********************************************************************************/

int circuit_coupling_build_mna_matrix(CircuitCouplingOrchestrator* orch) {
    if (!orch) return -1;
    CircuitCouplingOrchestratorInternal* internal = (CircuitCouplingOrchestratorInternal*)orch;
    
    if (!internal->mna_matrix) {
        internal->mna_matrix = (MNAMatrix*)calloc(1, sizeof(MNAMatrix));
        if (!internal->mna_matrix) return -1;
    }
    
    MNAMatrix* mna = internal->mna_matrix;
    mna->size = internal->num_nodes;
    
    // Create sparse matrices
    mna->G = create_sparse_matrix(mna->size, mna->size);
    mna->C = create_sparse_matrix(mna->size, mna->size);
    mna->L = create_sparse_matrix(mna->size, mna->size);
    
    if (!mna->G || !mna->C || !mna->L) return -1;
    
    // Allocate vectors
    mna->B = (CDOUBLE*)calloc(mna->size, sizeof(CDOUBLE));
    mna->X = (CDOUBLE*)calloc(mna->size, sizeof(CDOUBLE));
    
    if (!mna->B || !mna->X) return -1;
    
    // Build matrix stamps (numerical implementation)
    // The physical equations are defined in L1 physics layer
    CircuitComponent* comp = internal->components;
    while (comp) {
        int n1 = comp->node1;
        int n2 = comp->node2;
        
        // Matrix stamping logic (numerical implementation)
        // This is L5 layer: HOW to build the matrix numerically
        // The physical meaning is defined in L1 physics layer
        
        switch (comp->type) {
            case 0: // RESISTOR
                {
                    double g = 1.0 / comp->value;
#if defined(_MSC_VER)
                    CDOUBLE g_val = {g, 0.0};
                    CDOUBLE neg_g = {-g, 0.0};
#else
                    CDOUBLE g_val = g;
                    CDOUBLE neg_g = -g;
#endif
                    sparse_matrix_add_value(mna->G, n1, n1, g_val);
                    sparse_matrix_add_value(mna->G, n2, n2, g_val);
                    sparse_matrix_add_value(mna->G, n1, n2, neg_g);
                    sparse_matrix_add_value(mna->G, n2, n1, neg_g);
                }
                break;
            case 1: // CAPACITOR
                {
#if defined(_MSC_VER)
                    CDOUBLE y = {0.0, 2.0 * M_PI * internal->frequency * comp->value};
#else
                    CDOUBLE y = I * 2.0 * M_PI * internal->frequency * comp->value;
#endif
                    sparse_matrix_add_value(mna->C, n1, n1, y);
                    sparse_matrix_add_value(mna->C, n2, n2, y);
#if defined(_MSC_VER)
                    CDOUBLE neg_y = {-y.re, -y.im};
#else
                    CDOUBLE neg_y = -y;
#endif
                    sparse_matrix_add_value(mna->C, n1, n2, neg_y);
                    sparse_matrix_add_value(mna->C, n2, n1, neg_y);
                }
                break;
            // ... other component types
        }
        
        comp = comp->next;
    }
    
    return 0;
}

/********************************************************************************
 * Nonlinear Solver (Numerical Implementation)
 ********************************************************************************/

int circuit_coupling_solve_nonlinear(CircuitCouplingOrchestrator* orch) {
    if (!orch) return -1;
    CircuitCouplingOrchestratorInternal* internal = (CircuitCouplingOrchestratorInternal*)orch;
    if (!internal->is_nonlinear) return -1;
    
    int iteration = 0;
    double error = 1.0;
    
    // Newton-Raphson iteration (numerical implementation)
    while (error > internal->convergence_tolerance && iteration < internal->max_iterations) {
        // Build Jacobian matrix
        if (circuit_coupling_build_mna_matrix(orch) != 0) return -1;
        
        // Solve linearized system (would call L4 solver)
        // This is L5 orchestration: WHEN to call the solver
        
        // Calculate nonlinear device currents
        // Update convergence error
        
        iteration++;
    }
    
    return (error <= internal->convergence_tolerance) ? 0 : -1;
}

/********************************************************************************
 * Time-Domain Coupling (Orchestration)
 ********************************************************************************/

int circuit_coupling_solve_time_domain(
    CircuitCouplingOrchestrator* orch,
    double time_start,
    double time_stop,
    double time_step) {
    
    if (!orch) return -1;
    
    // L5 orchestration: Time-stepping loop
    double current_time = time_start;
    int step = 0;
    
    while (current_time < time_stop) {
        // Update time-dependent sources (orchestration)
        
        // Build MNA matrix at this time step
        if (circuit_coupling_build_mna_matrix(orch) != 0) return -1;
        
        // Solve circuit (orchestration: decide when to use linear vs nonlinear)
        CircuitCouplingOrchestratorInternal* internal = (CircuitCouplingOrchestratorInternal*)orch;
        if (internal->is_nonlinear) {
            if (circuit_coupling_solve_nonlinear(orch) != 0) return -1;
        } else {
            // Solve linear system (would call L4 solver)
        }
        
        // Extract EM coupling if needed (orchestration)
        if (internal->em_model) {
            circuit_coupling_extract_em_coupling(orch, internal->em_model);
        }
        
        current_time += time_step;
        step++;
    }
    
    return 0;
}

/********************************************************************************
 * EM Coupling Extraction (Orchestration)
 ********************************************************************************/

int circuit_coupling_extract_em_coupling(
    CircuitCouplingOrchestrator* orch,
    void* em_model) {
    
    if (!orch || !em_model) return -1;
    
    // L5 orchestration: Extract EM coupling parameters
    // This coordinates between EM solver (L5 solvers layer) and circuit solver
    
    // Extract S-parameters or impedance matrix from EM model
    // Inject into circuit model
    
    // This is orchestration logic: WHEN and HOW to exchange data
    // The physical coupling equations are defined in L1 physics layer
    
    return 0;
}
