/**
 * @file core_multiphysics.h
 * @brief Multi-physics coupling capabilities for electromagnetic-thermal-mechanical analysis
 * @details Commercial-grade multi-physics coupling similar to ANSYS Multiphysics and COMSOL
 */

#ifndef CORE_MULTIPHYSICS_H
#define CORE_MULTIPHYSICS_H

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PHYSICS_DOMAINS 10
#define MAX_COUPLING_TERMS 50
#define MAX_TEMPERATURE_POINTS 1000
#define MAX_THERMAL_ITERATIONS 100
#define THERMAL_CONVERGENCE_TOLERANCE 1e-6

typedef enum {
    PHYSICS_ELECTROMAGNETIC = 0,
    PHYSICS_THERMAL = 1,
    PHYSICS_MECHANICAL = 2,
    PHYSICS_FLUID = 3,
    PHYSICS_CIRCUIT = 4,
    PHYSICS_SEMICONDUCTOR = 5,
    PHYSICS_PIEZOELECTRIC = 6,
    PHYSICS_MAGNETOSTATIC = 7,
    PHYSICS_ELECTROSTATIC = 8,
    PHYSICS_CURRENT_FLOW = 9
} physics_domain_t;

typedef enum {
    COUPLING_WEAK = 0,           /* One-way coupling */
    COUPLING_STRONG = 1,         /* Two-way coupling */
    COUPLING_FULL = 2,           /* Fully coupled */
    COUPLING_ITERATIVE = 3,      /* Iterative coupling */
    COUPLING_STAGGERED = 4,      /* Staggered coupling */
    COUPLING_MONOLITHIC = 5      /* Monolithic coupling */
} coupling_strategy_t;

typedef enum {
    THERMAL_CONDUCTION = 0,
    THERMAL_CONVECTION = 1,
    THERMAL_RADIATION = 2,
    THERMAL_JOULE_HEATING = 3,
    THERMAL_MICROWAVE_HEATING = 4,
    THERMAL_EDDY_CURRENT_HEATING = 5
} thermal_mechanism_t;

typedef enum {
    MECHANICAL_LINEAR_ELASTIC = 0,
    MECHANICAL_NONLINEAR_ELASTIC = 1,
    MECHANICAL_PLASTIC = 2,
    MECHANICAL_CREEP = 3,
    MECHANICAL_THERMAL_STRESS = 4,
    MECHANICAL_ELECTROSTRICTIVE = 5,
    MECHANICAL_MAGNETOSTRICTIVE = 6
} mechanical_model_t;

typedef struct {
    double temperature;              /* Temperature in Kelvin */
    double pressure;                 /* Pressure in Pascals */
    double electric_field[3];        /* Electric field vector */
    double magnetic_field[3];        /* Magnetic field vector */
    double current_density[3];       /* Current density vector */
    double displacement[3];          /* Mechanical displacement */
    double stress[6];                /* Stress tensor (xx, yy, zz, xy, yz, zx) */
    double strain[6];                /* Strain tensor */
    double heat_flux[3];             /* Heat flux vector */
    double velocity[3];              /* Fluid velocity */
    double concentration;            /* Concentration for semiconductor */
    double* user_defined;            /* User-defined field variables */
    int num_user_fields;
} physics_field_t;

typedef struct {
    physics_domain_t domain;         /* Physics domain */
    int dimension;                   /* Spatial dimension (1D, 2D, 3D) */
    int num_nodes;                   /* Number of nodes */
    int num_elements;                /* Number of elements */
    double* coordinates;             /* Node coordinates */
    int* connectivity;               /* Element connectivity */
    physics_field_t* fields;         /* Field variables at nodes */
    double* material_properties;     /* Material properties */
    int* boundary_conditions;        /* Boundary condition flags */
    double frequency;                /* Operating frequency for EM */
    double time;                     /* Current time for transient */
    bool is_stationary;              /* Stationary or transient */
    char name[64];                   /* Domain name */
} physics_domain_data_t;

typedef struct {
    physics_domain_t source_domain;  /* Source physics domain */
    physics_domain_t target_domain;  /* Target physics domain */
    coupling_strategy_t strategy;    /* Coupling strategy */
    double coupling_coefficient;       /* Coupling coefficient */
    double* coupling_matrix;           /* Coupling matrix */
    int matrix_size;                   /* Matrix size */
    bool is_nonlinear;                 /* Nonlinear coupling */
    bool is_time_dependent;            /* Time-dependent coupling */
    double update_frequency;           /* Update frequency for iterative coupling */
    double convergence_tolerance;      /* Convergence tolerance */
    int max_iterations;                /* Maximum iterations */
    char source_field[32];             /* Source field name */
    char target_field[32];             /* Target field name */
} physics_coupling_t;

typedef struct {
    physics_domain_data_t* domains[MAX_PHYSICS_DOMAINS];
    physics_coupling_t* couplings[MAX_COUPLING_TERMS];
    int num_domains;
    int num_couplings;
    coupling_strategy_t global_strategy;
    double convergence_tolerance;
    int max_global_iterations;
    bool enable_adaptive_coupling;
    bool enable_parallel_coupling;
    int num_threads;
    double coupling_time_step;
    double current_time;
    double end_time;
    bool is_transient;
} multiphysics_system_t;

typedef struct {
    double thermal_conductivity;       /* W/(m·K) */
    double specific_heat;              /* J/(kg·K) */
    double density;                    /* kg/m³ */
    double thermal_expansion;          /* 1/K */
    double young_modulus;              /* Pa */
    double poisson_ratio;
    double electrical_conductivity;    /* S/m */
    double relative_permittivity;
    double relative_permeability;
    double loss_tangent;
    double temperature_coefficient;    /* 1/K */
    double reference_temperature;      /* K */
    double* temperature_dependence;    /* Temperature-dependent properties */
    int num_temp_points;
} multiphysics_material_t;

typedef struct {
    double* temperatures;              /* Temperature points */
    double* thermal_stress;          /* Thermal stress tensor */
    double* thermal_strain;            /* Thermal strain tensor */
    double* thermal_displacement;      /* Thermal displacement */
    double* heat_generation;           /* Heat generation rate */
    double* heat_flux;                 /* Heat flux */
    int num_points;
    double max_temperature;
    double min_temperature;
    double max_stress;
    double max_displacement;
    double total_heat;
} thermal_analysis_result_t;

typedef struct {
    double* frequencies;               /* Frequency points */
    double* em_loss_density;           /* EM loss density */
    double* temperature_rise;          /* Temperature rise */
    double* thermal_time_constant;     /* Thermal time constant */
    double* coupling_strength;         /* Coupling strength */
    int num_frequencies;
    double max_loss_density;
    double max_temperature_rise;
    double coupling_bandwidth;
} em_thermal_coupling_result_t;

typedef struct {
    double* coordinates;               /* Coordinates */
    double* mechanical_stress;         /* Mechanical stress tensor */
    double* mechanical_strain;           /* Mechanical strain tensor */
    double* mechanical_displacement;     /* Mechanical displacement */
    double* von_mises_stress;            /* Von Mises stress */
    double* safety_factor;               /* Safety factor */
    int num_points;
    double max_stress;
    double max_displacement;
    double max_von_mises;
    double min_safety_factor;
} mechanical_analysis_result_t;

typedef struct {
    double* time_points;               /* Time points */
    double* temperature_evolution;     /* Temperature evolution */
    double* stress_evolution;            /* Stress evolution */
    double* displacement_evolution;      /* Displacement evolution */
    double* coupling_residual;             /* Coupling residual */
    int num_time_points;
    int num_iterations;
    double final_temperature;
    double final_stress;
    double final_displacement;
    double max_coupling_residual;
    bool converged;
} transient_coupling_result_t;

typedef struct {
    physics_domain_t analysis_type;      /* Analysis type */
    double convergence_tolerance;        /* Convergence tolerance */
    int max_iterations;                  /* Maximum iterations */
    double time_step;                    /* Time step for transient */
    double start_time;                     /* Start time */
    double end_time;                       /* End time */
    bool enable_nonlinear;                 /* Enable nonlinear analysis */
    bool enable_adaptive_mesh;             /* Enable adaptive meshing */
    bool enable_error_estimation;          /* Enable error estimation */
    int output_frequency;                  /* Output frequency */
    double coupling_tolerance;             /* Coupling tolerance */
    int coupling_update_frequency;         /* Coupling update frequency */
} multiphysics_config_t;

/**
 * @brief Initialize multi-physics system
 * @return 0 on success, -1 on failure
 */
int multiphysics_init(void);

/**
 * @brief Create multi-physics system
 * @param config Multi-physics configuration
 * @return Pointer to multi-physics system, NULL on failure
 */
multiphysics_system_t* multiphysics_system_create(multiphysics_config_t* config);

/**
 * @brief Destroy multi-physics system
 * @param system Multi-physics system to destroy
 */
void multiphysics_system_destroy(multiphysics_system_t* system);

/**
 * @brief Add physics domain to system
 * @param system Multi-physics system
 * @param domain Domain data
 * @return 0 on success, -1 on failure
 */
int multiphysics_add_domain(multiphysics_system_t* system, physics_domain_data_t* domain);

/**
 * @brief Add physics coupling
 * @param system Multi-physics system
 * @param coupling Coupling definition
 * @return 0 on success, -1 on failure
 */
int multiphysics_add_coupling(multiphysics_system_t* system, physics_coupling_t* coupling);

/**
 * @brief Define multi-physics material
 * @param material Material properties
 * @param name Material name
 * @return Material ID on success, -1 on failure
 */
int multiphysics_define_material(multiphysics_material_t* material, const char* name);

/**
 * @brief Solve stationary multi-physics problem
 * @param system Multi-physics system
 * @param result Thermal analysis result
 * @return 0 on success, -1 on failure
 */
int multiphysics_solve_stationary(multiphysics_system_t* system, thermal_analysis_result_t* result);

/**
 * @brief Solve transient multi-physics problem
 * @param system Multi-physics system
 * @param result Transient coupling result
 * @return 0 on success, -1 on failure
 */
int multiphysics_solve_transient(multiphysics_system_t* system, transient_coupling_result_t* result);

/**
 * @brief Perform EM-thermal coupling analysis
 * @param system Multi-physics system
 * @param em_solver_data EM solver data (MOM or PEEC)
 * @param result EM-thermal coupling result
 * @return 0 on success, -1 on failure
 */
int multiphysics_em_thermal_coupling(multiphysics_system_t* system, void* em_solver_data,
                                    em_thermal_coupling_result_t* result);

/**
 * @brief Perform thermal-mechanical coupling analysis
 * @param system Multi-physics system
 * @param thermal_result Thermal analysis result
 * @param mechanical_result Mechanical analysis result
 * @return 0 on success, -1 on failure
 */
int multiphysics_thermal_mechanical_coupling(multiphysics_system_t* system,
                                           thermal_analysis_result_t* thermal_result,
                                           mechanical_analysis_result_t* mechanical_result);

/**
 * @brief Update EM losses in thermal domain
 * @param system Multi-physics system
 * @param em_solver_data EM solver data
 * @param loss_scaling Scaling factor for losses
 * @return 0 on success, -1 on failure
 */
int multiphysics_update_em_losses(multiphysics_system_t* system, void* em_solver_data, double loss_scaling);

/**
 * @brief Update temperature-dependent material properties
 * @param system Multi-physics system
 * @param temperature_field Temperature field data
 * @param num_points Number of temperature points
 * @return 0 on success, -1 on failure
 */
int multiphysics_update_temperature_dependent_properties(multiphysics_system_t* system,
                                                       double* temperature_field, int num_points);

/**
 * @brief Calculate Joule heating from electromagnetic solution
 * @param em_solver_data EM solver data (MOM or PEEC)
 * @param heating_density Output heating density array
 * @param num_points Number of points
 * @return 0 on success, -1 on failure
 */
int multiphysics_calculate_joule_heating(void* em_solver_data, double* heating_density, int num_points);

/**
 * @brief Calculate electromagnetic forces
 * @param em_solver_data EM solver data
 * @param force_density Output force density array
 * @param num_points Number of points
 * @return 0 on success, -1 on failure
 */
int multiphysics_calculate_em_forces(void* em_solver_data, double* force_density, int num_points);

/**
 * @brief Check multi-physics convergence
 * @param system Multi-physics system
 * @param tolerance Convergence tolerance
 * @return true if converged, false otherwise
 */
bool multiphysics_check_convergence(multiphysics_system_t* system, double tolerance);

/**
 * @brief Export multi-physics results
 * @param system Multi-physics system
 * @param filename Output filename
 * @param format Output format ("VTK", "CSV", "MAT")
 * @return 0 on success, -1 on failure
 */
int multiphysics_export_results(multiphysics_system_t* system, const char* filename, const char* format);

/**
 * @brief Print multi-physics statistics
 * @param system Multi-physics system
 */
void multiphysics_print_statistics(multiphysics_system_t* system);

/**
 * @brief Cleanup multi-physics library
 */
void multiphysics_cleanup(void);

/* Advanced coupling functions for specific physics combinations */

/**
 * @brief Perform electro-thermal coupling for high-frequency devices
 * @param em_solver_data EM solver data
 * @param thermal_domain Thermal domain data
 * @param coupling_config Coupling configuration
 * @param result Electro-thermal coupling result
 * @return 0 on success, -1 on failure
 */
int multiphysics_electrothermal_coupling(void* em_solver_data, physics_domain_data_t* thermal_domain,
                                        multiphysics_config_t* coupling_config,
                                        em_thermal_coupling_result_t* result);

/**
 * @brief Perform magneto-mechanical coupling for actuators
 * @param em_solver_data EM solver data
 * @param mechanical_domain Mechanical domain data
 * @param coupling_config Coupling configuration
 * @param result Mechanical analysis result
 * @return 0 on success, -1 on failure
 */
int multiphysics_magnetomechanical_coupling(void* em_solver_data, physics_domain_data_t* mechanical_domain,
                                          multiphysics_config_t* coupling_config,
                                          mechanical_analysis_result_t* result);

/**
 * @brief Perform piezoelectric coupling for sensors/actuators
 * @param em_solver_data EM solver data
 * @param mechanical_domain Mechanical domain data
 * @param coupling_config Coupling configuration
 * @param result Coupled analysis result
 * @return 0 on success, -1 on failure
 */
int multiphysics_piezoelectric_coupling(void* em_solver_data, physics_domain_data_t* mechanical_domain,
                                        multiphysics_config_t* coupling_config,
                                        transient_coupling_result_t* result);

/**
 * @brief Perform fluid-structure-thermal coupling
 * @param fluid_domain Fluid domain data
 * @param thermal_domain Thermal domain data
 * @param mechanical_domain Mechanical domain data
 * @param coupling_config Coupling configuration
 * @param result Transient coupling result
 * @return 0 on success, -1 on failure
 */
int multiphysics_fluid_thermal_mechanical_coupling(physics_domain_data_t* fluid_domain,
                                                  physics_domain_data_t* thermal_domain,
                                                  physics_domain_data_t* mechanical_domain,
                                                  multiphysics_config_t* coupling_config,
                                                  transient_coupling_result_t* result);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MULTIPHYSICS_H */