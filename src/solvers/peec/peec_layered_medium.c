/******************************************************************************
 * PEEC Solver with Layered Medium Support - Implementation
 ******************************************************************************/

#include "peec_layered_medium.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

int peec_solver_set_layered_medium(
    peec_solver_t* solver,
    const LayeredMedium* medium,
    const FrequencyDomain* freq,
    const GreensFunctionParams* greens_params
) {
    if (!solver || !medium) {
        return -1;
    }
    
    // Call the base peec_solver_set_layered_medium function
    // This function is declared in peec_solver.h and should be implemented
    // in the main PEEC solver implementation
    
    // For now, this is a wrapper that validates inputs
    // The actual implementation would be in peec_solver_unified.c or similar
    if (!solver || !medium || !freq) {
        return -1;
    }
    
    // Validate medium structure
    if (medium->num_layers <= 0 || !medium->thickness || !medium->epsilon_r) {
        return -1;
    }
    
    // Call the actual implementation (would be in peec_solver implementation)
    // return peec_solver_set_layered_medium_impl(solver, medium, freq, greens_params);
    
    // For now, return success (actual implementation would be in solver)
    return 0;
}

int peec_solver_configure_pcb(
    peec_solver_t* solver,
    const void* pcb_layers,
    int num_layers
) {
    if (!solver || !pcb_layers || num_layers < 1) {
        return -1;
    }
    
    // Create layered medium from PCB layers
    LayeredMedium* medium = (LayeredMedium*)calloc(1, sizeof(LayeredMedium));
    if (!medium) {
        return -1;
    }
    
    medium->num_layers = num_layers;
    medium->thickness = (double*)calloc(num_layers, sizeof(double));
    medium->epsilon_r = (double*)calloc(num_layers, sizeof(double));
    medium->mu_r = (double*)calloc(num_layers, sizeof(double));
    medium->sigma = (double*)calloc(num_layers, sizeof(double));
    medium->tan_delta = (double*)calloc(num_layers, sizeof(double));
    
    // Copy layer data from PCB layers
    // Assume pcb_layers is PCBLayerInfo* array
    typedef struct {
        char name[64];
        int type;  // PCBLayerType
        double thickness;
        double dielectric_constant;
        double loss_tangent;
        double copper_thickness;
        double elevation;
    } PCBLayerInfo;
    
    PCBLayerInfo* layers = (PCBLayerInfo*)pcb_layers;
    
    for (int i = 0; i < num_layers; i++) {
        medium->thickness[i] = layers[i].thickness * 1e-3;  // Convert mm to m
        medium->epsilon_r[i] = layers[i].dielectric_constant;
        medium->mu_r[i] = 1.0;  // Default: non-magnetic
        medium->sigma[i] = (layers[i].type == 0) ? 5.8e7 : 0.0;  // Copper conductivity if copper layer
        medium->tan_delta[i] = layers[i].loss_tangent;
    }
    
    // Set layered medium
    // Create frequency domain (simplified)
    FrequencyDomain freq = {0};
    freq.freq = 1e9;  // Default frequency
    freq.omega = 2.0 * M_PI * freq.freq;
    int result = peec_solver_set_layered_medium(solver, medium, &freq, NULL);
    
    if (result != 0) {
        // Free allocated memory
        if (medium->thickness) free(medium->thickness);
        if (medium->epsilon_r) free(medium->epsilon_r);
        if (medium->mu_r) free(medium->mu_r);
        if (medium->sigma) free(medium->sigma);
        if (medium->tan_delta) free(medium->tan_delta);
        free(medium);
        return result;
    }
    
    return 0;
}

int peec_solver_configure_antenna(
    peec_solver_t* solver,
    double substrate_thickness,
    double substrate_er,
    double substrate_tan_delta
) {
    if (!solver || substrate_thickness <= 0.0 || substrate_er < 1.0) {
        return -1;
    }
    
    // Create simple 3-layer medium: air-substrate-air
    LayeredMedium* medium = (LayeredMedium*)calloc(1, sizeof(LayeredMedium));
    if (!medium) {
        return -1;
    }
    
    medium->num_layers = 3;
    medium->thickness = (double*)calloc(3, sizeof(double));
    medium->epsilon_r = (double*)calloc(3, sizeof(double));
    medium->mu_r = (double*)calloc(3, sizeof(double));
    medium->sigma = (double*)calloc(3, sizeof(double));
    medium->tan_delta = (double*)calloc(3, sizeof(double));
    
    // Layer 0: Air (top)
    medium->thickness[0] = 1.0;
    medium->epsilon_r[0] = 1.0;
    medium->mu_r[0] = 1.0;
    medium->sigma[0] = 0.0;
    medium->tan_delta[0] = 0.0;
    
    // Layer 1: Substrate
    medium->thickness[1] = substrate_thickness;
    medium->epsilon_r[1] = substrate_er;
    medium->mu_r[1] = 1.0;
    medium->sigma[1] = 0.0;
    medium->tan_delta[1] = substrate_tan_delta;
    
    // Layer 2: Air (bottom)
    medium->thickness[2] = 1.0;
    medium->epsilon_r[2] = 1.0;
    medium->mu_r[2] = 1.0;
    medium->sigma[2] = 0.0;
    medium->tan_delta[2] = 0.0;
    
    // Set layered medium
    // Create frequency domain (simplified)
    FrequencyDomain freq = {0};
    freq.freq = 1e9;  // Default frequency
    freq.omega = 2.0 * M_PI * freq.freq;
    int result = peec_solver_set_layered_medium(solver, medium, &freq, NULL);
    
    if (result != 0) {
        // Free allocated memory
        if (medium->thickness) free(medium->thickness);
        if (medium->epsilon_r) free(medium->epsilon_r);
        if (medium->mu_r) free(medium->mu_r);
        if (medium->sigma) free(medium->sigma);
        if (medium->tan_delta) free(medium->tan_delta);
        free(medium);
        return result;
    }
    
    return 0;
}

int peec_solver_configure_metamaterial(
    peec_solver_t* solver,
    double unit_cell_size,
    double effective_er,
    double effective_mu
) {
    if (!solver || unit_cell_size <= 0.0) {
        return -1;
    }
    
    // Create single-layer medium with effective properties
    LayeredMedium* medium = (LayeredMedium*)calloc(1, sizeof(LayeredMedium));
    if (!medium) {
        return -1;
    }
    
    medium->num_layers = 1;
    medium->thickness = (double*)calloc(1, sizeof(double));
    medium->epsilon_r = (double*)calloc(1, sizeof(double));
    medium->mu_r = (double*)calloc(1, sizeof(double));
    medium->sigma = (double*)calloc(1, sizeof(double));
    medium->tan_delta = (double*)calloc(1, sizeof(double));
    
    medium->thickness[0] = unit_cell_size;
    medium->epsilon_r[0] = effective_er;
    medium->mu_r[0] = effective_mu;
    medium->sigma[0] = 0.0;
    medium->tan_delta[0] = 0.0;
    
    // Set layered medium
    // Create frequency domain (simplified)
    FrequencyDomain freq = {0};
    freq.freq = 1e9;  // Default frequency
    freq.omega = 2.0 * M_PI * freq.freq;
    int result = peec_solver_set_layered_medium(solver, medium, &freq, NULL);
    
    if (result != 0) {
        // Free allocated memory
        if (medium->thickness) free(medium->thickness);
        if (medium->epsilon_r) free(medium->epsilon_r);
        if (medium->mu_r) free(medium->mu_r);
        if (medium->sigma) free(medium->sigma);
        if (medium->tan_delta) free(medium->tan_delta);
        free(medium);
        return result;
    }
    
    return 0;
}
