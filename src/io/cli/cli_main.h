/********************************************************************************
 * CLI Main Interface (L6 IO/Workflow/API Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines CLI interface.
 * L6 layer: IO/Workflow/API - provides CLI interface.
 *
 * Architecture Rule: L6 provides CLI, does NOT change simulation semantics.
 ********************************************************************************/

#ifndef CLI_MAIN_H
#define CLI_MAIN_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CLI Options
 */
typedef struct {
    char* input_file;
    char* output_file;
    char* solver_type;
    real_t frequency;
    bool verbose;
    bool help;
} cli_options_t;

/**
 * Parse command line arguments
 */
int cli_parse_arguments(
    int argc,
    char* argv[],
    cli_options_t* options
);

/**
 * Print help message
 */
void cli_print_help(const char* program_name);

/**
 * Main CLI entry point
 */
int cli_main(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif // CLI_MAIN_H
