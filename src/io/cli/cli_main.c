/********************************************************************************
 * CLI Main Implementation (L6 IO/Workflow/API Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements CLI interface.
 * L6 layer: IO/Workflow/API - provides CLI interface.
 *
 * Architecture Rule: L6 provides CLI, does NOT change simulation semantics.
 ********************************************************************************/

#include "cli_main.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../api/c_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MSVC compatibility: strdup is deprecated, use _strdup
#if defined(_MSC_VER) && !defined(strdup)
#define strdup _strdup
#endif

void cli_print_help(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -i, --input FILE       Input geometry file\n");
    printf("  -o, --output FILE      Output results file\n");
    printf("  -s, --solver TYPE      Solver type (mom/peec/mtl/hybrid)\n");
    printf("  -f, --frequency FREQ   Frequency in Hz\n");
    printf("  -v, --verbose          Verbose output\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -i geometry.stl -o results.s2p -s mom -f 1e9\n", program_name);
    printf("\n");
}

int cli_parse_arguments(
    int argc,
    char* argv[],
    cli_options_t* options) {
    
    if (!argv || !options) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Initialize options
    memset(options, 0, sizeof(cli_options_t));
    options->frequency = 1e9;  // Default 1 GHz
    options->verbose = false;
    options->help = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 < argc) {
                options->input_file = strdup(argv[++i]);
            } else {
                return STATUS_ERROR_INVALID_INPUT;
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                options->output_file = strdup(argv[++i]);
            } else {
                return STATUS_ERROR_INVALID_INPUT;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--solver") == 0) {
            if (i + 1 < argc) {
                options->solver_type = strdup(argv[++i]);
            } else {
                return STATUS_ERROR_INVALID_INPUT;
            }
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--frequency") == 0) {
            if (i + 1 < argc) {
                options->frequency = atof(argv[++i]);
            } else {
                return STATUS_ERROR_INVALID_INPUT;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            options->help = true;
        } else {
            return STATUS_ERROR_INVALID_INPUT;
        }
    }
    
    return STATUS_SUCCESS;
}

int cli_main(int argc, char* argv[]) {
    cli_options_t options;
    status_t status = cli_parse_arguments(argc, argv, &options);
    
    if (status != STATUS_SUCCESS) {
        cli_print_help(argv[0]);
        return 1;
    }
    
    if (options.help) {
        cli_print_help(argv[0]);
        return 0;
    }
    
    // Validate required options
    if (!options.input_file) {
        fprintf(stderr, "Error: Input file is required\n");
        cli_print_help(argv[0]);
        return 1;
    }
    
    if (!options.output_file) {
        fprintf(stderr, "Error: Output file is required\n");
        cli_print_help(argv[0]);
        return 1;
    }
    
    // L6 layer provides CLI, delegates to C API
    if (options.verbose) {
        printf("PulseMoM CLI\n");
        printf("Input file: %s\n", options.input_file);
        printf("Output file: %s\n", options.output_file);
        printf("Solver: %s\n", options.solver_type ? options.solver_type : "default");
        printf("Frequency: %.2e Hz\n", options.frequency);
        printf("\n");
    }
    
    // Create simulation handle
    simulation_handle_t* sim = api_create_simulation();
    if (!sim) {
        fprintf(stderr, "Error: Failed to create simulation handle\n");
        return 1;
    }
    
    // Load geometry
    if (options.verbose) {
        printf("Loading geometry...\n");
    }
    status = api_load_geometry(sim, options.input_file);
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to load geometry\n");
        api_destroy_simulation(sim);
        return 1;
    }
    
    // Set solver configuration (simplified)
    if (options.solver_type) {
        api_set_solver_config(sim, options.solver_type, NULL);
    }
    
    // Run simulation
    if (options.verbose) {
        printf("Running simulation...\n");
    }
    status = api_run_simulation(sim);
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Simulation failed\n");
        api_destroy_simulation(sim);
        return 1;
    }
    
    // Export results
    if (options.verbose) {
        printf("Exporting results...\n");
    }
    
    // Detect output format from file extension
    const char* ext = strrchr(options.output_file, '.');
    const char* format = "touchstone";  // Default
    if (ext) {
        if (strcmp(ext, ".s2p") == 0 || strcmp(ext, ".s4p") == 0) {
            format = "touchstone";
        } else if (strcmp(ext, ".csv") == 0) {
            format = "csv";
        } else if (strcmp(ext, ".json") == 0) {
            format = "json";
        } else if (strcmp(ext, ".h5") == 0 || strcmp(ext, ".hdf5") == 0) {
            format = "hdf5";
        }
    }
    
    status = api_export_results(sim, options.output_file, format);
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to export results\n");
        api_destroy_simulation(sim);
        return 1;
    }
    
    if (options.verbose) {
        printf("Simulation completed successfully\n");
    }
    
    // Cleanup
    api_destroy_simulation(sim);
    
    // Free allocated strings
    if (options.input_file) free(options.input_file);
    if (options.output_file) free(options.output_file);
    if (options.solver_type) free(options.solver_type);
    
    return 0;
}
