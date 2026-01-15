#ifndef ENCLOSURE_CALCULATION_H
#define ENCLOSURE_CALCULATION_H

#include "../complex.h"
#include "../matrix.h"
#include "../electromagnetic.h"
#include "../mom/mom_solver.h"

#define MAX_ENCLOSURE_SEGMENTS 50000
#define MAX_FREQUENCY_POINTS_ENC 1000
#define MAX_CAVITY_MODES 1000
#define MAX_APERTURE_TYPES 50
#define MAX_CONDUCTOR_TYPES 20

typedef enum {
    ENCLOSURE_TYPE_RECTANGULAR,
    ENCLOSURE_TYPE_CIRCULAR,
    ENCLOSURE_TYPE_ELLIPTICAL,
    ENCLOSURE_TYPE_TRIANGULAR,
    ENCLOSURE_TYPE_HONEYCOMB,
    ENCLOSURE_TYPE_WAVEGUIDE,
    ENCLOSURE_TYPE_COAXIAL,
    ENCLOSURE_TYPE_MICROSTRIP,
    ENCLOSURE_TYPE_STRIPLINE,
    ENCLOSURE_TYPE_COPLANAR,
    ENCLOSURE_TYPE_SLOTLINE,
    ENCLOSURE_TYPE_FINLINE,
    ENCLOSURE_TYPE_RIDGE,
    ENCLOSURE_TYPE corrugated,
    ENCLOSURE_TYPE_LOSSY,
    ENCLOSURE_TYPE_ANISOTROPIC,
    ENCLOSURE_TYPE_INHOMOGENEOUS,
    ENCLOSURE_TYPE_MULTILAYER,
    ENCLOSURE_TYPE_PERIODIC,
    ENCLOSURE_TYPE_CUSTOM
} EnclosureType;

typedef enum {
    CAVITY_MODE_TE,
    CAVITY_MODE_TM,
    CAVITY_MODE_TEM,
    CAVITY_MODE_HYBRID,
    CAVITY_MODE_QUASI_TE,
    CAVITY_MODE_QUASI_TM,
    CAVITY_MODE_EVANESCENT,
    CAVITY_MODE_COMPLEX,
    CAVITY_MODE_DEGENERATE,
    CAVITY_MODE_LEAKY
} CavityModeType;

typedef enum {
    BOUNDARY_CONDITION_PERFECT_ELECTRIC_CONDUCTOR,
    BOUNDARY_CONDITION_PERFECT_MAGNETIC_CONDUCTOR,
    BOUNDARY_CONDITION_IMPEDANCE,
    BOUNDARY_CONDITION_ABSORBING,
    BOUNDARY_CONDITION_RADIATING,
    BOUNDARY_CONDITION_PERIODIC,
    BOUNDARY_CONDITION_SYMMETRIC,
    BOUNDARY_CONDITION_ANTISYMMETRIC,
    BOUNDARY_CONDITION_MIXED,
    BOUNDARY_CONDITION_LEAKY
} EnclosureBoundaryCondition;

typedef struct {
    double x, y, z;
    double length, width, height;
    double radius;
    double thickness;
    Complex permittivity;
    Complex permeability;
    double conductivity;
    double loss_tangent;
    EnclosureBoundaryCondition boundary_condition;
    int segment_id;
    int connected_to[10];
    int num_connections;
} EnclosureSegment;

typedef struct {
    int m, n, p;
    double frequency;
    Complex propagation_constant;
    Complex wave_impedance;
    double quality_factor;
    double attenuation;
    double phase_velocity;
    double group_velocity;
    double power_handling;
    CavityModeType mode_type;
    int is_propagating;
    int is_resonant;
    int is_evanescent;
    int is_degenerate;
} CavityMode;

typedef struct {
    double x, y, z;
    double length, width;
    double area;
    double perimeter;
    Complex equivalent_impedance;
    Complex equivalent_admittance;
    double radiation_resistance;
    double leakage_coefficient;
    int aperture_id;
    int is_radiating;
    int is_leaking;
    int is_coupling;
} ApertureStructure;

typedef struct {
    double x, y, z;
    double length, width, height;
    double conductivity;
    double permeability;
    double surface_impedance;
    double skin_depth;
    double loss_factor;
    Complex current_density;
    Complex magnetic_field;
    Complex electric_field;
    int conductor_id;
    int is_ground;
    int is_signal;
    int is_power;
} ConductorStructure;

typedef struct {
    double frequency;
    Complex input_impedance;
    Complex reflection_coefficient;
    Complex transmission_coefficient;
    Complex scattering_matrix[4][4];
    double return_loss;
    double insertion_loss;
    double isolation;
    double coupling;
    double resonance_frequency;
    double bandwidth;
    double quality_factor;
    double power_handling;
    double shielding_effectiveness;
} EnclosureParameters;

typedef struct {
    double center_frequency;
    double bandwidth;
    double insertion_loss;
    double return_loss;
    double isolation;
    double coupling;
    double shielding_effectiveness;
    double power_handling_capability;
    double quality_factor;
    double spurious_free_range;
    double harmonic_distortion;
    double intermodulation_distortion;
} EnclosureMetrics;

typedef struct {
    EnclosureType type;
    double frequency_min, frequency_max;
    int num_frequencies;
    double enclosure_length, enclosure_width, enclosure_height;
    double wall_thickness;
    double conductivity;
    double permeability;
    Complex wall_permittivity;
    Complex wall_permeability;
    double loss_tangent;
    double surface_roughness;
    int num_segments;
    int num_cavity_modes;
    int num_apertures;
    int num_conductors;
    double mesh_density;
    double convergence_tolerance;
    int max_iterations;
    int use_full_wave_analysis;
    int use_cavity_model;
    int use_transmission_line_model;
    int use_mode_matching;
    int use_coupled_integral_equations;
    int use_fast_multipole_method;
    int use_preconditioner;
    int use_parallel_computation;
    int num_threads;
} EnclosureCalculationConfig;

typedef struct {
    EnclosureCalculationConfig config;
    EnclosureSegment segments[MAX_ENCLOSURE_SEGMENTS];
    int num_segments;
    CavityMode cavity_modes[MAX_CAVITY_MODES];
    int num_cavity_modes;
    ApertureStructure apertures[MAX_APERTURE_TYPES];
    int num_apertures;
    ConductorStructure conductors[MAX_CONDUCTOR_TYPES];
    int num_conductors;
    EnclosureParameters params[MAX_FREQUENCY_POINTS_ENC];
    int num_frequencies;
    EnclosureMetrics metrics;
    MomSolver* mom_solver;
    Complex* impedance_matrix;
    Complex* admittance_matrix;
    Complex* scattering_matrix;
    double* frequency_vector;
    int matrix_size;
    int calculation_completed;
    double computation_time;
    double memory_usage;
    int convergence_status;
} EnclosureCalculationSolver;

EnclosureCalculationSolver* enclosure_calculation_create(EnclosureCalculationConfig* config);
void enclosure_calculation_destroy(EnclosureCalculationSolver* solver);

int enclosure_calculation_simulate(EnclosureCalculationSolver* solver);
int enclosure_calculation_solve_frequency(EnclosureCalculationSolver* solver, double frequency);
int enclosure_calculation_solve_frequency_sweep(EnclosureCalculationSolver* solver);

int enclosure_setup_rectangular(EnclosureCalculationSolver* solver, double length, double width, double height);
int enclosure_setup_circular(EnclosureCalculationSolver* solver, double radius, double height);
int enclosure_setup_waveguide(EnclosureCalculationSolver* solver, double width, double height, double length);
int enclosure_setup_coaxial(EnclosureCalculationSolver* solver, double inner_radius, double outer_radius, double length);
int enclosure_setup_microstrip(EnclosureCalculationSolver* solver, double substrate_height, double enclosure_height);

int enclosure_add_aperture(EnclosureCalculationSolver* solver, double x, double y, double z, double length, double width);
int enclosure_add_conductor(EnclosureCalculationSolver* solver, double x, double y, double z, double length, double width, double height);
int enclosure_add_ground_plane(EnclosureCalculationSolver* solver, double z_position, double conductivity);

int enclosure_compute_cavity_modes(EnclosureCalculationSolver* solver, double frequency);
int enclosure_compute_cavity_resonances(EnclosureCalculationSolver* solver);
int enclosure_compute_mode_coupling(EnclosureCalculationSolver* solver);
int enclosure_compute_field_distribution(EnclosureCalculationSolver* solver, double frequency);

int enclosure_compute_impedance_matrix(EnclosureCalculationSolver* solver, double frequency);
int enclosure_compute_scattering_matrix(EnclosureCalculationSolver* solver, double frequency);
int enclosure_compute_admittance_matrix(EnclosureCalculationSolver* solver, double frequency);

int enclosure_compute_shielding_effectiveness(EnclosureCalculationSolver* solver, double frequency);
int enclosure_compute_power_handling(EnclosureCalculationSolver* solver, double frequency);
int enclosure_compute_quality_factor(EnclosureCalculationSolver* solver, double frequency);
int enclosure_compute_insertion_loss(EnclosureCalculationSolver* solver, double frequency);

int enclosure_compute_aperture_coupling(EnclosureCalculationSolver* solver, int aperture1, int aperture2);
int enclosure_compute_aperture_radiation(EnclosureCalculationSolver* solver, int aperture_id);
int enclosure_compute_aperture_leakage(EnclosureCalculationSolver* solver, int aperture_id);

int enclosure_validate_cavity_modes(EnclosureCalculationSolver* solver);
int enclosure_validate_field_distribution(EnclosureCalculationSolver* solver);
int enclosure_validate_boundary_conditions(EnclosureCalculationSolver* solver);
int enclosure_validate_power_conservation(EnclosureCalculationSolver* solver);

int enclosure_optimize_geometry(EnclosureCalculationSolver* solver, double target_frequency);
int enclosure_optimize_shielding(EnclosureCalculationSolver* solver, double target_se);
int enclosure_adaptive_mesh_refinement(EnclosureCalculationSolver* solver, double error_threshold);

void enclosure_get_cavity_modes(EnclosureCalculationSolver* solver, CavityMode** modes, int* num_modes);
void enclosure_get_field_distribution(EnclosureCalculationSolver* solver, double x, double y, double z, Complex* e_field, Complex* h_field);
void enclosure_get_scattering_parameters(EnclosureCalculationSolver* solver, double frequency, Complex*** s_matrix);

int enclosure_export_results(EnclosureCalculationSolver* solver, const char* filename, const char* format);
int enclosure_import_geometry(EnclosureCalculationSolver* solver, const char* filename);

void enclosure_print_summary(EnclosureCalculationSolver* solver);
void enclosure_benchmark(EnclosureCalculationSolver* solver);

#endif