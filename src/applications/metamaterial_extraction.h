#ifndef METAMATERIAL_EXTRACTION_H
#define METAMATERIAL_EXTRACTION_H

#include "../complex.h"
#include "../matrix.h"
#include "../electromagnetic.h"
#include "../mom/mom_solver.h"
#include "../solvers/mom/mom_fullwave_solver.h"

#define MAX_METAMATERIAL_CELLS 1000
#define MAX_UNIT_CELL_TYPES 50
#define MAX_FREQUENCY_POINTS_MM 500
#define MAX_PARAMETER_EXTRACTION_METHODS 10

typedef enum {
    METAMATERIAL_TYPE_SRR,           
    METAMATERIAL_TYPE_CSRR,          
    METAMATERIAL_TYPE_WIRE_MEDIUM,   
    METAMATERIAL_TYPE_SPLIT_RING,    
    METAMATERIAL_TYPE_OMEGA_PARTICLE,
    METAMATERIAL_TYPE_SPIRAL,        
    METAMATERIAL_TYPE_FRACTAL,       
    METAMATERIAL_TYPE_COMPLEMENTARY, 
    METAMATERIAL_TYPE_BIANISOTROPIC, 
    METAMATERIAL_TYPE_CHIRAL,        
    METAMATERIAL_TYPE_ANISOTROPIC,   
    METAMATERIAL_TYPE_HYPERBOLIC,    
    METAMATERIAL_TYPE_EPSILON_NEAR_ZERO,
    METAMATERIAL_TYPE_MU_NEAR_ZERO,  
    METAMATERIAL_TYPE_DOUBLE_NEGATIVE,
    METAMATERIAL_TYPE_INDEFINITE,    
    METAMATERIAL_TYPE_CUSTOM
} MetamaterialType;

typedef enum {
    EXTRACTION_METHOD_NRW,           
    EXTRACTION_METHOD_S_PARAMETER,   
    EXTRACTION_METHOD_RETRIEVAL,     
    EXTRACTION_METHOD_INVERSION,     
    EXTRACTION_METHOD_OPTIMIZATION,  
    EXTRACTION_METHOD_BLOCH,         
    EXTRACTION_METHOD_EFFECTIVE_MEDIUM,
    EXTRACTION_METHOD_CLAUSIUS_MOSSOTTI,
    EXTRACTION_METHOD_MAXWELL_GARNETT,
    EXTRACTION_METHOD_MIXING_RULES
} ExtractionMethod;

typedef enum {
    BOUNDARY_CONDITION_PERIODIC,
    BOUNDARY_CONDITION_PEC,
    BOUNDARY_CONDITION_PMC,
    BOUNDARY_CONDITION_ABC,
    BOUNDARY_CONDITION_PML,
    BOUNDARY_CONDITION_EBG,
    BOUNDARY_CONDITION_HIGGS
} BoundaryConditionType;

typedef struct {
    double length, width, height;
    double periodicity_x, periodicity_y, periodicity_z;
    double substrate_thickness;
    double substrate_permittivity;
    double substrate_conductivity;
    double conductor_thickness;
    double conductor_conductivity;
    double gap_size;
    double strip_width;
    double inner_radius;
    double outer_radius;
    double number_of_turns;
    double chirality_parameter;
    double anisotropy_ratio;
    double loss_tangent;
} UnitCellGeometry;

typedef struct {
    MetamaterialType type;
    UnitCellGeometry geometry;
    Complex epsilon_eff[MAX_FREQUENCY_POINTS_MM];
    Complex mu_eff[MAX_FREQUENCY_POINTS_MM];
    Complex eta_eff[MAX_FREQUENCY_POINTS_MM];
    Complex n_eff[MAX_FREQUENCY_POINTS_MM];
    Complex z_eff[MAX_FREQUENCY_POINTS_MM];
    double chirality[MAX_FREQUENCY_POINTS_MM];
    double frequency_vector[MAX_FREQUENCY_POINTS_MM];
    int num_frequencies;
    double frequency_min, frequency_max;
    int num_unit_cells;
    int num_periods_x, num_periods_y, num_periods_z;
    BoundaryConditionType boundary_condition;
    ExtractionMethod extraction_method;
    int use_full_wave_simulation;
    int use_periodic_boundary_conditions;
    int use_bloch_floquet;
    int use_anisotropic_model;
    int use_dispersive_model;
    int use_nonlinear_model;
    double convergence_tolerance;
    int max_iterations;
} MetamaterialExtractionConfig;

typedef struct {
    Complex s11, s12, s21, s22;
    double frequency;
    double incident_angle_theta;
    double incident_angle_phi;
    Complex polarization_vector[3];
    double power_incident;
    double power_reflected;
    double power_transmitted;
} SParameters;

typedef struct {
    Complex epsilon_tensor[3][3];
    Complex mu_tensor[3][3];
    Complex xi_tensor[3][3];
    Complex zeta_tensor[3][3];
    Complex epsilon_xx, epsilon_xy, epsilon_xz;
    Complex epsilon_yx, epsilon_yy, epsilon_yz;
    Complex epsilon_zx, epsilon_zy, epsilon_zz;
    Complex mu_xx, mu_xy, mu_xz;
    Complex mu_yx, mu_yy, mu_yz;
    Complex mu_zx, mu_zy, mu_zz;
    Complex xi_xx, xi_xy, xi_xz;
    Complex xi_yx, xi_yy, xi_yz;
    Complex xi_zx, xi_zy, xi_zz;
    Complex zeta_xx, zeta_xy, zeta_xz;
    Complex zeta_yx, zeta_yy, zeta_yz;
    Complex zeta_zx, zeta_zy, zeta_zz;
    int is_anisotropic;
    int is_bianisotropic;
    int is_chiral;
} EffectiveParameters;

typedef struct {
    double resonance_frequency;
    double plasma_frequency;
    double damping_factor;
    double oscillator_strength;
    double coupling_coefficient;
    double magnetic_coupling;
    double electric_coupling;
    double quality_factor;
    double bandwidth;
    double insertion_loss;
    double reflection_loss;
    double transmission_coefficient;
    double reflection_coefficient;
    double absorption_coefficient;
    double figure_of_merit;
    double refractive_index_contrast;
    double impedance_matching;
} MetamaterialMetrics;

typedef struct {
    MetamaterialExtractionConfig config;
    UnitCellGeometry unit_cells[MAX_UNIT_CELL_TYPES];
    int num_unit_cell_types;
    SParameters s_params[MAX_FREQUENCY_POINTS_MM];
    EffectiveParameters effective_params[MAX_FREQUENCY_POINTS_MM];
    MetamaterialMetrics metrics[MAX_FREQUENCY_POINTS_MM];
    AntennaFullwaveSolver* fullwave_solver;
    MomSolver* mom_solver;
    Complex* scattering_matrix;
    Complex* impedance_matrix;
    Complex* admittance_matrix;
    double* bloch_vector;
    int matrix_size;
    int extraction_completed;
    double computation_time;
    double memory_usage;
    int convergence_status;
} MetamaterialExtractionSolver;

typedef struct {
    Complex epsilon_host;
    Complex mu_host;
    Complex epsilon_inclusion;
    Complex mu_inclusion;
    double volume_fraction;
    double particle_shape_factor;
    double depolarization_factors[3];
    double mixing_rule_parameter;
    int inclusion_type;
} EffectiveMediumParameters;

MetamaterialExtractionSolver* metamaterial_extraction_create(MetamaterialExtractionConfig* config);
void metamaterial_extraction_destroy(MetamaterialExtractionSolver* solver);

int metamaterial_extraction_simulate(MetamaterialExtractionSolver* solver);
int metamaterial_extraction_solve_frequency(MetamaterialExtractionSolver* solver, double frequency);
int metamaterial_extraction_solve_frequency_sweep(MetamaterialExtractionSolver* solver);

int metamaterial_setup_srr(MetamaterialExtractionSolver* solver, double outer_radius, double inner_radius, double gap_size);
int metamaterial_setup_csrr(MetamaterialExtractionSolver* solver, double outer_radius, double inner_radius, double slot_width);
int metamaterial_setup_wire_medium(MetamaterialExtractionSolver* solver, double wire_radius, double wire_length, double wire_spacing);
int metamaterial_setup_omega_particle(MetamaterialExtractionSolver* solver, double radius, int num_arms);
int metamaterial_setup_chiral_structure(MetamaterialExtractionSolver* solver, double pitch, double radius, int num_turns);

int metamaterial_compute_s_parameters(MetamaterialExtractionSolver* solver, double frequency);
int metamaterial_extract_epsilon_mu_nrw(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx);
int metamaterial_extract_epsilon_mu_retrieval(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx);
int metamaterial_extract_epsilon_mu_optimization(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx);
int metamaterial_extract_bloch_properties(MetamaterialExtractionSolver* solver, double frequency);

int metamaterial_extract_anisotropic_parameters(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx);
int metamaterial_extract_bianisotropic_parameters(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx);
int metamaterial_extract_chiral_parameters(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx);

int metamaterial_apply_clausius_mossotti(MetamaterialExtractionSolver* solver, EffectiveMediumParameters* params, int freq_idx);
int metamaterial_apply_maxwell_garnett(MetamaterialExtractionSolver* solver, EffectiveMediumParameters* params, int freq_idx);
int metamaterial_apply_bruggeman(MetamaterialExtractionSolver* solver, EffectiveMediumParameters* params, int freq_idx);
int metamaterial_apply_looyenga(MetamaterialExtractionSolver* solver, EffectiveMediumParameters* params, int freq_idx);

int metamaterial_compute_effective_permittivity(MetamaterialExtractionSolver* solver, double frequency);
int metamaterial_compute_effective_permeability(MetamaterialExtractionSolver* solver, double frequency);
int metamaterial_compute_effective_refractive_index(MetamaterialExtractionSolver* solver, double frequency);
int metamaterial_compute_effective_impedance(MetamaterialExtractionSolver* solver, double frequency);

int metamaterial_compute_resonance_frequency(MetamaterialExtractionSolver* solver);
int metamaterial_compute_plasma_frequency(MetamaterialExtractionSolver* solver);
int metamaterial_compute_quality_factor(MetamaterialExtractionSolver* solver);
int metamaterial_compute_bandwidth(MetamaterialExtractionSolver* solver);

int metamaterial_validate_extracted_parameters(MetamaterialExtractionSolver* solver, int freq_idx);
int metamaterial_check_passivity(MetamaterialExtractionSolver* solver, int freq_idx);
int metamaterial_check_causality(MetamaterialExtractionSolver* solver, int freq_idx);
int metamaterial_check_energy_conservation(MetamaterialExtractionSolver* solver, int freq_idx);

int metamaterial_optimize_geometry(MetamaterialExtractionSolver* solver, double target_frequency);
int metamaterial_optimize_performance(MetamaterialExtractionSolver* solver, double target_fom);
int metamaterial_adaptive_mesh_refinement(MetamaterialExtractionSolver* solver, double error_threshold);

void metamaterial_get_effective_parameters(MetamaterialExtractionSolver* solver, double frequency, EffectiveParameters* params);
void metamaterial_get_s_parameters(MetamaterialExtractionSolver* solver, double frequency, SParameters* params);
void metamaterial_get_metrics(MetamaterialExtractionSolver* solver, double frequency, MetamaterialMetrics* metrics);

int metamaterial_export_results(MetamaterialExtractionSolver* solver, const char* filename, const char* format);
int metamaterial_import_geometry(MetamaterialExtractionSolver* solver, const char* filename);

void metamaterial_print_summary(MetamaterialExtractionSolver* solver);
void metamaterial_benchmark(MetamaterialExtractionSolver* solver);

Complex metamaterial_nicolson_ross_weir_epsilon(SParameters* s_params, double thickness, double frequency);
Complex metamaterial_nicolson_ross_weir_mu(SParameters* s_params, double thickness, double frequency);
Complex metamaterial_nicolson_ross_weir_eta(SParameters* s_params, double thickness, double frequency);

Complex metamaterial_retrieval_epsilon(SParameters* s_params, double thickness, double frequency);
Complex metamaterial_retrieval_mu(SParameters* s_params, double thickness, double frequency);
Complex metamaterial_retrieval_n(SParameters* s_params, double thickness, double frequency);

int metamaterial_kramers_kronig_check(Complex* epsilon, Complex* mu, int num_freqs, double* frequencies);
int metamaterial_dispersion_analysis(MetamaterialExtractionSolver* solver);

#endif