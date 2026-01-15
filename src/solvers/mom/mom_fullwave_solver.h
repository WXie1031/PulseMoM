#ifndef ANTENNA_FULLWAVE_H
#define ANTENNA_FULLWAVE_H

#include "../complex.h"
#include "../matrix.h"
#include "../electromagnetic.h"
#include "../mom/mom_solver.h"

#define MAX_ANTENNA_SEGMENTS 100000
#define MAX_FREQUENCY_POINTS 1000
#define MAX_ANTENNA_PARAMS 50
#define NEAR_FIELD_POINTS 10000
#define FAR_FIELD_ANGLES 361

typedef enum {
    ANTENNA_TYPE_DIPOLE,
    ANTENNA_TYPE_MONOPOLE,
    ANTENNA_TYPE_LOOP,
    ANTENNA_TYPE_MICROSTRIP,
    ANTENNA_TYPE_PATCH,
    ANTENNA_TYPE_HORN,
    ANTENNA_TYPE_YAGI,
    ANTENNA_TYPE_SPIRAL,
    ANTENNA_TYPE_LOG_PERIODIC,
    ANTENNA_TYPE_ARRAY,
    ANTENNA_TYPE_FRACTAL,
    ANTENNA_TYPE_METAMATERIAL,
    ANTENNA_TYPE_CUSTOM
} AntennaType;

typedef enum {
    FEED_TYPE_DELTA_GAP,
    FEED_TYPE_MAGNETIC_FRILL,
    FEED_TYPE_VOLTAGE_SOURCE,
    FEED_TYPE_CURRENT_SOURCE,
    FEED_TYPE_WAVEGUIDE_PORT,
    FEED_TYPE_COAXIAL_PORT,
    FEED_TYPE_MICROSTRIP_PORT
} FeedType;

typedef enum {
    GROUND_TYPE_NONE,
    GROUND_TYPE_PERFECT_ELECTRIC_CONDUCTOR,
    GROUND_TYPE_PERFECT_MAGNETIC_CONDUCTOR,
    GROUND_TYPE_IMPEDANCE,
    GROUND_TYPE_REALISTIC
} GroundType;

typedef struct {
    double x, y, z;
    double radius;
    double length;
    Complex impedance;
    int segment_id;
    int connected_to[10];
    int num_connections;
} AntennaSegment;

typedef struct {
    double frequency;
    Complex gamma;
    Complex zin;
    double vswr;
    double return_loss;
    double radiation_efficiency;
    double directivity;
    double gain;
    double realized_gain;
    double beamwidth_e, beamwidth_h;
    double sidelobe_level;
    double front_to_back_ratio;
    double axial_ratio;
    double polarization_ratio;
} AntennaParameters;

typedef struct {
    double theta, phi;
    double r;
    Complex e_theta, e_phi;
    Complex h_theta, h_phi;
    double power_density;
    Complex poynting_vector[3];
} FarFieldPoint;

typedef struct {
    double x, y, z;
    Complex e_field[3];
    Complex h_field[3];
    Complex j_current[3];
    Complex m_current[3];
    double power_density;
} NearFieldPoint;

typedef struct {
    double center_freq;
    double bandwidth;
    double q_factor;
    double radiation_resistance;
    double loss_resistance;
    double efficiency;
    double directivity;
    double gain;
    double noise_temperature;
    double g_over_t;
} AntennaMetrics;

typedef struct {
    AntennaType type;
    FeedType feed_type;
    GroundType ground_type;
    double frequency_min, frequency_max;
    int num_frequencies;
    double substrate_height;
    double substrate_permittivity;
    double substrate_conductivity;
    double conductor_conductivity;
    double conductor_thickness;
    double ground_plane_size;
    int num_segments;
    int num_near_field_points;
    int num_far_field_angles;
    double mesh_density;
    double convergence_tolerance;
    int max_iterations;
    int use_adaptive_mesh;
    int use_fast_multipole;
    int use_preconditioner;
    int use_parallel_computation;
    int num_threads;
} AntennaSimulationConfig;

typedef struct {
    AntennaSimulationConfig config;
    AntennaSegment segments[MAX_ANTENNA_SEGMENTS];
    int num_segments;
    AntennaParameters params[MAX_FREQUENCY_POINTS];
    int num_frequencies;
    FarFieldPoint far_field[FAR_FIELD_ANGLES][FAR_FIELD_ANGLES];
    NearFieldPoint near_field[NEAR_FIELD_POINTS];
    int num_near_field_points;
    AntennaMetrics metrics;
    MomSolver* mom_solver;
    Complex* impedance_matrix;
    Complex* voltage_vector;
    Complex* current_vector;
    double* frequency_vector;
    int matrix_size;
    int simulation_completed;
    double computation_time;
    double memory_usage;
    int convergence_status;
} AntennaFullwaveSolver;

typedef struct {
    double frequency;
    double wavelength;
    double k0;
    double eta0;
    Complex propagation_constant;
    Complex characteristic_impedance;
    double skin_depth;
    double surface_resistance;
} WaveParameters;

AntennaFullwaveSolver* antenna_fullwave_create(AntennaSimulationConfig* config);
void antenna_fullwave_destroy(AntennaFullwaveSolver* solver);

int antenna_fullwave_simulate(AntennaFullwaveSolver* solver);
int antenna_fullwave_solve_frequency(AntennaFullwaveSolver* solver, double frequency);
int antenna_fullwave_solve_frequency_sweep(AntennaFullwaveSolver* solver);

int antenna_fullwave_setup_dipole(AntennaFullwaveSolver* solver, double length, double radius, double height);
int antenna_fullwave_setup_monopole(AntennaFullwaveSolver* solver, double length, double radius);
int antenna_fullwave_setup_loop(AntennaFullwaveSolver* solver, double radius, double wire_radius);
int antenna_fullwave_setup_patch(AntennaFullwaveSolver* solver, double length, double width, double height);
int antenna_fullwave_setup_horn(AntennaFullwaveSolver* solver, double a, double b, double length);
int antenna_fullwave_setup_array(AntennaFullwaveSolver* solver, int num_elements, double spacing);

int antenna_fullwave_add_feed(AntennaFullwaveSolver* solver, double x, double y, double z, FeedType type);
int antenna_fullwave_add_load(AntennaFullwaveSolver* solver, double x, double y, double z, Complex impedance);
int antenna_fullwave_add_ground(AntennaFullwaveSolver* solver, GroundType type, double height);

int antenna_fullwave_compute_impedance_matrix(AntennaFullwaveSolver* solver, double frequency);
int antenna_fullwave_compute_voltage_vector(AntennaFullwaveSolver* solver, double frequency);
int antenna_fullwave_solve_current_distribution(AntennaFullwaveSolver* solver);

int antenna_fullwave_compute_far_field(AntennaFullwaveSolver* solver, double frequency);
int antenna_fullwave_compute_near_field(AntennaFullwaveSolver* solver, double frequency, double x_min, double x_max, double y_min, double y_max, double z_min, double z_max);

int antenna_fullwave_compute_radiation_pattern(AntennaFullwaveSolver* solver);
int antenna_fullwave_compute_directivity(AntennaFullwaveSolver* solver);
int antenna_fullwave_compute_gain(AntennaFullwaveSolver* solver);
int antenna_fullwave_compute_efficiency(AntennaFullwaveSolver* solver);

int antenna_fullwave_compute_input_impedance(AntennaFullwaveSolver* solver, double frequency);
int antenna_fullwave_compute_vswr(AntennaFullwaveSolver* solver, double frequency);
int antenna_fullwave_compute_return_loss(AntennaFullwaveSolver* solver, double frequency);

int antenna_fullwave_compute_s_parameters(AntennaFullwaveSolver* solver, int num_ports);
int antenna_fullwave_mutual_coupling(AntennaFullwaveSolver* solver, int segment1, int segment2);

WaveParameters antenna_fullwave_compute_wave_parameters(double frequency, double relative_permittivity, double conductivity);
Complex antenna_fullwave_compute_segment_impedance(AntennaSegment* segment, double frequency, WaveParameters* wave_params);
Complex antenna_fullwave_compute_mutual_impedance(AntennaSegment* seg1, AntennaSegment* seg2, double frequency, WaveParameters* wave_params);

int antenna_fullwave_adaptive_mesh_refinement(AntennaFullwaveSolver* solver, double error_threshold);
int antenna_fullwave_convergence_check(AntennaFullwaveSolver* solver, double tolerance);
void antenna_fullwave_optimize_computation(AntennaFullwaveSolver* solver);

void antenna_fullwave_get_current_distribution(AntennaFullwaveSolver* solver, Complex* current_magnitude, Complex* current_phase);
void antenna_fullwave_get_radiation_pattern(AntennaFullwaveSolver* solver, double* theta_angles, double* phi_angles, Complex*** e_field);
void antenna_fullwave_get_input_impedance(AntennaFullwaveSolver* solver, double* frequency, Complex* impedance);

int antenna_fullwave_export_results(AntennaFullwaveSolver* solver, const char* filename, const char* format);
int antenna_fullwave_import_antenna(AntennaFullwaveSolver* solver, const char* filename);

void antenna_fullwave_print_summary(AntennaFullwaveSolver* solver);
void antenna_fullwave_benchmark(AntennaFullwaveSolver* solver);

#endif