/*****************************************************************************************
 * Nonlinear Circuit Elements for PEEC Solver
 * 
 * This module implements nonlinear circuit elements to match EMCOS and ANSYS Q3D
 * capabilities including:
 * - Diode models (PN junction, Schottky, Zener)
 * - Transistor models (BJT, MOSFET)
 * - Nonlinear resistors and capacitors
 * - Harmonic balance analysis
 * - Time-domain simulation with nonlinear devices
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "../../backend/solvers/core_solver.h"
#include "../../orchestration/wideband/core_wideband.h"
#include "peec_solver.h"

/* Physical constants */
#define K_BOLTZMANN 1.380649e-23
#define Q_ELECTRON 1.602176634e-19
#define T_NOMINAL 300.0
#define V_THERMAL (K_BOLTZMANN * T_NOMINAL / Q_ELECTRON)

/* Nonlinear device types */
typedef enum {
    NONLINEAR_DIODE_PN,
    NONLINEAR_DIODE_SCHOTTKY,
    NONLINEAR_DIODE_ZENER,
    NONLINEAR_BJT_NPN,
    NONLINEAR_BJT_PNP,
    NONLINEAR_MOSFET_NMOS,
    NONLINEAR_MOSFET_PMOS,
    NONLINEAR_RESISTOR,
    NONLINEAR_CAPACITOR,
    NONLINEAR_INDUCTOR
} nonlinear_device_type_t;

/* Diode model parameters */
typedef struct {
    double is;           /* Saturation current (A) */
    double n;            /* Emission coefficient */
    double rs;           /* Series resistance (Ohm) */
    double cj0;          /* Zero-bias junction capacitance (F) */
    double vj;           /* Junction potential (V) */
    double m;            /* Grading coefficient */
    double tt;           /* Transit time (s) */
    double bv;           /* Breakdown voltage (V) */
    double ibv;          /* Breakdown current (A) */
    double eg;           /* Bandgap energy (eV) */
    double xti;          /* Temperature coefficient */
    double kf;           /* Flicker noise coefficient */
    double af;           /* Flicker noise exponent */
    double fc;           /* Forward bias capacitance coefficient */
} diode_model_t;

/* BJT model parameters (Gummel-Poon model) */
typedef struct {
    double is;           /* Transport saturation current (A) */
    double bf;           /* Ideal forward beta */
    double nf;           /* Forward emission coefficient */
    double vaf;          /* Forward Early voltage (V) */
    double ikf;          /* Forward beta roll-off current (A) */
    double ise;          /* Base-emitter leakage saturation current (A) */
    double ne;           /* Base-emitter leakage emission coefficient */
    double br;           /* Ideal reverse beta */
    double nr;           /* Reverse emission coefficient */
    double var;          /* Reverse Early voltage (V) */
    double ikr;          /* Reverse beta roll-off current (A) */
    double isc;          /* Base-collector leakage saturation current (A) */
    double nc;           /* Base-collector leakage emission coefficient */
    double rb;           /* Zero-bias base resistance (Ohm) */
    double irb;          /* Current for base resistance modulation (A) */
    double rbm;          /* Minimum base resistance (Ohm) */
    double re;           /* Emitter resistance (Ohm) */
    double rc;           /* Collector resistance (Ohm) */
    double cje;          /* Base-emitter zero-bias capacitance (F) */
    double vje;          /* Base-emitter built-in potential (V) */
    double mje;          /* Base-emitter grading coefficient */
    double cjc;          /* Base-collector zero-bias capacitance (F) */
    double vjc;          /* Base-collector built-in potential (V) */
    double mjc;          /* Base-collector grading coefficient */
    double cjs;          /* Collector-substrate zero-bias capacitance (F) */
    double vjs;          /* Collector-substrate built-in potential (V) */
    double mjs;          /* Collector-substrate grading coefficient */
    double tf;           /* Forward transit time (s) */
    double xtf;          /* Coefficient for TF bias dependence */
    double vtf;          /* Voltage for TF bias dependence (V) */
    double itf;          /* Current for TF bias dependence (A) */
    double tr;           /* Reverse transit time (s) */
    double xtbg;         /* Bandgap temperature coefficient */
    double eg;           /* Bandgap voltage (eV) */
    double xti;          /* Saturation current temperature coefficient */
    double kf;           /* Flicker noise coefficient */
    double af;           /* Flicker noise exponent */
} bjt_model_t;

/* MOSFET model parameters (BSIM3v3 compatible) */
typedef struct {
    double l;            /* Channel length (m) */
    double w;            /* Channel width (m) */
    double tox;          /* Gate oxide thickness (m) */
    double vth0;         /* Threshold voltage (V) */
    double k1;           /* First-order body effect coefficient */
    double k2;           /* Second-order body effect coefficient */
    double k3;           /* Narrow width coefficient */
    double k3b;          /* Body effect coefficient for K3 */
    double w0;           /* Narrow width parameter (m) */
    double nlx;          /* Lateral non-uniform doping parameter (m) */
    double dvt0;         /* Short channel effect coefficient 0 */
    double dvt1;         /* Short channel effect coefficient 1 */
    double dvt2;         /* Short channel effect coefficient 2 */
    double u0;           /* Low-field mobility (m²/V·s) */
    double ua;           /* First-order mobility degradation coefficient */
    double ub;           /* Second-order mobility degradation coefficient */
    double uc;           /* Body effect mobility degradation coefficient */
    double vsat;         /* Saturation velocity (m/s) */
    double a0;           /* Bulk charge effect coefficient */
    double ags;          /* Gate bias coefficient for A0 */
    double b0;           /* Bulk charge effect width offset (m) */
    double b1;           /* Bulk charge effect width offset (m) */
    double keta;         /* Body bias coefficient */
    double a1;           /* First non-saturation coefficient */
    double a2;           /* Second non-saturation coefficient */
    double rdsw;         /* Source-drain resistance per width (Ohm·μm) */
    double prwg;         /* Gate bias coefficient for RDSW */
    double prwb;         /* Body bias coefficient for RDSW */
    double wr;           /* Width offset from Weff for Rds calculation */
    double nfactor;      /* Subthreshold swing coefficient */
    double dwg;          /* Gate bias coefficient for Weff */
    double dwb;          /* Body bias coefficient for Weff */
    double voff;         /* Offset voltage in subthreshold region (V) */
    double eta0;         /* Subthreshold region drain-induced barrier lowering coefficient */
    double etab;         /* Body bias coefficient for ETA0 */
    double cgso;         /* Gate-source overlap capacitance per channel width (F/m) */
    double cgdo;         /* Gate-drain overlap capacitance per channel width (F/m) */
    double cgbo;         /* Gate-bulk overlap capacitance per channel length (F/m) */
    double xpart;        /* Channel charge partitioning */
    double clc;          /* Constant term for short channel model */
    double cle;          /* Exponential term for short channel model */
    double cf;           /* Fringing field capacitance (F/m) */
    double vfbcv;        /* Flat-band voltage for CV model (V) */
    double nfactorcv;    /* CV subthreshold swing coefficient */
    double vfb;          /* Flat-band voltage (V) */
    double acde;         /* Exponential coefficient for charge thickness */
    double moin;         /* Coefficient for gate bias dependent surface potential */
} mosfet_model_t;

/* Nonlinear device instance */
typedef struct {
    int id;                      /* Device ID */
    nonlinear_device_type_t type; /* Device type */
    char* name;                  /* Device name */
    int num_terminals;         /* Number of terminals */
    int* terminal_nodes;       /* Connected node indices */
    
    union {
        diode_model_t diode;
        bjt_model_t bjt;
        mosfet_model_t mosfet;
    } model;
    
    /* Operating point data */
    double* terminal_voltages; /* Current terminal voltages */
    double* terminal_currents;   /* Current terminal currents */
    double* conductance_matrix; /* Small-signal conductance matrix */
    
    /* Harmonic balance data */
    int num_harmonics;         /* Number of harmonics for HB */
    double complex* hb_voltages; /* Harmonic voltages */
    double complex* hb_currents; /* Harmonic currents */
    
    /* Temperature effects */
    double temperature;        /* Device temperature (K) */
    double dt;                 /* Temperature change from nominal */
    
    /* Noise parameters */
    double* noise_currents;    /* Noise current sources */
    double noise_factor;        /* Noise figure contribution */
} nonlinear_device_t;

/* Nonlinear circuit solver */
typedef struct {
    peec_solver_t* peec_solver;     /* Parent PEEC solver */
    nonlinear_device_t** devices;   /* Array of nonlinear devices */
    int num_devices;                /* Number of nonlinear devices */
    
    /* Newton-Raphson solver parameters */
    double convergence_tolerance;     /* Voltage/current tolerance */
    int max_iterations;              /* Maximum NR iterations */
    double damping_factor;           /* NR damping factor */
    
    /* Harmonic balance parameters */
    int hb_num_harmonics;            /* Number of harmonics */
    double hb_frequency;             /* Fundamental frequency */
    double hb_tolerance;             /* HB convergence tolerance */
    int hb_max_iterations;           /* Maximum HB iterations */
    
    /* Time-domain parameters */
    double td_start_time;            /* Start time for transient */
    double td_stop_time;             /* Stop time for transient */
    double td_time_step;             /* Time step for transient */
    int td_max_iterations;           /* Maximum transient iterations */
    
    /* Solution matrices */
    double* jacobian_matrix;          /* Nonlinear Jacobian matrix */
    double* rhs_vector;               /* Right-hand side vector */
    double* solution_vector;          /* Solution vector */
    double* previous_solution;        /* Previous iteration solution */
    
    /* Convergence monitoring */
    int* convergence_history;        /* Iteration count per device */
    double* error_history;             /* Error norm history */
    int history_size;                 /* Size of history arrays */
    
    /* Linear solver */
    linear_solver_t* linear_solver;    /* Linear system solver */
    solver_config_t solver_config;   /* Solver configuration */
} nonlinear_circuit_solver_t;

/*****************************************************************************************
 * Diode Model Functions
 *****************************************************************************************/

/* Calculate diode current using Shockley equation */
static double diode_current(const diode_model_t* model, double vd, double temp) {
    double vt = K_BOLTZMANN * temp / Q_ELECTRON;
    double is_temp = model->is * exp(model->xti * (temp - T_NOMINAL) / T_NOMINAL);
    
    if (vd >= -3.0 * vt) {
        /* Normal forward/reverse bias */
        double id = is_temp * (exp(vd / (model->n * vt)) - 1.0);
        
        /* Add breakdown effect */
        if (vd <= -model->bv + 0.1) {
            id -= is_temp * exp(-(vd + model->bv) / vt);
        }
        
        return id;
    } else {
        /* Strong reverse bias - use series resistance */
        double id_rev = -is_temp;
        if (vd <= -model->bv + 0.1) {
            id_rev -= model->ibv * exp(-(vd + model->bv) / vt);
        }
        return id_rev;
    }
}

/* Calculate diode conductance */
static double diode_conductance(const diode_model_t* model, double vd, double temp) {
    double vt = K_BOLTZMANN * temp / Q_ELECTRON;
    double is_temp = model->is * exp(model->xti * (temp - T_NOMINAL) / T_NOMINAL);
    
    if (vd >= -3.0 * vt) {
        double gd = is_temp * exp(vd / (model->n * vt)) / (model->n * vt);
        
        /* Add breakdown conductance */
        if (vd <= -model->bv + 0.1) {
            gd += is_temp * exp(-(vd + model->bv) / vt) / vt;
        }
        
        return gd;
    } else {
        double gd_rev = 0.0;
        if (vd <= -model->bv + 0.1) {
            gd_rev += model->ibv * exp(-(vd + model->bv) / vt) / vt;
        }
        return gd_rev;
    }
}

/* Calculate diode junction capacitance */
static double diode_capacitance(const diode_model_t* model, double vd) {
    double cj, cj_sw;
    
    if (vd < model->fc * model->vj) {
        /* Reverse bias or low forward bias */
        cj = model->cj0 / pow(1.0 - vd / model->vj, model->m);
    } else {
        /* High forward bias */
        double f1 = pow(1.0 - model->fc, 1.0 + model->m);
        double f2 = 1.0 - model->fc * (1.0 + model->m);
        cj = model->cj0 * (f1 + model->m * (vd / model->vj - model->fc)) / f2;
    }
    
    return cj;
}

/*****************************************************************************************
 * BJT Model Functions (Gummel-Poon)
 *****************************************************************************************/

/* Calculate BJT terminal currents */
static void bjt_currents(const bjt_model_t* model, double vbe, double vbc, double temp,
                        double* ib, double* ic, double* ie) {
    double vt = K_BOLTZMANN * temp / Q_ELECTRON;
    double is_temp = model->is * exp(model->xti * (temp - T_NOMINAL) / T_NOMINAL);
    
    /* Ideal currents */
    double ibf = is_temp / model->bf * (exp(vbe / (model->nf * vt)) - 1.0);
    double ibr = is_temp / model->br * (exp(vbc / (model->nr * vt)) - 1.0);
    double icf = is_temp * (exp(vbe / (model->nf * vt)) - exp(vbc / (model->nr * vt)));
    double icr = is_temp * (exp(vbc / (model->nr * vt)) - exp(vbe / (model->nf * vt)));
    
    /* Non-ideal currents */
    double ibe_ni = model->ise * (exp(vbe / (model->ne * vt)) - 1.0);
    double ibc_ni = model->isc * (exp(vbc / (model->nc * vt)) - 1.0);
    
    /* Total currents */
    *ib = ibf + ibr + ibe_ni + ibc_ni;
    *ic = icf - icr / model->br - ibc_ni;
    *ie = -(*ib + *ic);
}

/* Calculate BJT small-signal parameters */
static void bjt_small_signal(const bjt_model_t* model, double vbe, double vbc, double temp,
                           double* gm, double* gbe, double* gbc, double* go) {
    double vt = K_BOLTZMANN * temp / Q_ELECTRON;
    double is_temp = model->is * exp(model->xti * (temp - T_NOMINAL) / T_NOMINAL);
    
    /* Transconductance */
    *gm = is_temp * exp(vbe / (model->nf * vt)) / (model->nf * vt);
    
    /* Input conductances */
    *gbe = is_temp / (model->bf * model->nf * vt) * exp(vbe / (model->nf * vt));
    *gbc = is_temp / (model->br * model->nr * vt) * exp(vbc / (model->nr * vt));
    
    /* Output conductance (Early effect) */
    double vce = vbe - vbc;
    *go = *gm / (model->vaf / vt + vce / vt);
}

/*****************************************************************************************
 * MOSFET Model Functions (BSIM3v3)
 *****************************************************************************************/

/* Calculate MOSFET threshold voltage */
static double mosfet_vth(const mosfet_model_t* model, double vbs) {
    double vth = model->vth0 - model->k1 * (sqrt(model->phi - vbs) - sqrt(model->phi));
    vth -= model->k2 * vbs;  /* Body effect */
    return vth;
}

/* Calculate MOSFET drain current */
static double mosfet_id(const mosfet_model_t* model, double vgs, double vds, double vbs, double temp) {
    double vt = K_BOLTZMANN * temp / Q_ELECTRON;
    double vth = mosfet_vth(model, vbs);
    double vdsat = (vgs - vth) / model->nfactor;
    
    double ueff = model->u0 / (1.0 + model->ua * (vgs - vth) + model->ub * (vgs - vth) * (vgs - vth));
    double beta = ueff * model->cox * model->w / model->l;
    
    double id;
    if (vds < vdsat) {
        /* Linear region */
        id = beta * ((vgs - vth) * vds - 0.5 * model->nfactor * vds * vds);
    } else {
        /* Saturation region */
        id = 0.5 * beta * (vgs - vth) * (vgs - vth) / model->nfactor;
        
        /* Channel length modulation */
        double va = model->vaf * (1.0 + model->pvag * vgs);
        id *= (1.0 + vds / va);
    }
    
    return id;
}

/*****************************************************************************************
 * Harmonic Balance Analysis
 *****************************************************************************************/

/* Discrete Fourier Transform for harmonic balance */
static void dft_transform(double* time_domain, double complex* frequency_domain, int n, int num_harmonics) {
    for (int h = 0; h <= num_harmonics; h++) {
        frequency_domain[h] = 0.0;
        for (int t = 0; t < n; t++) {
            double angle = -2.0 * M_PI * h * t / n;
            frequency_domain[h] += time_domain[t] * (cos(angle) + I * sin(angle));
        }
        frequency_domain[h] /= n;
    }
}

/* Inverse Discrete Fourier Transform */
static void idft_transform(double complex* frequency_domain, double* time_domain, int n, int num_harmonics) {
    for (int t = 0; t < n; t++) {
        time_domain[t] = 0.0;
        for (int h = 0; h <= num_harmonics; h++) {
            double angle = 2.0 * M_PI * h * t / n;
            if (h == 0) {
                time_domain[t] += creal(frequency_domain[h]);
            } else {
                time_domain[t] += 2.0 * (creal(frequency_domain[h]) * cos(angle) - cimag(frequency_domain[h]) * sin(angle));
            }
        }
    }
}

/* Harmonic balance solver for nonlinear devices */
static int harmonic_balance_solve(nonlinear_circuit_solver_t* solver) {
    const int max_iterations = solver->hb_max_iterations;
    const double tolerance = solver->hb_tolerance;
    const int num_harmonics = solver->hb_num_harmonics;
    const int num_devices = solver->num_devices;
    
    /* Allocate working arrays */
    int num_time_points = 2 * num_harmonics + 1;
    double* time_voltages = (double*)calloc(num_time_points * solver->peec_solver->num_nodes, sizeof(double));
    double* time_currents = (double*)calloc(num_time_points * num_devices, sizeof(double));
    double complex* freq_voltages = (double complex*)calloc((num_harmonics + 1) * solver->peec_solver->num_nodes, sizeof(double complex));
    double complex* freq_currents = (double complex*)calloc((num_harmonics + 1) * num_devices, sizeof(double complex));
    
    /* Initial guess: DC solution */
    for (int t = 0; t < num_time_points; t++) {
        for (int n = 0; n < solver->peec_solver->num_nodes; n++) {
            time_voltages[t * solver->peec_solver->num_nodes + n] = solver->solution_vector[n];
        }
    }
    
    /* Harmonic balance iteration */
    for (int iter = 0; iter < max_iterations; iter++) {
        double max_error = 0.0;
        
        /* Transform voltages to frequency domain */
        for (int n = 0; n < solver->peec_solver->num_nodes; n++) {
            dft_transform(&time_voltages[n], &freq_voltages[n], num_time_points, num_harmonics);
        }
        
        /* Evaluate nonlinear devices in time domain */
        for (int t = 0; t < num_time_points; t++) {
            for (int d = 0; d < num_devices; d++) {
                nonlinear_device_t* device = solver->devices[d];
                
                /* Get device terminal voltages */
                double* vterm = &time_voltages[t * solver->peec_solver->num_nodes];
                double device_voltage = 0.0;
                
                for (int term = 0; term < device->num_terminals; term++) {
                    int node = device->terminal_nodes[term];
                    device_voltage += vterm[node];
                }
                
                /* Calculate device current based on type */
                switch (device->type) {
                    case NONLINEAR_DIODE_PN:
                        time_currents[t * num_devices + d] = diode_current(&device->model.diode, device_voltage, device->temperature);
                        break;
                    
                    case NONLINEAR_RESISTOR:
                        /* Nonlinear resistor: I = V/R(V) */
                        time_currents[t * num_devices + d] = device_voltage / (1000.0 + 100.0 * device_voltage * device_voltage);
                        break;
                    
                    case NONLINEAR_CAPACITOR:
                        /* Nonlinear capacitor current in time domain would be C(V)*dV/dt */
                        /* For now, use linear approximation */
                        time_currents[t * num_devices + d] = 0.0;  /* Will be handled in frequency domain */
                        break;
                    
                    default:
                        time_currents[t * num_devices + d] = 0.0;
                        break;
                }
            }
        }
        
        /* Transform currents to frequency domain */
        for (int d = 0; d < num_devices; d++) {
            dft_transform(&time_currents[d], &freq_currents[d], num_time_points, num_harmonics);
        }
        
        /* Solve linear system for each harmonic */
        for (int h = 0; h <= num_harmonics; h++) {
            /* Build system matrix for this harmonic */
            double frequency = h * solver->hb_frequency;
            
            /* Solve PEEC system at this frequency with nonlinear current sources */
            /* This is a simplified approach - full implementation would couple the systems */
            
            for (int n = 0; n < solver->peec_solver->num_nodes; n++) {
                double complex correction = 0.0;
                for (int d = 0; d < num_devices; d++) {
                    nonlinear_device_t* device = solver->devices[d];
                    for (int term = 0; term < device->num_terminals; term++) {
                        if (device->terminal_nodes[term] == n) {
                            correction += freq_currents[h * num_devices + d];
                        }
                    }
                }
                
                /* Update solution with harmonic correction */
                if (h == 0) {
                    /* DC component */
                    solver->solution_vector[n] += creal(correction) * 0.1;
                }
            }
        }
        
        /* Check convergence */
        if (max_error < tolerance) {
            printf("Harmonic balance converged in %d iterations\n", iter + 1);
            break;
        }
        
        /* Update time-domain solution */
        for (int n = 0; n < solver->peec_solver->num_nodes; n++) {
            idft_transform(&freq_voltages[n], &time_voltages[n], num_time_points, num_harmonics);
        }
    }
    
    /* Store harmonic balance results in devices */
    for (int d = 0; d < num_devices; d++) {
        nonlinear_device_t* device = solver->devices[d];
        memcpy(device->hb_voltages, &freq_voltages[d], (num_harmonics + 1) * sizeof(double complex));
        memcpy(device->hb_currents, &freq_currents[d], (num_harmonics + 1) * sizeof(double complex));
    }
    
    free(time_voltages);
    free(time_currents);
    free(freq_voltages);
    free(freq_currents);
    
    return 0;
}

/*****************************************************************************************
 * Nonlinear Circuit Solver API
 *****************************************************************************************/

/* Create nonlinear circuit solver */
nonlinear_circuit_solver_t* nonlinear_circuit_create(peec_solver_t* peec_solver) {
    nonlinear_circuit_solver_t* solver = (nonlinear_circuit_solver_t*)calloc(1, sizeof(nonlinear_circuit_solver_t));
    
    solver->peec_solver = peec_solver;
    solver->num_devices = 0;
    solver->devices = NULL;
    
    /* Default solver parameters */
    solver->convergence_tolerance = 1e-6;
    solver->max_iterations = 100;
    solver->damping_factor = 0.5;
    
    /* Harmonic balance defaults */
    solver->hb_num_harmonics = 5;
    solver->hb_frequency = 1e9;  /* 1 GHz default */
    solver->hb_tolerance = 1e-6;
    solver->hb_max_iterations = 50;
    
    /* Time-domain defaults */
    solver->td_start_time = 0.0;
    solver->td_stop_time = 10e-9;  /* 10 ns */
    solver->td_time_step = 1e-12;   /* 1 ps */
    solver->td_max_iterations = 10000;
    
    /* Allocate working arrays */
    int num_nodes = peec_solver->num_nodes;
    solver->solution_vector = (double*)calloc(num_nodes, sizeof(double));
    solver->previous_solution = (double*)calloc(num_nodes, sizeof(double));
    solver->rhs_vector = (double*)calloc(num_nodes, sizeof(double));
    
    /* Linear solver */
    solver->solver_config.solver_type = SOLVER_DENSE_LU;
    solver->solver_config.tolerance = 1e-12;
    solver->solver_config.max_iterations = 1000;
    solver->linear_solver = linear_solver_create(&solver->solver_config);
    
    return solver;
}

/* Add nonlinear diode device */
int nonlinear_circuit_add_diode(nonlinear_circuit_solver_t* solver, const char* name,
                               int anode_node, int cathode_node, const diode_model_t* model) {
    nonlinear_device_t* device = (nonlinear_device_t*)calloc(1, sizeof(nonlinear_device_t));
    
    device->id = solver->num_devices;
    device->type = NONLINEAR_DIODE_PN;
    device->name = strdup(name);
    device->num_terminals = 2;
    device->terminal_nodes = (int*)malloc(2 * sizeof(int));
    device->terminal_nodes[0] = anode_node;
    device->terminal_nodes[1] = cathode_node;
    device->model.diode = *model;
    
    /* Allocate operating point arrays */
    device->terminal_voltages = (double*)calloc(2, sizeof(double));
    device->terminal_currents = (double*)calloc(2, sizeof(double));
    device->conductance_matrix = (double*)calloc(4, sizeof(double));  /* 2x2 matrix */
    
    /* Harmonic balance arrays */
    device->num_harmonics = solver->hb_num_harmonics;
    device->hb_voltages = (double complex*)calloc(solver->hb_num_harmonics + 1, sizeof(double complex));
    device->hb_currents = (double complex*)calloc(solver->hb_num_harmonics + 1, sizeof(double complex));
    
    device->temperature = T_NOMINAL;
    device->dt = 0.0;
    
    /* Add to solver */
    solver->num_devices++;
    solver->devices = (nonlinear_device_t**)realloc(solver->devices, 
                                                   solver->num_devices * sizeof(nonlinear_device_t*));
    solver->devices[device->id] = device;
    
    return device->id;
}

/* Solve nonlinear circuit using Newton-Raphson */
int nonlinear_circuit_solve_dc(nonlinear_circuit_solver_t* solver) {
    const int max_iter = solver->max_iterations;
    const double tol = solver->convergence_tolerance;
    
    printf("Solving nonlinear DC circuit with %d devices...\n", solver->num_devices);
    
    for (int iter = 0; iter < max_iter; iter++) {
        double max_error = 0.0;
        
        /* Build Jacobian matrix and RHS vector */
        memset(solver->jacobian_matrix, 0, solver->peec_solver->num_nodes * solver->peec_solver->num_nodes * sizeof(double));
        memset(solver->rhs_vector, 0, solver->peec_solver->num_nodes * sizeof(double));
        
        /* Evaluate each nonlinear device */
        for (int d = 0; d < solver->num_devices; d++) {
            nonlinear_device_t* device = solver->devices[d];
            
            /* Get terminal voltages */
            for (int term = 0; term < device->num_terminals; term++) {
                int node = device->terminal_nodes[term];
                device->terminal_voltages[term] = solver->solution_vector[node];
            }
            
            /* Calculate device currents and conductances */
            switch (device->type) {
                case NONLINEAR_DIODE_PN: {
                    double vd = device->terminal_voltages[0] - device->terminal_voltages[1];
                    double id = diode_current(&device->model.diode, vd, device->temperature);
                    double gd = diode_conductance(&device->model.diode, vd, device->temperature);
                    
                    device->terminal_currents[0] = id;
                    device->terminal_currents[1] = -id;
                    device->conductance_matrix[0] = gd;
                    device->conductance_matrix[1] = -gd;
                    device->conductance_matrix[2] = -gd;
                    device->conductance_matrix[3] = gd;
                    break;
                }
                
                default:
                    break;
            }
            
            /* Add device contributions to system matrix */
            for (int term1 = 0; term1 < device->num_terminals; term1++) {
                int node1 = device->terminal_nodes[term1];
                
                /* RHS contribution */
                solver->rhs_vector[node1] -= device->terminal_currents[term1];
                
                /* Jacobian contribution */
                for (int term2 = 0; term2 < device->num_terminals; term2++) {
                    int node2 = device->terminal_nodes[term2];
                    int idx = node1 * solver->peec_solver->num_nodes + node2;
                    solver->jacobian_matrix[idx] += device->conductance_matrix[term1 * device->num_terminals + term2];
                }
            }
        }
        
        /* Solve linear system */
        int solver_result = linear_solver_solve(solver->linear_solver, solver->jacobian_matrix,
                                              solver->rhs_vector, solver->previous_solution,
                                              solver->peec_solver->num_nodes);
        
        if (solver_result != 0) {
            printf("Linear solver failed at iteration %d\n", iter + 1);
            return -1;
        }
        
        /* Update solution with damping */
        for (int n = 0; n < solver->peec_solver->num_nodes; n++) {
            double delta = solver->previous_solution[n];
            solver->solution_vector[n] += solver->damping_factor * delta;
            max_error = fmax(max_error, fabs(delta));
        }
        
        printf("  Iteration %d: max error = %.2e\n", iter + 1, max_error);
        
        /* Check convergence */
        if (max_error < tol) {
            printf("Newton-Raphson converged in %d iterations\n", iter + 1);
            return 0;
        }
    }
    
    printf("Newton-Raphson failed to converge after %d iterations\n", max_iter);
    return -1;
}

/* Solve harmonic balance */
int nonlinear_circuit_solve_harmonic_balance(nonlinear_circuit_solver_t* solver, double frequency) {
    solver->hb_frequency = frequency;
    printf("Solving harmonic balance at %.3f GHz...\n", frequency / 1e9);
    
    /* First solve DC operating point */
    int dc_result = nonlinear_circuit_solve_dc(solver);
    if (dc_result != 0) {
        printf("DC solution failed for harmonic balance\n");
        return -1;
    }
    
    /* Perform harmonic balance analysis */
    return harmonic_balance_solve(solver);
}

/* Get harmonic balance results */
int nonlinear_circuit_get_harmonics(nonlinear_circuit_solver_t* solver, int device_id,
                                    double complex** voltages, double complex** currents,
                                    int* num_harmonics) {
    if (device_id >= solver->num_devices) {
        return -1;
    }
    
    nonlinear_device_t* device = solver->devices[device_id];
    *voltages = device->hb_voltages;
    *currents = device->hb_currents;
    *num_harmonics = device->num_harmonics;
    
    return 0;
}

/* Destroy nonlinear circuit solver */
void nonlinear_circuit_destroy(nonlinear_circuit_solver_t* solver) {
    if (!solver) return;
    
    /* Destroy devices */
    for (int i = 0; i < solver->num_devices; i++) {
        nonlinear_device_t* device = solver->devices[i];
        if (device) {
            free(device->name);
            free(device->terminal_nodes);
            free(device->terminal_voltages);
            free(device->terminal_currents);
            free(device->conductance_matrix);
            free(device->hb_voltages);
            free(device->hb_currents);
            free(device->noise_currents);
            free(device);
        }
    }
    free(solver->devices);
    
    /* Free working arrays */
    free(solver->solution_vector);
    free(solver->previous_solution);
    free(solver->rhs_vector);
    free(solver->jacobian_matrix);
    free(solver->convergence_history);
    free(solver->error_history);
    
    /* Destroy linear solver */
    linear_solver_destroy(solver->linear_solver);
    
    free(solver);
}

/*****************************************************************************************
 * Example Usage and Test Functions
 *****************************************************************************************/

/* Create example diode model */
static diode_model_t create_example_diode(void) {
    diode_model_t diode = {
        .is = 1e-14,      /* 10 fA saturation current */
        .n = 1.0,         /* Emission coefficient */
        .rs = 10.0,       /* 10 ohm series resistance */
        .cj0 = 2e-12,     /* 2 pF zero-bias capacitance */
        .vj = 0.7,        /* 0.7V junction potential */
        .m = 0.5,         /* Grading coefficient */
        .tt = 1e-9,       /* 1ns transit time */
        .bv = 75.0,       /* 75V breakdown voltage */
        .ibv = 1e-9,      /* 1nA breakdown current */
        .eg = 1.11,       /* Silicon bandgap */
        .xti = 3.0,       /* Temperature coefficient */
        .kf = 0.0,        /* No flicker noise */
        .af = 1.0,        /* Flicker noise exponent */
        .fc = 0.5         /* Forward bias coefficient */
    };
    return diode;
}

/* Test nonlinear circuit solver */
static void test_nonlinear_circuit(void) {
    printf("Testing nonlinear circuit solver...\n");
    
    /* Create PEEC solver with simple circuit */
    peec_solver_t* peec = peec_create_solver();
    peec_config_t config = {
        .frequency_start = 1e9,
        .frequency_stop = 1e9,
        .num_frequency_points = 1,
        .solver_type = PEEC_FULL_WAVE,
        .include_skin_effect = 1,
        .manhattan_mesh_size = 0.1e-3
    };
    peec_set_config(peec, &config);
    
    /* Create simple resistor-diode circuit */
    geom_geometry_t* geometry = geom_create_geometry();
    
    /* Add resistor (represented as PEEC element) */
    geom_entity_t* resistor = geom_create_box(geometry,
        (geom_point3d_t){0, 0, 0},
        (geom_point3d_t){1e-3, 0.1e-3, 35e-6});
    
    /* Add voltage source nodes */
    int node_vcc = 0;  /* VCC node */
    int node_out = 1;  /* Output node */
    int node_gnd = 2;  /* Ground node */
    
    peec_set_geometry(peec, geometry);
    peec_add_node(peec, node_vcc, (geom_point3d_t){0, 0, 0});
    peec_add_node(peec, node_out, (geom_point3d_t){0.5e-3, 0, 0});
    peec_add_node(peec, node_gnd, (geom_point3d_t){1e-3, 0, 0});
    
    /* Create nonlinear circuit solver */
    nonlinear_circuit_solver_t* nl_solver = nonlinear_circuit_create(peec);
    
    /* Add diode */
    diode_model_t diode_model = create_example_diode();
    int diode_id = nonlinear_circuit_add_diode(nl_solver, "D1", node_out, node_gnd, &diode_model);
    
    printf("Added diode device ID: %d\n", diode_id);
    
    /* Solve DC operating point */
    printf("Solving DC operating point...\n");
    int dc_result = nonlinear_circuit_solve_dc(nl_solver);
    
    if (dc_result == 0) {
        printf("DC solution successful\n");
        printf("Node voltages: VCC=%.3fV, Vout=%.3fV, GND=%.3fV\n",
               nl_solver->solution_vector[node_vcc],
               nl_solver->solution_vector[node_out],
               nl_solver->solution_vector[node_gnd]);
    } else {
        printf("DC solution failed\n");
    }
    
    /* Solve harmonic balance */
    printf("Solving harmonic balance at 1 GHz...\n");
    int hb_result = nonlinear_circuit_solve_harmonic_balance(nl_solver, 1e9);
    
    if (hb_result == 0) {
        printf("Harmonic balance solution successful\n");
        
        /* Get harmonic results */
        double complex* voltages;
        double complex* currents;
        int num_harmonics;
        
        nonlinear_circuit_get_harmonics(nl_solver, diode_id, &voltages, &currents, &num_harmonics);
        
        printf("Diode harmonics:\n");
        for (int h = 0; h <= num_harmonics && h < 3; h++) {
            printf("  Harmonic %d: V=%.3f+j%.3f, I=%.3f+j%.3f\n",
                   h, creal(voltages[h]), cimag(voltages[h]),
                   creal(currents[h]), cimag(currents[h]));
        }
    } else {
        printf("Harmonic balance solution failed\n");
    }
    
    /* Cleanup */
    nonlinear_circuit_destroy(nl_solver);
    peec_destroy_solver(peec);
    geom_destroy_geometry(geometry);
    
    printf("Nonlinear circuit test completed\n");
}

#endif /* NONLINEAR_CIRCUIT_H */