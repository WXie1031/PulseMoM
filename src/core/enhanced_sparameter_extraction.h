/******************************************************************************
 * Enhanced S-Parameter Extraction for PCB Electromagnetic Simulation
 * 
 * This module provides commercial-grade S-parameter extraction capabilities:
 * - Real MoM-based Z to S parameter conversion
 * - Port re-normalization and mixed-mode S-parameters
 * - Vector fitting with passivity enforcement
 * - TRL/TRLM calibration and de-embedding
 * - Wideband rational modeling
 ******************************************************************************/

#ifndef ENHANCED_SPARAMETER_EXTRACTION_H
#define ENHANCED_SPARAMETER_EXTRACTION_H

#include <stdbool.h>
#include "pcb_electromagnetic_modeling.h"

#if !defined(_MSC_VER)
#include <complex.h>
typedef double complex sparameter_complex_t;
#else
// MSVC doesn't have complex.h, use custom complex type
typedef struct { double re; double im; } sparameter_complex_t;
#endif

/******************************************************************************
 * Enhanced S-Parameter Data Structures
 ******************************************************************************/

typedef struct SParameterData {
    double frequency;           // Frequency in Hz
    sparameter_complex_t *s_matrix;  // N-port S-matrix (column-major)
    int num_ports;            // Number of ports
    double *z_reference;      // Reference impedance per port
    bool is_differential;     // Mixed-mode flag
} SParameterData;

typedef struct SParameterSet {
    SParameterData *data;     // Array of S-parameter data
    int num_frequencies;      // Number of frequency points
    int num_ports;            // Number of ports
    char **port_names;        // Port names
    char *description;        // Description string
} SParameterSet;

typedef struct MixedModeSParameters {
    SParameterSet *differential;  // Differential mode S-parameters
    SParameterSet *common;       // Common mode S-parameters
    SParameterSet *mixed;        // Mixed-mode conversion terms
    int num_diff_pairs;          // Number of differential pairs
} MixedModeSParameters;

typedef struct VectorFittingModel {
    double *poles;            // Complex poles (rad/s)
    double *residues;         // Complex residues
    double *constant_term;    // Constant term (D)
    double *proportional_term; // Proportional term (E)
    int num_poles;            // Number of poles
    int num_ports;            // Number of ports
    bool is_passive;          // Passivity status
} VectorFittingModel;

typedef struct TRLCalibration {
    sparameter_complex_t *thru_matrix;    // Thru standard S-parameters
    sparameter_complex_t *reflect_matrix; // Reflect standard S-parameters
    sparameter_complex_t *line_matrix;      // Line standard S-parameters
    double line_length;             // Line length in meters
    double reflect_offset;          // Reflect offset in meters
    int num_ports;                  // Number of ports
    int num_frequencies;            // Number of frequencies
} TRLCalibration;

/******************************************************************************
 * Enhanced S-Parameter Extraction Functions
 ******************************************************************************/

/**
 * Extract S-parameters from MoM impedance matrix with real port definitions
 * Implements: S = (Z - Zref) * inv(Z + Zref)
 */
SParameterSet* extract_sparameters_from_mom(
    const PCBEMModel* em_model,
    const sparameter_complex_t* impedance_matrix,
    const double* z_reference,
    const double* frequencies,
    int num_frequencies
);

/**
 * Convert single-ended S-parameters to mixed-mode (differential/common)
 * Supports 4-port to 2 differential pair conversion
 */
MixedModeSParameters* convert_to_mixed_mode(
    const SParameterSet* single_ended,
    const int* diff_pair_mapping,  // [0,1,2,3] -> [diff1+,diff1-,diff2+,diff2-]
    int num_diff_pairs
);

/**
 * Re-normalize S-parameters to new reference impedance
 * Supports frequency-dependent reference impedance
 */
SParameterSet* renomalize_sparameters(
    const SParameterSet* original,
    const double* new_z_reference,  // Per-port or per-frequency
    bool frequency_dependent
);

/**
 * Apply TRL calibration to remove fixture effects
 */
SParameterSet* apply_trl_calibration(
    const SParameterSet* raw_sparameters,
    const TRLCalibration* calibration
);

/**
 * Vector fitting for rational model generation with passivity enforcement
 */
VectorFittingModel* vector_fitting_with_passivity(
    const SParameterSet* sparams,
    int num_poles,
    double convergence_tolerance,
    bool enforce_passivity
);

/**
 * Check passivity of S-parameters using Hamiltonian eigenvalue test
 */
bool check_passivity(const SParameterSet* sparams, double tolerance);

/**
 * Enforce passivity using spectral perturbation method
 */
SParameterSet* enforce_passivity(
    const SParameterSet* non_passive,
    double tolerance
);

/**
 * Check causality using Kramers-Kronig relations
 */
bool check_causality(const SParameterSet* sparams, double tolerance);

/**
 * Generate Touchstone file from S-parameters
 */
int write_touchstone_file(
    const SParameterSet* sparams,
    const char* filename,
    int format_type  // 1: MA, 2: DB, 3: RI
);

/**
 * Read Touchstone file into S-parameter structure
 */
SParameterSet* read_touchstone_file(const char* filename);

/**
 * Calculate S-parameter derived quantities
 */
typedef struct SParameterMetrics {
    double *return_loss_db;      // Sii in dB
    double *insertion_loss_db;   // Sij in dB (i≠j)
    double *vswr;                // Voltage standing wave ratio
    double *group_delay;         // Group delay in seconds
    double *phase_degrees;       // Phase in degrees
    double *magnitude_db;        // Magnitude in dB
} SParameterMetrics;

SParameterMetrics* calculate_sparameter_metrics(
    const SParameterSet* sparams
);

/**
 * Wideband rational modeling for circuit simulation
 */
typedef struct RationalModel {
    double *poles;              // Complex poles
    double *residues;           // Complex residues
    double constant_term;       // D term
    double proportional_term;   // E term
    int num_poles;
    int num_ports;
} RationalModel;

RationalModel* generate_rational_model(
    const SParameterSet* sparams,
    int max_poles,
    double error_tolerance
);

/**
 * Export rational model as SPICE subcircuit
 */
int export_spice_subcircuit(
    const RationalModel* model,
    const char* filename,
    const char* subcircuit_name
);

/**
 * Advanced de-embedding techniques
 */
typedef struct DeembeddingOptions {
    bool use_trl;               // Use TRL calibration
    bool use_open_short;        // Use open-short method
    bool use_thru_reflect;      // Use thru-reflect method
    bool use_automatic;         // Automatic method selection
    double frequency_limit;     // Frequency limit for method selection
    int interpolation_order;    // Interpolation order
} DeembeddingOptions;

SParameterSet* advanced_deembedding(
    const SParameterSet* raw_data,
    const SParameterSet* calibration_standards[4], // Short, Open, Load, Thru
    const DeembeddingOptions* options
);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * Interpolate S-parameters to new frequency points
 */
SParameterSet* interpolate_sparameters(
    const SParameterSet* original,
    const double* new_frequencies,
    int num_new_frequencies,
    int interpolation_method  // 1: linear, 2: spline, 3: rational
);

/**
 * Concatenate S-parameter sets across frequency
 */
SParameterSet* concatenate_sparameters(
    const SParameterSet** sparams_array,
    int num_sets
);

/**
 * Extract sub-matrix for selected ports
 */
SParameterSet* extract_port_subset(
    const SParameterSet* full_set,
    const int* port_indices,
    int num_selected_ports
);

/**
 * Calculate time-domain response using inverse FFT
 */
typedef struct TimeDomainResponse {
    double *time_points;        // Time in seconds
    sparameter_complex_t *response;     // Time-domain response
    int num_points;
    double time_step;
} TimeDomainResponse;

TimeDomainResponse* calculate_time_domain_response(
    const SParameterSet* sparams,
    double time_window,         // Time window in seconds
    int num_points,             // Number of time points
    const char* window_function // "rect", "hanning", "hamming", "kaiser"
);

/******************************************************************************
 * Error Handling and Validation
 ******************************************************************************/

typedef struct SParameterValidation {
    bool is_passive;
    bool is_causal;
    bool is_reciprocal;
    bool is_symmetric;
    double max_passivity_violation;
    double max_causality_violation;
    double max_reciprocity_violation;
    char error_message[256];
} SParameterValidation;

SParameterValidation* validate_sparameters(
    const SParameterSet* sparams,
    double tolerance
);

#endif // ENHANCED_SPARAMETER_EXTRACTION_H
bool check_causality(const SParameterSet* sset, double tol);
SParameterSet* renomalize_sparameters(
    const SParameterSet* original,
    const double* new_z_reference,
    bool frequency_dependent
);
