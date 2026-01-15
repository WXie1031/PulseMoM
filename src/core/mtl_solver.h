/********************************************************************************
 *  PulseEM - Unified Electromagnetic Simulation Platform
 *
 *  Copyright (C) 2024-2025 PulseEM Technologies
 *
 *  Commercial License - All Rights Reserved
 *  Unauthorized copying, modification, or distribution is strictly prohibited
 *  Proprietary and confidential - see LICENSE file for details
 *
 *  File: mtl_solver.h
 *  Description: Multi-conductor Transmission Line (MTL) solver for cable simulation
 ********************************************************************************/

#ifndef MTL_SOLVER_H
#define MTL_SOLVER_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#if defined(_MSC_VER)
// MSVC doesn't have complex.h, use custom complex type
typedef struct { double re; double im; } mtl_complex_t;
static inline mtl_complex_t mtl_complex_make(double re, double im) { mtl_complex_t z; z.re = re; z.im = im; return z; }
#define mtl_complex_real(z) ((z).re)
#define mtl_complex_imag(z) ((z).im)
static inline double mtl_complex_abs(mtl_complex_t z) { return sqrt(z.re*z.re + z.im*z.im); }
#else
#include <complex.h>
typedef double complex mtl_complex_t;
#define mtl_complex_make(re, im) ((re) + I*(im))
#define mtl_complex_real(z) (creal(z))
#define mtl_complex_imag(z) (cimag(z))
#define mtl_complex_abs(z) (cabs(z))
#endif

/* Forward declarations */
typedef struct mtl_cable mtl_cable_t;
typedef struct mtl_solver mtl_solver_t;
typedef struct mtl_results mtl_results_t;

/* Cable types */
typedef enum {
    MTL_CABLE_COAXIAL,
    MTL_CABLE_TWISTED_PAIR,
    MTL_CABLE_RIBBON,
    MTL_CABLE_HARNESS,
    MTL_CABLE_CUSTOM
} mtl_cable_type_t;

/* Conductor materials */
typedef enum {
    MTL_MATERIAL_COPPER,
    MTL_MATERIAL_ALUMINUM,
    MTL_MATERIAL_SILVER,
    MTL_MATERIAL_GOLD,
    MTL_MATERIAL_STEEL,
    MTL_MATERIAL_CUSTOM
} mtl_material_t;

/* Dielectric materials */
typedef enum {
    MTL_DIELECTRIC_PVC,
    MTL_DIELECTRIC_PE,
    MTL_DIELECTRIC_PTFE,
    MTL_DIELECTRIC_RUBBER,
    MTL_DIELECTRIC_CUSTOM
} mtl_dielectric_t;

/* Cable geometry definition */
typedef struct {
    double radius;              /* Conductor radius (m) */
    double thickness;           /* Insulation thickness (m) */
    double position[3];         /* 3D position (x,y,z) */
    double orientation[3];      /* Direction vector */
    mtl_material_t material;    /* Conductor material */
    mtl_dielectric_t dielectric; /* Insulation material */
} mtl_conductor_t;

/* Cable structure */
struct mtl_cable {
    char name[256];             /* Cable identifier */
    mtl_cable_type_t type;      /* Cable type */
    int num_conductors;         /* Number of conductors */
    mtl_conductor_t* conductors; /* Array of conductors */
    double length;              /* Cable length (m) */
    double frequency_start;     /* Start frequency (Hz) */
    double frequency_end;       /* End frequency (Hz) */
    int frequency_points;       /* Number of frequency points */
    
    /* KBL format support */
    char kbl_file[1024];        /* KBL file path */
    bool has_kbl_data;         /* KBL data availability */
    
    /* Stochastic placement */
    bool stochastic_placement;   /* Enable random placement */
    double placement_variance;   /* Position variance */
    int placement_seed;         /* Random seed */
};

/* MTL solver configuration */
typedef struct {
    /* Solver parameters */
    double tolerance;           /* Solution tolerance */
    int max_iterations;       /* Maximum iterations */
    bool use_skin_effect;      /* Include skin effect */
    bool use_proximity_effect;   /* Include proximity effect */
    bool use_common_mode;       /* Include common mode current */
    
    /* Frequency analysis */
    bool enable_afs;            /* Adaptive frequency sampling */
    double afs_threshold;       /* AFS convergence threshold */
    int afs_max_points;       /* Maximum AFS points */
    
    /* Hybrid MoM/MTL coupling */
    bool enable_hybrid;         /* Enable hybrid coupling */
    double coupling_threshold;   /* Coupling threshold */
    char mom_solver[256];       /* MoM solver configuration */
    
    /* Parallel processing */
    int num_threads;          /* Number of threads */
    bool enable_gpu;           /* Enable GPU acceleration */
    
    /* Output options */
    bool save_impedance_matrix;  /* Save Z matrix */
    bool save_admittance_matrix; /* Save Y matrix */
    bool save_scattering_matrix; /* Save S matrix */
    bool save_currents;        /* Save current distributions */
} mtl_config_t;

/* MTL results structure */
struct mtl_results {
    /* Frequency domain results */
    int num_frequencies;       /* Number of frequency points */
    double* frequencies;       /* Frequency vector (Hz) */
    
    /* Impedance matrix */
    mtl_complex_t** Z_matrix;  /* Impedance matrix vs frequency */
    mtl_complex_t** Y_matrix;  /* Admittance matrix vs frequency */
    mtl_complex_t** S_matrix;  /* Scattering matrix vs frequency */
    
    /* Current distributions */
    mtl_complex_t** currents;   /* Current on each conductor */
    mtl_complex_t** voltages; /* Voltage on each conductor */
    
    /* Cable parameters */
    double* R_per_unit;        /* Resistance per unit length */
    double* L_per_unit;        /* Inductance per unit length */
    double* C_per_unit;        /* Capacitance per unit length */
    double* G_per_unit;        /* Conductance per unit length */
    
    /* Special effects */
    double* skin_depth;        /* Skin depth vs frequency */
    double* proximity_factor;  /* Proximity effect factor */
    double* common_mode_current; /* Common mode current */
    double* transfer_impedance; /* Transfer impedance */
    
    /* Performance metrics */
    double solve_time;         /* Total solution time */
    int iterations;            /* Number of iterations */
    double memory_usage;       /* Peak memory usage (MB) */
};

/* Function prototypes */

/* Cable creation and management */
mtl_cable_t* mtl_cable_create(const char* name, mtl_cable_type_t type, int num_conductors);
void mtl_cable_destroy(mtl_cable_t* cable);
int mtl_cable_add_conductor(mtl_cable_t* cable, const mtl_conductor_t* conductor);
int mtl_cable_set_frequency_range(mtl_cable_t* cable, double start, double end, int points);
int mtl_cable_load_kbl(mtl_cable_t* cable, const char* kbl_file);
int mtl_cable_save_kbl(const mtl_cable_t* cable, const char* kbl_file);

/* Cable geometry and placement */
int mtl_cable_set_stochastic_placement(mtl_cable_t* cable, bool enable, 
                                     double variance, int seed);
int mtl_cable_generate_random_placement(mtl_cable_t* cable);

/* Material properties */
double mtl_material_conductivity(mtl_material_t material);
double mtl_material_permeability(mtl_material_t material);
double mtl_dielectric_permittivity(mtl_dielectric_t dielectric);
double mtl_dielectric_loss_tangent(mtl_dielectric_t dielectric);

/* MTL solver creation and configuration */
mtl_solver_t* mtl_solver_create(void);
void mtl_solver_destroy(mtl_solver_t* solver);
int mtl_solver_set_config(mtl_solver_t* solver, const mtl_config_t* config);
int mtl_solver_add_cable(mtl_solver_t* solver, mtl_cable_t* cable);
int mtl_solver_set_frequency_range(mtl_solver_t* solver, double start, double end, int points);

/* Core solver functions */
int mtl_solver_analyze(mtl_solver_t* solver);
int mtl_solver_solve_frequency_domain(mtl_solver_t* solver, double frequency);
mtl_results_t* mtl_solver_get_results(mtl_solver_t* solver);

/* Query functions */
int mtl_solver_get_num_conductors(const mtl_solver_t* solver);
int mtl_solver_get_num_cables(const mtl_solver_t* solver);

/* Advanced analysis functions */
int mtl_solver_calculate_skin_effect(mtl_solver_t* solver, mtl_cable_t* cable, 
                                   double frequency, double* skin_depth);
int mtl_solver_calculate_proximity_effect(mtl_solver_t* solver, mtl_cable_t* cable,
                                        double frequency, double* proximity_factor);
int mtl_solver_calculate_transfer_impedance(mtl_solver_t* solver, mtl_cable_t* cable,
                                          double frequency, double* Zt);
int mtl_solver_calculate_common_mode(mtl_solver_t* solver, mtl_cable_t* cable,
                                   double frequency, double* Icm);

/* Hybrid MoM/MTL coupling */
int mtl_solver_enable_hybrid_coupling(mtl_solver_t* solver, bool enable);
int mtl_solver_set_mom_coupling(mtl_solver_t* solver, void* mom_solver);
int mtl_solver_update_coupling_matrix(mtl_solver_t* solver);

/* Cable parameter extraction */
int mtl_extract_per_unit_parameters(mtl_solver_t* solver, mtl_cable_t* cable,
                                  double* R, double* L, double* C, double* G);
int mtl_extract_impedance_matrix(mtl_solver_t* solver, mtl_cable_t* cable,
                               double frequency, mtl_complex_t** Z);
int mtl_extract_admittance_matrix(mtl_solver_t* solver, mtl_cable_t* cable,
                                double frequency, mtl_complex_t** Y);
int mtl_extract_scattering_matrix(mtl_solver_t* solver, mtl_cable_t* cable,
                                double frequency, mtl_complex_t** S);

/* Results management */
mtl_results_t* mtl_results_create(int num_frequencies, int num_conductors);
void mtl_results_destroy(mtl_results_t* results);
int mtl_results_save_to_file(const mtl_results_t* results, const char* filename);
mtl_results_t* mtl_results_load_from_file(const char* filename);

/* Utility functions */
int mtl_validate_cable(const mtl_cable_t* cable);
void mtl_print_cable_info(const mtl_cable_t* cable);
void mtl_print_results_summary(const mtl_results_t* results);
double mtl_estimate_memory_usage(const mtl_solver_t* solver);

/* Error handling */
typedef enum {
    MTL_SUCCESS = 0,
    MTL_ERROR_INVALID_ARGUMENT = -1,
    MTL_ERROR_OUT_OF_MEMORY = -2,
    MTL_ERROR_CONVERGENCE = -3,
    MTL_ERROR_SINGULAR = -4,
    MTL_ERROR_FILE_IO = -5,
    MTL_ERROR_LICENSE = -6,
    MTL_ERROR_INTERNAL = -99
} mtl_error_t;

const char* mtl_error_string(mtl_error_t error);

#endif /* MTL_SOLVER_H */