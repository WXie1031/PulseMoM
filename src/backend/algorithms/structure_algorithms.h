/******************************************************************************
 * Specialized Structure Calculation Algorithms
 * 
 * Optimized algorithms for electromagnetic simulation of different PCB/IC
 * structure types with adaptive method selection
 ******************************************************************************/

#ifndef STRUCTURE_ALGORITHMS_H
#define STRUCTURE_ALGORITHMS_H

#include "../../discretization/geometry/pcb_ic_structures.h"
#include <stdint.h>
#include <stdbool.h>
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Algorithm Types
 ******************************************************************************/
typedef enum {
    ALGORITHM_NONE = 0,
    
    /* Quasi-Static Methods */
    ALGORITHM_QUASI_STATIC,
    ALGORITHM_MAGNETOSTATIC,
    ALGORITHM_ELECTROSTATIC,
    
    /* Full-Wave Methods */
    ALGORITHM_MOMENT_METHOD,
    ALGORITHM_FINITE_ELEMENT,
    ALGORITHM_FINITE_DIFFERENCE,
    ALGORITHM_FINITE_INTEGRATION,
    
    /* Specialized Methods */
    ALGORITHM_TRANSMISSION_LINE,
    ALGORITHM_CAVITY_MODE,
    ALGORITHM_WAVEGUIDE_MODE,
    ALGORITHM_ANTENNA_MODE,
    
    /* Hybrid Methods */
    ALGORITHM_HYBRID_MOM_FEM,
    ALGORITHM_HYBRID_FDTD_FEM,
    ALGORITHM_HYBRID_ML_MOM,
    
    /* Fast Methods */
    ALGORITHM_FAST_MULTIPOLE,
    ALGORITHM_ADAPTIVE_INTEGRAL,
    ALGORITHM_PRECORRECTED_FFT,
    
    /* Optimization Methods */
    ALGORITHM_GENETIC_ALGORITHM,
    ALGORITHM_PARTICLE_SWARM,
    ALGORITHM_GRADIENT_BASED,
    
    ALGORITHM_COUNT
} AlgorithmType;

/******************************************************************************
 * Algorithm Properties
 ******************************************************************************/
typedef struct {
    AlgorithmType type;
    const char* name;
    const char* description;
    
    /* Performance characteristics */
    double time_complexity;      /* O(n^p) where p is the exponent */
    double memory_complexity;    /* Memory scaling factor */
    double accuracy_level;       /* Typical accuracy (0-1) */
    double frequency_range_min;  /* Minimum frequency (Hz) */
    double frequency_range_max;  /* Maximum frequency (Hz) */
    
    /* Applicability */
    bool supports_2d;
    bool supports_3d;
    bool supports_periodic;
    bool supports_lossy;
    bool supports_dispersive;
    bool supports_nonlinear;
    
    /* Structure compatibility */
    bool supports_transmission_lines;
    bool supports_antennas;
    bool supports_vias;
    bool supports_packages;
    bool supports_metamaterials;
    bool supports_enclosures;
    
} AlgorithmProperties;

/******************************************************************************
 * Algorithm Selection Criteria
 ******************************************************************************/
typedef struct {
    /* Input requirements */
    StructureCategory structure_category;
    uint32_t structure_type;
    double frequency;
    double target_accuracy;
    double available_memory;
    double time_constraint;
    
    /* Problem characteristics */
    bool is_periodic;
    bool is_symmetric;
    bool is_multiscale;
    bool is_multiphysics;
    bool has_nonlinear_materials;
    bool has_dispersive_materials;
    
    /* Computational constraints */
    uint32_t max_iterations;
    double convergence_tolerance;
    bool parallel_enabled;
    uint32_t num_threads;
    
} AlgorithmSelectionCriteria;

/******************************************************************************
 * Algorithm Results
 ******************************************************************************/
typedef struct {
    /* Selected algorithm */
    AlgorithmType algorithm;
    double estimated_time;
    double estimated_memory;
    double expected_accuracy;
    double confidence_level;
    
    /* Alternative algorithms */
    AlgorithmType alternative1;
    AlgorithmType alternative2;
    double speedup_factor1;
    double speedup_factor2;
    
    /* Performance metrics */
    double computational_complexity;
    double memory_efficiency;
    double numerical_stability;
    
} AlgorithmSelectionResult;

/******************************************************************************
 * Transmission Line Algorithms
 ******************************************************************************/
typedef struct {
    /* Physical parameters */
    double width;
    double height;
    double thickness;
    double conductivity;
    double permittivity;
    double permeability;
    double loss_tangent;
    
    /* Frequency-dependent results */
    double frequency;
    double complex characteristic_impedance;
    double complex propagation_constant;
    double complex effective_permittivity;
    double attenuation_constant;
    double phase_constant;
    
    /* Quality factors */
    double conductor_loss;
    double dielectric_loss;
    double radiation_loss;
    double total_q_factor;
    
} TransmissionLineResults;

/******************************************************************************
 * Via Algorithms
 ******************************************************************************/
typedef struct {
    /* Physical parameters */
    double diameter;
    double height;
    double pad_diameter;
    double antipad_diameter;
    double conductivity;
    double plating_thickness;
    
    /* Electrical parameters */
    double complex inductance;
    double complex resistance;
    double complex capacitance;
    double complex impedance;
    
    /* Parasitic effects */
    double series_resistance;
    double series_inductance;
    double shunt_capacitance;
    double mutual_inductance;
    
    /* Frequency response */
    double self_resonant_frequency;
    double insertion_loss;
    double return_loss;
    
} ViaResults;

/******************************************************************************
 * Antenna Algorithms
 ******************************************************************************/
typedef struct {
    /* Far-field parameters */
    double gain;
    double directivity;
    double efficiency;
    double bandwidth;
    double beamwidth;
    double sidelobe_level;
    
    /* Input parameters */
    double complex input_impedance;
    double vswr;
    double return_loss;
    double radiation_resistance;
    double loss_resistance;
    
    /* Near-field parameters */
    double near_field_radius;
    double reactive_power;
    double radiated_power;
    double stored_energy;
    
    /* Pattern characteristics */
    double* theta_angles;        /* Array of theta angles (radians) */
    double* phi_angles;        /* Array of phi angles (radians) */
    double complex** e_theta;  /* E-theta component (theta x phi) */
    double complex** e_phi;      /* E-phi component (theta x phi) */
    uint32_t num_theta;
    uint32_t num_phi;
    
} AntennaResults;

/******************************************************************************
 * Metamaterial Algorithms
 ******************************************************************************/
typedef struct {
    /* Effective medium parameters */
    double complex effective_permittivity;
    double complex effective_permeability;
    double complex effective_conductivity;
    double complex effective_impedance;
    
    /* Refractive index */
    double complex refractive_index;
    double complex wave_impedance;
    
    /* Transmission/reflection */
    double complex transmission_coefficient;
    double complex reflection_coefficient;
    double transmission_magnitude;
    double reflection_magnitude;
    double transmission_phase;
    double reflection_phase;
    
    /* Resonance characteristics */
    double resonant_frequency;
    double quality_factor;
    double bandwidth;
    double insertion_loss;
    
    /* Dispersion characteristics */
    double* frequencies;
    double complex* permittivity_dispersion;
    double complex* permeability_dispersion;
    uint32_t num_frequencies;
    
} MetamaterialResults;

/******************************************************************************
 * Algorithm Selection Functions
 ******************************************************************************/

/* Algorithm selection */
AlgorithmSelectionResult select_optimal_algorithm(const AlgorithmSelectionCriteria* criteria);
AlgorithmProperties get_algorithm_properties(AlgorithmType algorithm);
bool is_algorithm_compatible(AlgorithmType algorithm, StructureCategory category, uint32_t type);

/* Algorithm comparison */
double estimate_algorithm_accuracy(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria);
double estimate_algorithm_time(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria);
double estimate_algorithm_memory(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria);

/* Specialized algorithm implementations */
TransmissionLineResults calculate_transmission_line(StructureCategory category, uint32_t type,
                                                   const double* parameters, uint32_t num_parameters,
                                                   double frequency, double accuracy);

ViaResults calculate_via_structure(StructureCategory category, uint32_t type,
                                 const double* parameters, uint32_t num_parameters,
                                 double frequency, double accuracy);

AntennaResults calculate_antenna_structure(StructureCategory category, uint32_t type,
                                         const double* parameters, uint32_t num_parameters,
                                         double frequency, double accuracy);

MetamaterialResults calculate_metamaterial_structure(StructureCategory category, uint32_t type,
                                                    const double* parameters, uint32_t num_parameters,
                                                    double frequency, double accuracy);

/* Adaptive algorithm selection */
AlgorithmType select_transmission_line_algorithm(const AlgorithmSelectionCriteria* criteria);
AlgorithmType select_via_algorithm(const AlgorithmSelectionCriteria* criteria);
AlgorithmType select_antenna_algorithm(const AlgorithmSelectionCriteria* criteria);
AlgorithmType select_metamaterial_algorithm(const AlgorithmSelectionCriteria* criteria);
AlgorithmType select_package_algorithm(const AlgorithmSelectionCriteria* criteria);

/* Algorithm validation */
bool validate_algorithm_selection(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria);
double get_algorithm_confidence(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria);
const char* get_algorithm_recommendation(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria);

/* Performance optimization */
double optimize_algorithm_parameters(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria,
                                  double* optimized_parameters, uint32_t num_parameters);

/* Algorithm benchmarking */
typedef struct {
    AlgorithmType algorithm;
    StructureCategory category;
    uint32_t type;
    double problem_size;
    double execution_time;
    double memory_usage;
    double accuracy_achieved;
    double convergence_rate;
    uint32_t iterations_required;
} AlgorithmBenchmark;

void benchmark_algorithm(AlgorithmType algorithm, const AlgorithmSelectionCriteria* criteria,
                        AlgorithmBenchmark* result);

/* Algorithm library management */
void initialize_algorithm_library(void);
void cleanup_algorithm_library(void);
bool register_custom_algorithm(const AlgorithmProperties* properties,
                              void* (*algorithm_function)(void*));

#ifdef __cplusplus
}
#endif

#endif /* STRUCTURE_ALGORITHMS_H */