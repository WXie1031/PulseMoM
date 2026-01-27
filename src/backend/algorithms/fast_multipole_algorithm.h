#ifndef FAST_MULTIPOLE_ALGORITHM_H
#define FAST_MULTIPOLE_ALGORITHM_H

#include "../complex.h"
#include "../matrix.h"
#include "../electromagnetic.h"
#include "../mom/mom_solver.h"

#define MAX_MULTIPOLE_LEVELS 10
#define MAX_MULTIPOLE_COEFFICIENTS 100
#define MAX_CLUSTER_SIZE 1000
#define MAX_NEAR_FIELD_INTERACTIONS 10000
#define MAX_FAR_FIELD_INTERACTIONS 100000

typedef enum {
    MULTIPOLE_TYPE_MONOPOLE,
    MULTIPOLE_TYPE_DIPOLE,
    MULTIPOLE_TYPE_QUADRUPOLE,
    MULTIPOLE_TYPE_OCTUPOLE,
    MULTIPOLE_TYPE_HEXADECAPOLE,
    MULTIPOLE_TYPE_SPHERICAL,
    MULTIPOLE_TYPE_CYLINDRICAL,
    MULTIPOLE_TYPE_CARTESIAN,
    MULTIPOLE_TYPE_ADAPTIVE,
    MULTIPOLE_TYPE_HIERARCHICAL
} MultipoleType;

typedef enum {
    FMM_ACCELERATION_TYPE_NONE,
    FMM_ACCELERATION_TYPE_SINGLE_LEVEL,
    FMM_ACCELERATION_TYPE_MULTI_LEVEL,
    FMP_ACCELERATION_TYPE_ADAPTIVE,
    FMM_ACCELERATION_TYPE_HIERARCHICAL,
    FMM_ACCELERATION_TYPE_HYBRID,
    FMM_ACCELERATION_TYPE_PARALLEL,
    FMM_ACCELERATION_TYPE_GPU
} FmmAccelerationType;

typedef enum {
    CLUSTER_TYPE_LEAF,
    CLUSTER_TYPE_INTERNAL,
    CLUSTER_TYPE_ROOT,
    CLUSTER_TYPE_GHOST,
    CLUSTER_TYPE_BOUNDARY
} ClusterType;

typedef enum {
    INTERACTION_TYPE_NEAR_FIELD,
    INTERACTION_TYPE_FAR_FIELD,
    INTERACTION_TYPE_SELF,
    INTERACTION_TYPE_COUPLED,
    INTERACTION_TYPE_DECOUPLED
} InteractionType;

typedef struct {
    double x, y, z;
    double radius;
    int level;
    int parent_id;
    int* children_ids;
    int num_children;
    int* source_ids;
    int num_sources;
    int* observation_ids;
    int num_observations;
    double bounding_box[6];
    double center_of_mass[3];
    Complex multipole_moments[MAX_MULTIPOLE_COEFFICIENTS];
    Complex local_expansion_coefficients[MAX_MULTIPOLE_COEFFICIENTS];
    int cluster_id;
    ClusterType type;
} FmmCluster;

typedef struct {
    int source_cluster_id;
    int observation_cluster_id;
    InteractionType interaction_type;
    double interaction_strength;
    Complex interaction_matrix[MAX_MULTIPOLE_COEFFICIENTS][MAX_MULTIPOLE_COEFFICIENTS];
    int interaction_level;
    int is_valid;
} FmmInteraction;

typedef struct {
    Complex monopole;
    Complex dipole[3];
    Complex quadrupole[3][3];
    Complex octupole[3][3][3];
    Complex hexadecapole[3][3][3][3];
    int expansion_order;
    double expansion_center[3];
    double convergence_radius;
    int is_converged;
} MultipoleExpansion;

typedef struct {
    Complex spherical_hankel[MAX_MULTIPOLE_COEFFICIENTS];
    Complex spherical_bessel[MAX_MULTIPOLE_COEFFICIENTS];
    Complex spherical_harmonics[MAX_MULTIPOLE_COEFFICIENTS][MAX_MULTIPOLE_COEFFICIENTS];
    Complex legendre_polynomials[MAX_MULTIPOLE_COEFFICIENTS][MAX_MULTIPOLE_COEFFICIENTS];
    Complex wigner_3j_symbols[MAX_MULTIPOLE_COEFFICIENTS][MAX_MULTIPOLE_COEFFICIENTS][MAX_MULTIPOLE_COEFFICIENTS];
    double spherical_coordinates[3];
    int max_order;
    int is_precomputed;
} SphericalHarmonics;

typedef struct {
    double frequency;
    double wavelength;
    double k0;
    double eta0;
    Complex propagation_constant;
    Complex wave_impedance;
    double skin_depth;
    double surface_resistance;
} FmmWaveParameters;

typedef struct {
    FmmAccelerationType acceleration_type;
    int max_multipole_order;
    int max_tree_levels;
    double cluster_size_threshold;
    double far_field_threshold;
    double near_field_threshold;
    double convergence_tolerance;
    int max_iterations;
    int use_preconditioner;
    int use_parallel_processing;
    int use_gpu_acceleration;
    int num_threads;
    int use_adaptive_order;
    int use_error_control;
    double target_accuracy;
} FastMultipoleConfig;

typedef struct {
    FastMultipoleConfig config;
    FmmCluster clusters[MAX_CLUSTER_SIZE];
    int num_clusters;
    FmmInteraction interactions[MAX_FAR_FIELD_INTERACTIONS];
    int num_interactions;
    MultipoleExpansion multipole_expansions[MAX_CLUSTER_SIZE];
    SphericalHarmonics spherical_harmonics;
    FmmWaveParameters wave_params;
    MomSolver* mom_solver;
    Complex* impedance_matrix;
    Complex* near_field_matrix;
    Complex* far_field_matrix;
    Complex* translation_operators;
    Complex* interpolation_matrices;
    double* source_positions;
    double* observation_positions;
    Complex* source_currents;
    Complex* observation_fields;
    int num_sources;
    int num_observations;
    int matrix_size;
    int tree_depth;
    int calculation_completed;
    double computation_time;
    double memory_usage;
    int convergence_status;
    double achieved_accuracy;
    int num_matrix_vector_products;
} FastMultipoleSolver;

FastMultipoleSolver* fast_multipole_create(FastMultipoleConfig* config);
void fast_multipole_destroy(FastMultipoleSolver* solver);

int fast_multipole_simulate(FastMultipoleSolver* solver);
int fast_multipole_solve_frequency(FastMultipoleSolver* solver, double frequency);
int fast_multipole_solve_frequency_sweep(FastMultipoleSolver* solver);

int fast_multipole_setup_sources(FastMultipoleSolver* solver, double* positions, Complex* currents, int num_sources);
int fast_multipole_setup_observations(FastMultipoleSolver* solver, double* positions, int num_observations);
int fast_multipole_setup_geometry(FastMultipoleSolver* solver, double* geometry_data, int geometry_size);

int fast_multipole_build_tree(FastMultipoleSolver* solver);
int fast_multipole_partition_space(FastMultipoleSolver* solver);
int fast_multipole_compute_clustering(FastMultipoleSolver* solver);

int fast_multipole_compute_multipole_expansions(FastMultipoleSolver* solver);
int fast_multipole_compute_local_expansions(FastMultipoleSolver* solver);
int fast_multipole_compute_translation_operators(FastMultipoleSolver* solver);

int fast_multipole_upward_pass(FastMultipoleSolver* solver);
int fast_multipole_downward_pass(FastMultipoleSolver* solver);
int fast_multipole_evaluation_pass(FastMultipoleSolver* solver);

int fast_multipole_compute_near_field_interactions(FastMultipoleSolver* solver);
int fast_multipole_compute_far_field_interactions(FastMultipoleSolver* solver);
int fast_multipole_compute_interaction_matrix(FastMultipoleSolver* solver);

int fast_multipole_matrix_vector_product(FastMultipoleSolver* solver, Complex* input_vector, Complex* output_vector);
int fast_multipole_precondition(FastMultipoleSolver* solver, Complex* vector, Complex* preconditioned_vector);

int fast_multipole_solve_iterative(FastMultipoleSolver* solver, Complex* rhs_vector, Complex* solution_vector);
int fast_multipole_solve_direct(FastMultipoleSolver* solver, Complex* rhs_vector, Complex* solution_vector);

int fast_multipole_validate_convergence(FastMultipoleSolver* solver);
int fast_multipole_estimate_error(FastMultipoleSolver* solver);
int fast_multipole_adaptive_refinement(FastMultipoleSolver* solver);

MultipoleExpansion fast_multipole_compute_multipole_moments(double* source_positions, Complex* source_currents, int num_sources, double* expansion_center, int expansion_order);
Complex fast_multipole_compute_green_function(FastMultipoleSolver* solver, double* source_pos, double* observation_pos);
Complex fast_multipole_compute_interaction_term(FastMultipoleSolver* solver, int source_cluster, int observation_cluster);

int fast_multipole_precompute_spherical_harmonics(FastMultipoleSolver* solver);
int fast_multipole_precompute_translation_operators(FastMultipoleSolver* solver);
int fast_multipole_precompute_interpolation_matrices(FastMultipoleSolver* solver);

void fast_multipole_get_field_distribution(FastMultipoleSolver* solver, double* positions, Complex* fields, int num_positions);
void fast_multipole_get_current_distribution(FastMultipoleSolver* solver, Complex* currents, int num_currents);
void fast_multipole_get_scattering_parameters(FastMultipoleSolver* solver, double frequency, Complex*** s_matrix);

int fast_multipole_export_results(FastMultipoleSolver* solver, const char* filename, const char* format);
int fast_multipole_import_geometry(FastMultipoleSolver* solver, const char* filename);

void fast_multipole_print_summary(FastMultipoleSolver* solver);
void fast_multipole_benchmark(FastMultipoleSolver* solver);

Complex spherical_hankel_function(int n, Complex z);
Complex spherical_bessel_function(int n, Complex z);
Complex spherical_harmonic_function(int l, int m, double theta, double phi);
Complex legendre_polynomial(int n, double x);

int fast_multipole_parallel_setup(FastMultipoleSolver* solver);
int fast_multipole_parallel_matrix_vector_product(FastMultipoleSolver* solver, Complex* input_vector, Complex* output_vector);
int fast_multipole_parallel_tree_construction(FastMultipoleSolver* solver);

#endif