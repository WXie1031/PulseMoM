/********************************************************************************
 * PFD File Parser Implementation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Parses PulseEM Data (PFD) files containing simulation configuration
 ********************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include "pfd_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Trim whitespace from string
char* trim(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Parse vector from string (format: "x,y,z")
int parse_vector(const char* str, double vec[3]) {
    return sscanf(str, "%lf,%lf,%lf", &vec[0], &vec[1], &vec[2]);
}

// Set default configuration values
void set_pfd_defaults(pfd_config_t* config) {
    memset(config, 0, sizeof(pfd_config_t));
    
    // Default file names
    strcpy(config->stl_file, "satellite.stl");
    strcpy(config->output_file, "results.json");
    
    // Default frequency (10 GHz)
    config->frequency = 10e9;
    
    // Default plane wave (X-direction, Z-polarized)
    config->plane_wave_direction[0] = 1.0;
    config->plane_wave_direction[1] = 0.0;
    config->plane_wave_direction[2] = 0.0;
    config->plane_wave_polarization[0] = 0.0;
    config->plane_wave_polarization[1] = 0.0;
    config->plane_wave_polarization[2] = 1.0;
    config->plane_wave_amplitude = 1000.0;  // 1 kV/m
    config->plane_wave_phase = 0.0;
    
    // Default mesh (lambda/10 at 10 GHz = 3mm)
    config->mesh_edge_length = 0.003;
    config->max_facets = 10000;
    
    // Default material (PEC)
    strcpy(config->material_name, "PEC");
    config->material_epsr = 1.0;
    config->material_mur = 1.0;
    config->material_sigma = 1e20;  // Perfect conductor
    
    // Default solver
    strcpy(config->solver_type, "mom");
    config->solver_tolerance = 1e-6;
    config->max_iterations = 1000;
    
    // Default geometry translation
    config->geometry_translation[0] = 0.0;
    config->geometry_translation[1] = 0.0;
    config->geometry_translation[2] = -0.55;  // -550mm
    
    // Default output settings
    config->compute_near_field = true;
    config->compute_far_field = true;
    config->compute_currents = true;
    config->compute_rcs = true;
}

// Parse PFD file
int parse_pfd_file(const char* filename, pfd_config_t* config) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Cannot open PFD file %s\n", filename);
        return -1;
    }
    
    // Read entire file into memory
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(fp);
        return -1;
    }
    
    size_t read_size = fread(content, 1, file_size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    // Parse content
    int result = parse_pfd_content(content, config);
    free(content);
    
    return result;
}

// Parse PFD content from string
int parse_pfd_content(const char* content, pfd_config_t* config) {
    set_pfd_defaults(config);
    
    char* content_copy = strdup(content);
    if (!content_copy) return -1;
    
    char* line = strtok(content_copy, "\n\r");
    while (line != NULL) {
        line = trim(line);
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            line = strtok(NULL, "\n\r");
            continue;
        }
        
        char key[64], value[256];
        if (sscanf(line, "%63s %255[^\n]", key, value) == 2) {
            // Remove trailing whitespace from value
            char* trimmed_value = trim(value);
            
            if (strcmp(key, "FREQUENCY") == 0) {
                config->frequency = atof(trimmed_value);
            } else if (strcmp(key, "STL_FILE") == 0) {
                strcpy(config->stl_file, trimmed_value);
            } else if (strcmp(key, "PLANE_WAVE_DIRECTION") == 0) {
                parse_vector(trimmed_value, config->plane_wave_direction);
            } else if (strcmp(key, "PLANE_WAVE_POLARIZATION") == 0) {
                parse_vector(trimmed_value, config->plane_wave_polarization);
            } else if (strcmp(key, "PLANE_WAVE_AMPLITUDE") == 0) {
                config->plane_wave_amplitude = atof(trimmed_value);
            } else if (strcmp(key, "PLANE_WAVE_PHASE") == 0) {
                config->plane_wave_phase = atof(trimmed_value);
            } else if (strcmp(key, "MESH_EDGE_LENGTH") == 0) {
                config->mesh_edge_length = atof(trimmed_value);
            } else if (strcmp(key, "MAX_FACETS") == 0) {
                config->max_facets = atoi(trimmed_value);
            } else if (strcmp(key, "MATERIAL_NAME") == 0) {
                strcpy(config->material_name, trimmed_value);
            } else if (strcmp(key, "MATERIAL_EPSR") == 0) {
                config->material_epsr = atof(trimmed_value);
            } else if (strcmp(key, "MATERIAL_MUR") == 0) {
                config->material_mur = atof(trimmed_value);
            } else if (strcmp(key, "MATERIAL_SIGMA") == 0) {
                config->material_sigma = atof(trimmed_value);
            } else if (strcmp(key, "SOLVER_TYPE") == 0) {
                strcpy(config->solver_type, trimmed_value);
            } else if (strcmp(key, "SOLVER_TOLERANCE") == 0) {
                config->solver_tolerance = atof(trimmed_value);
            } else if (strcmp(key, "MAX_ITERATIONS") == 0) {
                config->max_iterations = atoi(trimmed_value);
            } else if (strcmp(key, "GEOMETRY_TRANSLATION") == 0) {
                parse_vector(trimmed_value, config->geometry_translation);
            } else if (strcmp(key, "OUTPUT_FILE") == 0) {
                strcpy(config->output_file, trimmed_value);
            }
        }
        
        line = strtok(NULL, "\n\r");
    }
    
    free(content_copy);
    return validate_pfd_config(config);
}

// Validate configuration
int validate_pfd_config(const pfd_config_t* config) {
    if (config->frequency <= 0) {
        printf("Error: Invalid frequency %.3e Hz\n", config->frequency);
        return -1;
    }
    
    if (config->mesh_edge_length <= 0) {
        printf("Error: Invalid mesh edge length %.3e m\n", config->mesh_edge_length);
        return -1;
    }
    
    if (config->max_facets <= 0) {
        printf("Error: Invalid max facets %d\n", config->max_facets);
        return -1;
    }
    
    if (strlen(config->stl_file) == 0) {
        printf("Error: STL file not specified\n");
        return -1;
    }
    
    if (config->plane_wave_amplitude <= 0) {
        printf("Error: Invalid plane wave amplitude %.3e V/m\n", config->plane_wave_amplitude);
        return -1;
    }
    
    return 0;
}

// Write PFD file
int write_pfd_file(const char* filename, const pfd_config_t* config) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot write PFD file %s\n", filename);
        return -1;
    }
    
    fprintf(fp, "# PulseEM Data (PFD) Configuration File\n");
    fprintf(fp, "# Generated by PFD Parser\n\n");
    
    fprintf(fp, "FREQUENCY %.3e\n", config->frequency);
    fprintf(fp, "STL_FILE %s\n", config->stl_file);
    fprintf(fp, "PLANE_WAVE_DIRECTION %.3f,%.3f,%.3f\n", 
            config->plane_wave_direction[0], 
            config->plane_wave_direction[1], 
            config->plane_wave_direction[2]);
    fprintf(fp, "PLANE_WAVE_POLARIZATION %.3f,%.3f,%.3f\n", 
            config->plane_wave_polarization[0], 
            config->plane_wave_polarization[1], 
            config->plane_wave_polarization[2]);
    fprintf(fp, "PLANE_WAVE_AMPLITUDE %.3f\n", config->plane_wave_amplitude);
    fprintf(fp, "PLANE_WAVE_PHASE %.3f\n", config->plane_wave_phase);
    fprintf(fp, "MESH_EDGE_LENGTH %.6f\n", config->mesh_edge_length);
    fprintf(fp, "MAX_FACETS %d\n", config->max_facets);
    fprintf(fp, "MATERIAL_NAME %s\n", config->material_name);
    fprintf(fp, "MATERIAL_EPSR %.3f\n", config->material_epsr);
    fprintf(fp, "MATERIAL_MUR %.3f\n", config->material_mur);
    fprintf(fp, "MATERIAL_SIGMA %.3e\n", config->material_sigma);
    fprintf(fp, "SOLVER_TYPE %s\n", config->solver_type);
    fprintf(fp, "SOLVER_TOLERANCE %.2e\n", config->solver_tolerance);
    fprintf(fp, "MAX_ITERATIONS %d\n", config->max_iterations);
    fprintf(fp, "GEOMETRY_TRANSLATION %.3f,%.3f,%.3f\n", 
            config->geometry_translation[0], 
            config->geometry_translation[1], 
            config->geometry_translation[2]);
    fprintf(fp, "OUTPUT_FILE %s\n", config->output_file);
    
    fclose(fp);
    return 0;
}

// Print configuration
void print_pfd_config(const pfd_config_t* config) {
    printf("=== PFD Configuration ===\n");
    printf("Frequency: %.3e Hz (%.1f GHz)\n", config->frequency, config->frequency / 1e9);
    printf("STL File: %s\n", config->stl_file);
    printf("Plane Wave Direction: (%.3f, %.3f, %.3f)\n", 
           config->plane_wave_direction[0], 
           config->plane_wave_direction[1], 
           config->plane_wave_direction[2]);
    printf("Plane Wave Amplitude: %.2f V/m\n", config->plane_wave_amplitude);
    printf("Mesh Edge Length: %.6f m (%.2f wavelengths)\n", 
           config->mesh_edge_length, 
           config->mesh_edge_length * config->frequency / 299792458.0);
    printf("Max Facets: %d\n", config->max_facets);
    printf("Material: %s (epsr=%.2f, mur=%.2f, sigma=%.2e S/m)\n", 
           config->material_name, 
           config->material_epsr, 
           config->material_mur, 
           config->material_sigma);
    printf("Solver: %s (tolerance=%.2e, max_iter=%d)\n", 
           config->solver_type, 
           config->solver_tolerance, 
           config->max_iterations);
    printf("Geometry Translation: (%.3f, %.3f, %.3f) m\n", 
           config->geometry_translation[0], 
           config->geometry_translation[1], 
           config->geometry_translation[2]);
    printf("Output File: %s\n", config->output_file);
    printf("========================\n");
}