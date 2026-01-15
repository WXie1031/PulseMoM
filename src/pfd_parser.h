/********************************************************************************
 * PFD File Parser for Satellite Electromagnetic Simulation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Parses PulseEM Data (PFD) files containing simulation configuration
 ********************************************************************************/

#ifndef PFD_PARSER_H
#define PFD_PARSER_H

#include <stdbool.h>

// PFD configuration structure
typedef struct {
    // Geometry
    char stl_file[256];
    double geometry_translation[3];
    
    // Frequency
    double frequency;
    
    // Plane wave excitation
    double plane_wave_direction[3];
    double plane_wave_polarization[3];
    double plane_wave_amplitude;
    double plane_wave_phase;
    
    // Mesh parameters
    double mesh_edge_length;
    int max_facets;
    
    // Material properties
    char material_name[64];
    double material_epsr;
    double material_mur;
    double material_sigma;
    
    // Solver settings
    char solver_type[16];  // "mom" or "peec"
    double solver_tolerance;
    int max_iterations;
    
    // Output settings
    char output_file[256];
    bool compute_near_field;
    bool compute_far_field;
    bool compute_currents;
    bool compute_rcs;
    
} pfd_config_t;

// Parse PFD file
int parse_pfd_file(const char* filename, pfd_config_t* config);

// Parse PFD content from string
int parse_pfd_content(const char* content, pfd_config_t* config);

// Write PFD file
int write_pfd_file(const char* filename, const pfd_config_t* config);

// Set default values
void set_pfd_defaults(pfd_config_t* config);

// Validate configuration
int validate_pfd_config(const pfd_config_t* config);

// Print configuration
void print_pfd_config(const pfd_config_t* config);

#endif // PFD_PARSER_H