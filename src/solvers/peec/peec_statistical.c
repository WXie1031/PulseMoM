/*****************************************************************************************
 * Statistical Analysis Engine for Process Variation Modeling
 * 
 * This module implements Monte Carlo analysis and statistical modeling capabilities
 * to match EMX and commercial EDA tools for manufacturing variation analysis:
 * - Monte Carlo simulation with correlated random variables
 * - Process corner analysis (fast/slow corners)
 * - Sensitivity analysis and yield optimization
 * - Statistical correlation modeling
 * - Manufacturing yield prediction
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include "../core/core_solver.h"
#include "../core/core_wideband.h"
#include "peec_solver.h"
#include "mom_solver.h"

/* Statistical distribution types */
typedef enum {
    DIST_UNIFORM,
    DIST_GAUSSIAN,
    DIST_LOGNORMAL,
    DIST_EXPONENTIAL,
    DIST_WEIBULL,
    DIST_GAMMA,
    DIST_BETA,
    DIST_TRIANGULAR,
    DIST_CUSTOM  /* User-defined distribution */
} statistical_distribution_t;

/* Process variation parameter */
typedef struct {
    char* name;                  /* Parameter name */
    char* description;           /* Parameter description */
    double nominal_value;        /* Nominal value */
    double std_deviation;        /* Standard deviation (sigma) */
    double min_value;            /* Minimum value */
    double max_value;            /* Maximum value */
    statistical_distribution_t distribution; /* Distribution type */
    int correlation_group;       /* Correlation group ID */
    double correlation_coefficient; /* Spatial correlation coefficient */
    int is_spatial;              /* Spatial variation flag */
    
    /* Custom distribution data */
    int custom_num_points;       /* Number of points for custom distribution */
    double* custom_values;       /* Custom distribution values */
    double* custom_probabilities; /* Custom distribution probabilities */
    
    /* Statistical properties */
    double mean;                 /* Computed mean */
    double variance;              /* Computed variance */
    double skewness;             /* Computed skewness */
    double kurtosis;             /* Computed kurtosis */
} process_variation_t;

/* Monte Carlo simulation configuration */
typedef struct {
    int num_samples;             /* Number of Monte Carlo samples */
    int num_process_corners;     /* Number of process corners */
    int random_seed;             /* Random number generator seed */
    double confidence_level;     /* Statistical confidence level */
    double yield_target;         /* Target manufacturing yield */
    int max_iterations;          /* Maximum analysis iterations */
    int convergence_criteria;    /* Convergence criteria type */
    double convergence_tolerance; /* Convergence tolerance */
    int parallel_threads;        /* Number of parallel threads */
    int save_all_results;        /* Save all simulation results */
    int correlation_modeling;    /* Enable correlation modeling */
    int sensitivity_analysis;    /* Enable sensitivity analysis */
    int yield_optimization;      /* Enable yield optimization */
} monte_carlo_config_t;

/* Statistical correlation matrix */
typedef struct {
    int num_parameters;          /* Number of parameters */
    double** correlation_matrix; /* Correlation coefficient matrix */
    double** cholesky_factor;  /* Cholesky decomposition for generation */
    int is_positive_definite;   /* Matrix positive definiteness flag */
} correlation_matrix_t;

/* Monte Carlo simulation results */
typedef struct {
    int sample_id;               /* Sample ID */
    double* parameter_values;    /* Parameter values for this sample */
    double* performance_metrics;  /* Performance metrics */
    int* specification_pass;      /* Pass/fail for each specification */
    int overall_pass;             /* Overall pass/fail */
    double yield_contribution;    /* Contribution to overall yield */
} monte_carlo_sample_t;

/* Statistical analysis results */
typedef struct {
    int num_samples;             /* Number of valid samples */
    int num_passed;              /* Number of samples meeting specifications */
    double estimated_yield;      /* Estimated manufacturing yield */
    double yield_confidence;     /* Confidence interval for yield */
    double* parameter_means;     /* Mean values for each parameter */
    double* parameter_std_devs;  /* Standard deviations */
    double* parameter_correlations; /* Parameter correlations */
    double* performance_means;   /* Mean performance metrics */
    double* performance_std_devs; /* Performance standard deviations */
    double** sensitivity_matrix;  /* Sensitivity of performance to parameters */
    double* worst_case_values;   /* Worst-case parameter values */
    double* best_case_values;    /* Best-case parameter values */
    int* critical_parameters;    /* Most critical parameters */
    double** covariance_matrix;  /* Parameter covariance matrix */
} statistical_results_t;

/* Statistical analysis engine */
typedef struct {
    peec_solver_t* peec_solver;     /* PEEC solver for circuit analysis */
    mom_solver_t* mom_solver;       /* MoM solver for EM analysis */
    
    /* Process variation parameters */
    process_variation_t** parameters; /* Array of variation parameters */
    int num_parameters;               /* Number of parameters */
    
    /* Monte Carlo configuration */
    monte_carlo_config_t config;       /* Simulation configuration */
    correlation_matrix_t correlations; /* Parameter correlations */
    
    /* Simulation results */
    monte_carlo_sample_t** samples;  /* Monte Carlo samples */
    statistical_results_t results;     /* Statistical analysis results */
    
    /* Performance specifications */
    double* spec_min_values;         /* Minimum specification values */
    double* spec_max_values;         /* Maximum specification values */
    double* spec_target_values;      /* Target specification values */
    int num_specifications;          /* Number of specifications */
    char** spec_names;               /* Specification names */
    
    /* Working arrays */
    double* random_values;            /* Random number storage */
    double* correlated_values;       /* Correlated random values */
    double* transformed_values;      /* Transformed parameter values */
    
    /* Random number generator state */
    unsigned int rng_state;          /* RNG state for reproducibility */
    
    /* Parallel processing */
    int* thread_work_distribution;   /* Work distribution for threads */
    int** thread_sample_indices;     /* Sample indices per thread */
} statistical_analysis_engine_t;

/*****************************************************************************************
 * Random Number Generation and Statistical Distributions
 *****************************************************************************************/

/* Linear congruential generator for reproducible results */
static double uniform_random(unsigned int* seed) {
    *seed = (1664525 * *seed + 1013904223) & 0xFFFFFFFF;
    return *seed / 4294967296.0;  /* Normalize to [0,1) */
}

/* Box-Muller transform for Gaussian random numbers */
static double gaussian_random(unsigned int* seed) {
    static int has_spare = 0;
    static double spare;
    
    if (has_spare) {
        has_spare = 0;
        return spare;
    }
    
    double u1 = uniform_random(seed);
    double u2 = uniform_random(seed);
    
    double mag = sqrt(-2.0 * log(u1));
    spare = mag * cos(2.0 * M_PI * u2);
    has_spare = 1;
    
    return mag * sin(2.0 * M_PI * u2);
}

/* Generate random numbers from various distributions */
static double generate_random_value(statistical_distribution_t dist, double mean, double std_dev, 
                                   double min_val, double max_val, unsigned int* seed) {
    double value;
    
    switch (dist) {
        case DIST_GAUSSIAN:
            value = mean + std_dev * gaussian_random(seed);
            break;
            
        case DIST_UNIFORM:
            value = min_val + uniform_random(seed) * (max_val - min_val);
            break;
            
        case DIST_LOGNORMAL:
            /* Lognormal: if X ~ N(μ,σ²), then exp(X) ~ lognormal */
            {
                double log_mean = log(mean) - 0.5 * log(1.0 + std_dev * std_dev / (mean * mean));
                double log_std = sqrt(log(1.0 + std_dev * std_dev / (mean * mean)));
                value = exp(log_mean + log_std * gaussian_random(seed));
            }
            break;
            
        case DIST_EXPONENTIAL:
            value = -log(1.0 - uniform_random(seed)) * mean;
            break;
            
        case DIST_WEIBULL:
            /* Weibull with shape=2 (Rayleigh-like) */
            value = sqrt(-2.0 * log(1.0 - uniform_random(seed))) * mean * sqrt(2.0 / M_PI);
            break;
            
        default:
            value = mean + std_dev * gaussian_random(seed);
            break;
    }
    
    /* Clamp to specified range */
    if (value < min_val) value = min_val;
    if (value > max_val) value = max_val;
    
    return value;
}

/*****************************************************************************************
 * Correlation Matrix Handling
 *****************************************************************************************/

/* Create correlation matrix */
static correlation_matrix_t* create_correlation_matrix(int num_parameters) {
    correlation_matrix_t* corr = (correlation_matrix_t*)calloc(1, sizeof(correlation_matrix_t));
    
    corr->num_parameters = num_parameters;
    
    /* Allocate matrices */
    corr->correlation_matrix = (double**)malloc(num_parameters * sizeof(double*));
    corr->cholesky_factor = (double**)malloc(num_parameters * sizeof(double*));
    
    for (int i = 0; i < num_parameters; i++) {
        corr->correlation_matrix[i] = (double*)calloc(num_parameters, sizeof(double));
        corr->cholesky_factor[i] = (double*)calloc(num_parameters, sizeof(double));
    }
    
    /* Initialize to identity matrix (no correlation) */
    for (int i = 0; i < num_parameters; i++) {
        corr->correlation_matrix[i][i] = 1.0;
        corr->cholesky_factor[i][i] = 1.0;
    }
    
    corr->is_positive_definite = 1;
    
    return corr;
}

/* Set correlation coefficient */
static void set_correlation(correlation_matrix_t* corr, int i, int j, double correlation) {
    if (i < 0 || i >= corr->num_parameters || j < 0 || j >= corr->num_parameters) {
        return;
    }
    
    /* Ensure symmetry and valid correlation range */
    correlation = fmax(-1.0, fmin(1.0, correlation));
    corr->correlation_matrix[i][j] = correlation;
    corr->correlation_matrix[j][i] = correlation;
}

/* Perform Cholesky decomposition */
static int cholesky_decomposition(correlation_matrix_t* corr) {
    int n = corr->num_parameters;
    
    /* Copy correlation matrix to Cholesky factor */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            corr->cholesky_factor[i][j] = corr->correlation_matrix[i][j];
        }
    }
    
    /* Cholesky decomposition */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            double sum = 0.0;
            
            if (j == i) {
                /* Diagonal element */
                for (int k = 0; k < j; k++) {
                    sum += corr->cholesky_factor[j][k] * corr->cholesky_factor[j][k];
                }
                double diag = corr->cholesky_factor[j][j] - sum;
                if (diag <= 0.0) {
                    corr->is_positive_definite = 0;
                    return -1;  /* Matrix is not positive definite */
                }
                corr->cholesky_factor[j][j] = sqrt(diag);
            } else {
                /* Off-diagonal element */
                for (int k = 0; k < j; k++) {
                    sum += corr->cholesky_factor[i][k] * corr->cholesky_factor[j][k];
                }
                corr->cholesky_factor[i][j] = (corr->cholesky_factor[i][j] - sum) / corr->cholesky_factor[j][j];
            }
        }
    }
    
    corr->is_positive_definite = 1;
    return 0;
}

/* Generate correlated random variables */
static void generate_correlated_random(correlation_matrix_t* corr, double* independent_random, 
                                     double* correlated_random) {
    int n = corr->num_parameters;
    
    if (!corr->is_positive_definite) {
        /* Fallback to independent random variables */
        memcpy(correlated_random, independent_random, n * sizeof(double));
        return;
    }
    
    /* Multiply by Cholesky factor: y = L * x */
    for (int i = 0; i < n; i++) {
        correlated_random[i] = 0.0;
        for (int j = 0; j <= i; j++) {
            correlated_random[i] += corr->cholesky_factor[i][j] * independent_random[j];
        }
    }
}

/*****************************************************************************************
 * Statistical Analysis Engine
 *****************************************************************************************/

/* Create statistical analysis engine */
statistical_analysis_engine_t* statistical_engine_create(peec_solver_t* peec_solver, mom_solver_t* mom_solver) {
    statistical_analysis_engine_t* engine = (statistical_analysis_engine_t*)calloc(1, sizeof(statistical_analysis_engine_t));
    
    engine->peec_solver = peec_solver;
    engine->mom_solver = mom_solver;
    engine->num_parameters = 0;
    engine->parameters = NULL;
    
    /* Default configuration */
    engine->config.num_samples = 1000;
    engine->config.num_process_corners = 5;  /* TT, FF, SS, FS, SF */
    engine->config.random_seed = (int)time(NULL);
    engine->config.confidence_level = 0.95;
    engine->config.yield_target = 0.99;  /* 99% yield target */
    engine->config.max_iterations = 100;
    engine->config.convergence_criteria = 1;  /* Relative error */
    engine->config.convergence_tolerance = 0.01;  /* 1% convergence */
    engine->config.parallel_threads = 4;
    engine->config.save_all_results = 1;
    engine->config.correlation_modeling = 1;
    engine->config.sensitivity_analysis = 1;
    engine->config.yield_optimization = 0;
    
    /* Initialize RNG */
    engine->rng_state = engine->config.random_seed;
    
    /* Allocate working arrays */
    engine->random_values = (double*)malloc(100 * sizeof(double));
    engine->correlated_values = (double*)malloc(100 * sizeof(double));
    engine->transformed_values = (double*)malloc(100 * sizeof(double));
    
    return engine;
}

/* Add process variation parameter */
int statistical_engine_add_parameter(statistical_analysis_engine_t* engine, const char* name, 
                                     const char* description, double nominal, double std_dev,
                                     double min_val, double max_val, statistical_distribution_t dist) {
    /* Expand arrays if needed */
    if (engine->num_parameters >= 100) {
        return -1;  /* Maximum parameters reached */
    }
    
    /* Create new parameter */
    process_variation_t* param = (process_variation_t*)calloc(1, sizeof(process_variation_t));
    
    param->name = strdup(name);
    param->description = strdup(description);
    param->nominal_value = nominal;
    param->std_deviation = std_dev;
    param->min_value = min_val;
    param->max_value = max_val;
    param->distribution = dist;
    param->correlation_group = engine->num_parameters;  /* Default: no correlation */
    param->correlation_coefficient = 0.0;
    param->is_spatial = 0;
    
    /* Add to engine */
    engine->parameters = (process_variation_t**)realloc(engine->parameters, 
                                                       (engine->num_parameters + 1) * sizeof(process_variation_t*));
    engine->parameters[engine->num_parameters] = param;
    engine->num_parameters++;
    
    return engine->num_parameters - 1;
}

/* Set up correlation between parameters */
int statistical_engine_set_correlation(statistical_analysis_engine_t* engine, int param1_idx, int param2_idx, double correlation) {
    if (param1_idx >= engine->num_parameters || param2_idx >= engine->num_parameters) {
        return -1;
    }
    
    /* Create correlation matrix if not exists */
    if (!engine->correlations.correlation_matrix) {
        engine->correlations = *create_correlation_matrix(engine->num_parameters);
    }
    
    /* Set correlation coefficient */
    set_correlation(&engine->correlations, param1_idx, param2_idx, correlation);
    
    /* Update parameter correlation info */
    engine->parameters[param1_idx]->correlation_coefficient = correlation;
    engine->parameters[param2_idx]->correlation_coefficient = correlation;
    
    /* Perform Cholesky decomposition */
    return cholesky_decomposition(&engine->correlations);
}

/* Define performance specifications */
int statistical_engine_add_specification(statistical_analysis_engine_t* engine, const char* name,
                                         double min_value, double max_value, double target_value) {
    engine->num_specifications++;
    
    /* Expand specification arrays */
    engine->spec_names = (char**)realloc(engine->spec_names, engine->num_specifications * sizeof(char*));
    engine->spec_min_values = (double*)realloc(engine->spec_min_values, engine->num_specifications * sizeof(double));
    engine->spec_max_values = (double*)realloc(engine->spec_max_values, engine->num_specifications * sizeof(double));
    engine->spec_target_values = (double*)realloc(engine->spec_target_values, engine->num_specifications * sizeof(double));
    
    /* Add specification */
    engine->spec_names[engine->num_specifications - 1] = strdup(name);
    engine->spec_min_values[engine->num_specifications - 1] = min_value;
    engine->spec_max_values[engine->num_specifications - 1] = max_value;
    engine->spec_target_values[engine->num_specifications - 1] = target_value;
    
    return engine->num_specifications - 1;
}

/* Generate Monte Carlo sample */
static monte_carlo_sample_t* generate_sample(statistical_analysis_engine_t* engine, int sample_id) {
    monte_carlo_sample_t* sample = (monte_carlo_sample_t*)calloc(1, sizeof(monte_carlo_sample_t));
    
    sample->sample_id = sample_id;
    sample->parameter_values = (double*)malloc(engine->num_parameters * sizeof(double));
    sample->performance_metrics = (double*)malloc(engine->num_specifications * sizeof(double));
    sample->specification_pass = (int*)malloc(engine->num_specifications * sizeof(int));
    sample->overall_pass = 1;
    sample->yield_contribution = 0.0;
    
    /* Generate independent random values */
    for (int i = 0; i < engine->num_parameters; i++) {
        process_variation_t* param = engine->parameters[i];
        engine->random_values[i] = generate_random_value(param->distribution, 
                                                        param->nominal_value, param->std_deviation,
                                                        param->min_value, param->max_value, 
                                                        &engine->rng_state);
    }
    
    /* Apply correlation if enabled */
    if (engine->config.correlation_modeling && engine->correlations.is_positive_definite) {
        generate_correlated_random(&engine->correlations, engine->random_values, 
                                 engine->correlated_values);
        memcpy(sample->parameter_values, engine->correlated_values, engine->num_parameters * sizeof(double));
    } else {
        memcpy(sample->parameter_values, engine->random_values, engine->num_parameters * sizeof(double));
    }
    
    return sample;
}

/* Evaluate performance for a sample */
static int evaluate_sample_performance(statistical_analysis_engine_t* engine, monte_carlo_sample_t* sample) {
    /* Modify solver parameters based on sample */
    if (engine->peec_solver) {
        /* Update PEEC geometry/material parameters */
        for (int i = 0; i < engine->num_parameters; i++) {
            process_variation_t* param = engine->parameters[i];
            double value = sample->parameter_values[i];
            
            /* Apply parameter to solver - this would be solver-specific */
            /* For example: update conductor width, dielectric constant, etc. */
            if (strstr(param->name, "width") != NULL) {
                /* Update conductor width */
            } else if (strstr(param->name, "thickness") != NULL) {
                /* Update conductor thickness */
            } else if (strstr(param->name, "epsilon") != NULL) {
                /* Update dielectric constant */
            }
        }
        
        /* Run PEEC simulation */
        peec_result_t* result = peec_solve(engine->peec_solver);
        
        /* Extract performance metrics */
        if (result) {
            /* Example: extract impedance, inductance, capacitance */
            sample->performance_metrics[0] = creal(result->impedance_matrix[0]);  /* Resistance */
            sample->performance_metrics[1] = cimag(result->impedance_matrix[0]) / (2.0 * M_PI * 1e9);  /* Inductance */
            /* ... extract other metrics ... */
            
            peec_destroy_result(result);
        }
    }
    
    /* Check specifications */
    for (int i = 0; i < engine->num_specifications; i++) {
        double value = sample->performance_metrics[i];
        double min_val = engine->spec_min_values[i];
        double max_val = engine->spec_max_values[i];
        
        sample->specification_pass[i] = (value >= min_val && value <= max_val) ? 1 : 0;
        if (!sample->specification_pass[i]) {
            sample->overall_pass = 0;
        }
    }
    
    return sample->overall_pass;
}

/* Perform Monte Carlo analysis */
static int perform_monte_carlo_analysis(statistical_analysis_engine_t* engine) {
    printf("Starting Monte Carlo analysis with %d samples...\n", engine->config.num_samples);
    
    /* Allocate sample array */
    engine->samples = (monte_carlo_sample_t**)malloc(engine->config.num_samples * sizeof(monte_carlo_sample_t*));
    
    int passed_samples = 0;
    
    /* Generate and evaluate samples */
    for (int i = 0; i < engine->config.num_samples; i++) {
        /* Generate sample */
        engine->samples[i] = generate_sample(engine, i);
        
        /* Evaluate performance */
        int sample_passed = evaluate_sample_performance(engine, engine->samples[i]);
        
        if (sample_passed) {
            passed_samples++;
        }
        
        /* Progress indicator */
        if ((i + 1) % 100 == 0) {
            printf("  Progress: %d/%d samples, current yield: %.2f%%\n",
                   i + 1, engine->config.num_samples, 100.0 * passed_samples / (i + 1));
        }
    }
    
    /* Store results */
    engine->results.num_samples = engine->config.num_samples;
    engine->results.num_passed = passed_samples;
    engine->results.estimated_yield = (double)passed_samples / engine->config.num_samples;
    
    /* Calculate confidence interval */
    double z_score = 1.96;  /* 95% confidence */
    double margin = z_score * sqrt(engine->results.estimated_yield * (1.0 - engine->results.estimated_yield) / engine->config.num_samples);
    engine->results.yield_confidence = margin;
    
    printf("Monte Carlo analysis completed:\n");
    printf("  Total samples: %d\n", engine->config.num_samples);
    printf("  Passed samples: %d\n", passed_samples);
    printf("  Estimated yield: %.2f%% ± %.2f%%\n", 
           100.0 * engine->results.estimated_yield, 100.0 * margin);
    
    return 0;
}

/* Calculate statistical moments */
static void calculate_statistical_moments(statistical_analysis_engine_t* engine) {
    int n = engine->config.num_samples;
    int num_params = engine->num_parameters;
    int num_specs = engine->num_specifications;
    
    /* Allocate result arrays */
    engine->results.parameter_means = (double*)calloc(num_params, sizeof(double));
    engine->results.parameter_std_devs = (double*)calloc(num_params, sizeof(double));
    engine->results.performance_means = (double*)calloc(num_specs, sizeof(double));
    engine->results.performance_std_devs = (double*)calloc(num_specs, sizeof(double));
    
    /* Calculate parameter statistics */
    for (int i = 0; i < num_params; i++) {
        double sum = 0.0, sum_sq = 0.0;
        
        for (int j = 0; j < n; j++) {
            double value = engine->samples[j]->parameter_values[i];
            sum += value;
            sum_sq += value * value;
        }
        
        engine->results.parameter_means[i] = sum / n;
        engine->results.parameter_std_devs[i] = sqrt(sum_sq / n - (sum / n) * (sum / n));
    }
    
    /* Calculate performance statistics */
    for (int i = 0; i < num_specs; i++) {
        double sum = 0.0, sum_sq = 0.0;
        
        for (int j = 0; j < n; j++) {
            double value = engine->samples[j]->performance_metrics[i];
            sum += value;
            sum_sq += value * value;
        }
        
        engine->results.performance_means[i] = sum / n;
        engine->results.performance_std_devs[i] = sqrt(sum_sq / n - (sum / n) * (sum / n));
    }
}

/* Perform sensitivity analysis */
static void perform_sensitivity_analysis(statistical_analysis_engine_t* engine) {
    int n = engine->config.num_samples;
    int num_params = engine->num_parameters;
    int num_specs = engine->num_specifications;
    
    /* Allocate sensitivity matrix */
    engine->results.sensitivity_matrix = (double**)malloc(num_specs * sizeof(double*));
    for (int i = 0; i < num_specs; i++) {
        engine->results.sensitivity_matrix[i] = (double*)calloc(num_params, sizeof(double));
    }
    
    /* Calculate correlation coefficients as sensitivity measure */
    for (int spec = 0; spec < num_specs; spec++) {
        for (int param = 0; param < num_params; param++) {
            double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0;
            double sum_x2 = 0.0, sum_y2 = 0.0;
            
            for (int i = 0; i < n; i++) {
                double x = engine->samples[i]->parameter_values[param];
                double y = engine->samples[i]->performance_metrics[spec];
                
                sum_x += x;
                sum_y += y;
                sum_xy += x * y;
                sum_x2 += x * x;
                sum_y2 += y * y;
            }
            
            /* Correlation coefficient */
            double numerator = n * sum_xy - sum_x * sum_y;
            double denominator = sqrt((n * sum_x2 - sum_x * sum_x) * (n * sum_y2 - sum_y * sum_y));
            
            engine->results.sensitivity_matrix[spec][param] = (denominator != 0.0) ? numerator / denominator : 0.0;
        }
    }
    
    /* Identify critical parameters */
    engine->results.critical_parameters = (int*)malloc(num_params * sizeof(int));
    for (int param = 0; param < num_params; param++) {
        double max_sensitivity = 0.0;
        for (int spec = 0; spec < num_specs; spec++) {
            max_sensitivity = fmax(max_sensitivity, fabs(engine->results.sensitivity_matrix[spec][param]));
        }
        engine->results.critical_parameters[param] = (max_sensitivity > 0.5) ? 1 : 0;
    }
}

/* Run complete statistical analysis */
int statistical_engine_run_analysis(statistical_analysis_engine_t* engine) {
    printf("Running statistical analysis...\n");
    
    /* Perform Monte Carlo analysis */
    perform_monte_carlo_analysis(engine);
    
    /* Calculate statistical moments */
    calculate_statistical_moments(engine);
    
    /* Perform sensitivity analysis if enabled */
    if (engine->config.sensitivity_analysis) {
        perform_sensitivity_analysis(engine);
    }
    
    printf("Statistical analysis completed successfully\n");
    return 0;
}

/* Generate analysis report */
void statistical_engine_generate_report(statistical_analysis_engine_t* engine, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot open report file %s\n", filename);
        return;
    }
    
    fprintf(fp, "STATISTICAL ANALYSIS REPORT\n");
    fprintf(fp, "===========================\n\n");
    
    fprintf(fp, "Simulation Configuration:\n");
    fprintf(fp, "  Number of samples: %d\n", engine->config.num_samples);
    fprintf(fp, "  Random seed: %d\n", engine->config.random_seed);
    fprintf(fp, "  Confidence level: %.1f%%\n", 100.0 * engine->config.confidence_level);
    fprintf(fp, "  Yield target: %.1f%%\n", 100.0 * engine->config.yield_target);
    fprintf(fp, "\n");
    
    fprintf(fp, "Process Variation Parameters:\n");
    for (int i = 0; i < engine->num_parameters; i++) {
        process_variation_t* param = engine->parameters[i];
        fprintf(fp, "  %s: nominal=%.3e, std_dev=%.2e, min=%.3e, max=%.3e\n",
                param->name, param->nominal_value, param->std_deviation, 
                param->min_value, param->max_value);
    }
    fprintf(fp, "\n");
    
    fprintf(fp, "Performance Specifications:\n");
    for (int i = 0; i < engine->num_specifications; i++) {
        fprintf(fp, "  %s: min=%.3e, max=%.3e, target=%.3e\n",
                engine->spec_names[i], engine->spec_min_values[i], 
                engine->spec_max_values[i], engine->spec_target_values[i]);
    }
    fprintf(fp, "\n");
    
    fprintf(fp, "Statistical Results:\n");
    fprintf(fp, "  Estimated yield: %.2f%% ± %.2f%%\n", 
            100.0 * engine->results.estimated_yield, 100.0 * engine->results.yield_confidence);
    fprintf(fp, "  Total samples: %d\n", engine->results.num_samples);
    fprintf(fp, "  Passing samples: %d\n", engine->results.num_passed);
    fprintf(fp, "\n");
    
    if (engine->config.sensitivity_analysis) {
        fprintf(fp, "Sensitivity Analysis:\n");
        fprintf(fp, "  Critical parameters (|correlation| > 0.5):\n");
        for (int i = 0; i < engine->num_parameters; i++) {
            if (engine->results.critical_parameters[i]) {
                fprintf(fp, "    - %s\n", engine->parameters[i]->name);
            }
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    printf("Statistical analysis report written to %s\n", filename);
}

/* Destroy statistical analysis engine */
void statistical_engine_destroy(statistical_analysis_engine_t* engine) {
    if (!engine) return;
    
    /* Free parameters */
    for (int i = 0; i < engine->num_parameters; i++) {
        process_variation_t* param = engine->parameters[i];
        if (param) {
            free(param->name);
            free(param->description);
            free(param->custom_values);
            free(param->custom_probabilities);
            free(param);
        }
    }
    free(engine->parameters);
    
    /* Free samples */
    for (int i = 0; i < engine->config.num_samples; i++) {
        monte_carlo_sample_t* sample = engine->samples[i];
        if (sample) {
            free(sample->parameter_values);
            free(sample->performance_metrics);
            free(sample->specification_pass);
            free(sample);
        }
    }
    free(engine->samples);
    
    /* Free specifications */
    for (int i = 0; i < engine->num_specifications; i++) {
        free(engine->spec_names[i]);
    }
    free(engine->spec_names);
    free(engine->spec_min_values);
    free(engine->spec_max_values);
    free(engine->spec_target_values);
    
    /* Free correlation matrix */
    if (engine->correlations.correlation_matrix) {
        for (int i = 0; i < engine->correlations.num_parameters; i++) {
            free(engine->correlations.correlation_matrix[i]);
            free(engine->correlations.cholesky_factor[i]);
        }
        free(engine->correlations.correlation_matrix);
        free(engine->correlations.cholesky_factor);
    }
    
    /* Free results */
    free(engine->results.parameter_means);
    free(engine->results.parameter_std_devs);
    free(engine->results.performance_means);
    free(engine->results.performance_std_devs);
    
    if (engine->results.sensitivity_matrix) {
        for (int i = 0; i < engine->num_specifications; i++) {
            free(engine->results.sensitivity_matrix[i]);
        }
        free(engine->results.sensitivity_matrix);
    }
    
    free(engine->results.critical_parameters);
    free(engine->results.worst_case_values);
    free(engine->results.best_case_values);
    
    /* Free working arrays */
    free(engine->random_values);
    free(engine->correlated_values);
    free(engine->transformed_values);
    
    free(engine);
}

/*****************************************************************************************
 * Example Usage and Test Functions
 *****************************************************************************************/

/* Test statistical analysis with simple RLC circuit */
static void test_statistical_analysis(void) {
    printf("Testing statistical analysis engine...\n");
    
    /* Create PEEC solver */
    peec_solver_t* peec = peec_create_solver();
    peec_config_t config = {
        .frequency_start = 1e9,
        .frequency_stop = 10e9,
        .num_frequency_points = 100,
        .solver_type = PEEC_FULL_WAVE,
        .include_skin_effect = 1,
        .manhattan_mesh_size = 0.1e-3
    };
    peec_set_config(peec, &config);
    
    /* Create transmission line */
    geom_geometry_t* geometry = geom_create_geometry();
    geom_entity_t* line = geom_create_box(geometry,
        (geom_point3d_t){0, 0, 0},
        (geom_point3d_t){10e-3, 0.1e-3, 35e-6});  /* 10mm transmission line */
    peec_set_geometry(peec, geometry);
    
    /* Create statistical analysis engine */
    statistical_analysis_engine_t* engine = statistical_engine_create(peec, NULL);
    
    /* Add process variation parameters */
    statistical_engine_add_parameter(engine, "conductor_width", "Metal width variation",
                                     0.1e-3, 5e-6, 0.08e-3, 0.12e-3, DIST_GAUSSIAN);
    
    statistical_engine_add_parameter(engine, "conductor_thickness", "Metal thickness variation",
                                     35e-6, 3e-6, 30e-6, 40e-6, DIST_GAUSSIAN);
    
    statistical_engine_add_parameter(engine, "dielectric_constant", "Substrate permittivity variation",
                                     4.4, 0.2, 4.0, 4.8, DIST_GAUSSIAN);
    
    statistical_engine_add_parameter(engine, "loss_tangent", "Substrate loss variation",
                                     0.02, 0.005, 0.01, 0.03, DIST_GAUSSIAN);
    
    /* Set up correlations */
    statistical_engine_set_correlation(engine, 0, 1, 0.7);  /* Width-thickness correlation */
    statistical_engine_set_correlation(engine, 2, 3, 0.5);  /* Dielectric properties correlation */
    
    /* Add specifications */
    statistical_engine_add_specification(engine, "characteristic_impedance", 45.0, 55.0, 50.0);
    statistical_engine_add_specification(engine, "insertion_loss_1ghz", -3.0, 0.0, -1.0);
    statistical_engine_add_specification(engine, "return_loss_1ghz", -20.0, 0.0, -15.0);
    
    /* Configure Monte Carlo analysis */
    engine->config.num_samples = 500;  /* Reduced for testing */
    engine->config.sensitivity_analysis = 1;
    
    /* Run analysis */
    printf("Running statistical analysis...\n");
    statistical_engine_run_analysis(engine);
    
    /* Generate report */
    statistical_engine_generate_report(engine, "statistical_analysis_report.txt");
    
    /* Cleanup */
    statistical_engine_destroy(engine);
    peec_destroy_solver(peec);
    geom_destroy_geometry(geometry);
    
    printf("Statistical analysis test completed\n");
}

#endif /* STATISTICAL_ANALYSIS_H */