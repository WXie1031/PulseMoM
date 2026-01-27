/********************************************************************************
 * Cavity and Waveguide Kernels Implementation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Implements Green's functions for cavity and waveguide problems
 ********************************************************************************/

#include "kernel_cavity_waveguide.h"
#include "core_common.h"
#include <math.h>

/**
 * @brief Compute rectangular cavity Green's function
 * 
 * For rectangular cavity, the Green's function is:
 * G(r, r') = Σ_{mnp} (2/(abc)) * sin(mπx/a) * sin(nπy/b) * sin(pπz/c) *
 *            sin(mπx'/a) * sin(nπy'/b) * sin(pπz'/c) / (k² - k_mnp²)
 * 
 * where k_mnp² = (mπ/a)² + (nπ/b)² + (pπ/c)²
 */
complex_t kernel_cavity_rectangular_green(const geom_point_t* r,
                                         const geom_point_t* r_prime,
                                         const rectangular_cavity_params_t* params,
                                         int m, int n, int p,
                                         double k) {
    if (!r || !r_prime || !params || m < 0 || n < 0 || p < 0) {
        return complex_zero();
    }
    
    if (params->a <= 0.0 || params->b <= 0.0 || params->c <= 0.0) {
        return complex_zero();
    }
    
    // Compute mode wavenumber
    double k_mnp_sq = (m * M_PI / params->a) * (m * M_PI / params->a) +
                      (n * M_PI / params->b) * (n * M_PI / params->b) +
                      (p * M_PI / params->c) * (p * M_PI / params->c);
    double k_mnp = sqrt(k_mnp_sq);
    
    // Compute mode functions
    double sin_mx = sin(m * M_PI * r->x / params->a);
    double sin_ny = sin(n * M_PI * r->y / params->b);
    double sin_pz = sin(p * M_PI * r->z / params->c);
    
    double sin_mx_prime = sin(m * M_PI * r_prime->x / params->a);
    double sin_ny_prime = sin(n * M_PI * r_prime->y / params->b);
    double sin_pz_prime = sin(p * M_PI * r_prime->z / params->c);
    
    // Compute mode product
    double mode_product = sin_mx * sin_ny * sin_pz * 
                         sin_mx_prime * sin_ny_prime * sin_pz_prime;
    
    // Compute denominator: k² - k_mnp²
    double k_sq = k * k;
    double denom = k_sq - k_mnp_sq;
    
    // Avoid division by zero (add small regularization)
    if (fabs(denom) < NUMERICAL_EPSILON) {
        denom = NUMERICAL_EPSILON;
    }
    
    // Compute Green's function
    double normalization = 2.0 / (params->a * params->b * params->c);
    double green_re = normalization * mode_product / denom;
    
    complex_t result;
    result.re = green_re;
    result.im = 0.0;  // Real for lossless cavity
    
    return result;
}

/**
 * @brief Compute rectangular waveguide Green's function
 * 
 * For rectangular waveguide, the Green's function depends on mode type:
 * - TE modes: G_TE = Σ_{mn} (2/(ab)) * cos(mπx/a) * cos(nπy/b) * ... / (k² - k_c²)
 * - TM modes: G_TM = Σ_{mn} (2/(ab)) * sin(mπx/a) * sin(nπy/b) * ... / (k² - k_c²)
 */
complex_t kernel_waveguide_rectangular_green(const geom_point_t* r,
                                            const geom_point_t* r_prime,
                                            const rectangular_waveguide_params_t* params,
                                            int m, int n,
                                            mode_type_t mode_type,
                                            double k) {
    if (!r || !r_prime || !params || m < 0 || n < 0) {
        return complex_zero();
    }
    
    if (params->a <= 0.0 || params->b <= 0.0) {
        return complex_zero();
    }
    
    // Compute cutoff wavenumber
    double k_c_sq = (m * M_PI / params->a) * (m * M_PI / params->a) +
                    (n * M_PI / params->b) * (n * M_PI / params->b);
    double k_c = sqrt(k_c_sq);
    
    // Compute mode functions based on mode type
    double mode_r, mode_r_prime;
    
    if (mode_type == MODE_TYPE_TE) {
        // TE modes: cos functions
        mode_r = cos(m * M_PI * r->x / params->a) * cos(n * M_PI * r->y / params->b);
        mode_r_prime = cos(m * M_PI * r_prime->x / params->a) * cos(n * M_PI * r_prime->y / params->b);
    } else if (mode_type == MODE_TYPE_TM) {
        // TM modes: sin functions
        mode_r = sin(m * M_PI * r->x / params->a) * sin(n * M_PI * r->y / params->b);
        mode_r_prime = sin(m * M_PI * r_prime->x / params->a) * sin(n * M_PI * r_prime->y / params->b);
    } else {
        return complex_zero();
    }
    
    // Compute mode product
    double mode_product = mode_r * mode_r_prime;
    
    // Compute denominator: k² - k_c²
    double k_sq = k * k;
    double denom = k_sq - k_c_sq;
    
    // Avoid division by zero
    if (fabs(denom) < NUMERICAL_EPSILON) {
        denom = NUMERICAL_EPSILON;
    }
    
    // Compute Green's function
    double normalization = 2.0 / (params->a * params->b);
    double green_re = normalization * mode_product / denom;
    
    complex_t result;
    result.re = green_re;
    result.im = 0.0;  // Real for lossless waveguide
    
    return result;
}

/**
 * @brief Compute cavity resonance frequency
 * 
 * f_mnp = (c0 / (2 * sqrt(eps_r * mu_r))) * sqrt((m/a)² + (n/b)² + (p/c)²)
 */
double kernel_cavity_resonance_frequency(const rectangular_cavity_params_t* params,
                                         int m, int n, int p) {
    if (!params || m < 0 || n < 0 || p < 0) {
        return 0.0;
    }
    
    if (params->a <= 0.0 || params->b <= 0.0 || params->c <= 0.0) {
        return 0.0;
    }
    
    // Compute mode wavenumber
    double k_mnp_sq = (m * M_PI / params->a) * (m * M_PI / params->a) +
                      (n * M_PI / params->b) * (n * M_PI / params->b) +
                      (p * M_PI / params->c) * (p * M_PI / params->c);
    double k_mnp = sqrt(k_mnp_sq);
    
    // Compute resonance frequency
    double eps_mu = params->eps_r * params->mu_r;
    if (eps_mu < NUMERICAL_EPSILON) {
        eps_mu = 1.0;  // Default to free space
    }
    
    double freq = (C0 / (2.0 * sqrt(eps_mu))) * k_mnp / M_PI;
    
    return freq;
}

/**
 * @brief Compute waveguide cutoff frequency
 * 
 * f_c = (c0 / (2 * sqrt(eps_r * mu_r))) * sqrt((m/a)² + (n/b)²)
 */
double kernel_waveguide_cutoff_frequency(const rectangular_waveguide_params_t* params,
                                       int m, int n,
                                       mode_type_t mode_type) {
    if (!params || m < 0 || n < 0) {
        return 0.0;
    }
    
    if (params->a <= 0.0 || params->b <= 0.0) {
        return 0.0;
    }
    
    // TE10 mode has m=1, n=0
    // For other modes, both m and n must be non-zero for TM modes
    if (mode_type == MODE_TYPE_TM && (m == 0 || n == 0)) {
        return 0.0;  // TM modes require both m and n > 0
    }
    
    // Compute cutoff wavenumber
    double k_c_sq = (m * M_PI / params->a) * (m * M_PI / params->a) +
                    (n * M_PI / params->b) * (n * M_PI / params->b);
    double k_c = sqrt(k_c_sq);
    
    // Compute cutoff frequency
    double eps_mu = params->eps_r * params->mu_r;
    if (eps_mu < NUMERICAL_EPSILON) {
        eps_mu = 1.0;  // Default to free space
    }
    
    double freq_c = (C0 / (2.0 * sqrt(eps_mu))) * k_c / M_PI;
    
    return freq_c;
}

/**
 * @brief Compute cavity mode expansion coefficient
 */
complex_t kernel_cavity_mode_coefficient(const rectangular_cavity_params_t* params,
                                        int m, int n, int p,
                                        double k) {
    if (!params || m < 0 || n < 0 || p < 0) {
        return complex_zero();
    }
    
    // Compute mode wavenumber
    double k_mnp_sq = (m * M_PI / params->a) * (m * M_PI / params->a) +
                      (n * M_PI / params->b) * (n * M_PI / params->b) +
                      (p * M_PI / params->c) * (p * M_PI / params->c);
    
    // Compute coefficient: 1 / (k² - k_mnp²)
    double k_sq = k * k;
    double denom = k_sq - k_mnp_sq;
    
    if (fabs(denom) < NUMERICAL_EPSILON) {
        denom = NUMERICAL_EPSILON;
    }
    
    complex_t result;
    result.re = 1.0 / denom;
    result.im = 0.0;
    
    return result;
}

/*********************************************************************
 * Bessel Function Implementation
 *********************************************************************/

/**
 * @brief Compute Bessel function of first kind J_n(x) using series expansion
 * 
 * J_n(x) = Σ_{k=0}^∞ [(-1)^k / (k! * Γ(n+k+1))] * (x/2)^(n+2k)
 */
double bessel_jn(int n, double x) {
    if (n < 0) {
        // J_{-n}(x) = (-1)^n * J_n(x)
        return ((n % 2 == 0) ? 1.0 : -1.0) * bessel_jn(-n, x);
    }
    
    if (x < 0.0) {
        // J_n(-x) = (-1)^n * J_n(x)
        return ((n % 2 == 0) ? 1.0 : -1.0) * bessel_jn(n, -x);
    }
    
    if (x == 0.0) {
        return (n == 0) ? 1.0 : 0.0;
    }
    
    // For large x, use asymptotic expansion
    if (x > 50.0) {
        double sqrt_2_pi_x = sqrt(2.0 / (M_PI * x));
        double phase = x - (n + 0.5) * M_PI * 0.5;
        return sqrt_2_pi_x * cos(phase);
    }
    
    // Series expansion for small to moderate x
    // Optimized: avoid pow() call, use iterative multiplication
    double sum = 0.0;
    double x_half = x * 0.5;
    double x_half_sq = x_half * x_half;  // Precompute (x/2)^2
    
    // Compute x_half_power = (x/2)^n using iterative multiplication (faster than pow)
    double x_half_power = 1.0;
    for (int i = 0; i < n; i++) {
        x_half_power *= x_half;
    }
    
    // Compute factorial and gamma function terms
    // Optimized: cache factorial values for small n
    static double factorial_cache[21] = {0};  // Cache for n! up to 20!
    static int factorial_cache_initialized = 0;
    
    if (!factorial_cache_initialized) {
        factorial_cache[0] = 1.0;
        for (int i = 1; i <= 20; i++) {
            factorial_cache[i] = factorial_cache[i-1] * i;
        }
        factorial_cache_initialized = 1;
    }
    
    double gamma_n_plus_1 = (n <= 20) ? factorial_cache[n] : 1.0;
    if (n > 20) {
        // Fallback for large n
        for (int i = 1; i <= n; i++) {
            gamma_n_plus_1 *= i;
        }
    }
    
    // Optimized series expansion with early termination
    double prev_sum = 0.0;
    for (int k = 0; k < 100; k++) {
        double term = x_half_power / gamma_n_plus_1;
        if (k % 2 == 1) {
            term = -term;
        }
        
        prev_sum = sum;
        sum += term;
        
        // Update for next iteration: x_half_power *= (x/2)^2
        x_half_power *= x_half_sq;
        gamma_n_plus_1 *= (n + k + 1);
        
        // Check convergence: early termination if change is negligible
        if (fabs(sum - prev_sum) < NUMERICAL_EPSILON * (fabs(sum) + 1.0)) {
            break;
        }
    }
    
    return sum;
}

/**
 * @brief Compute derivative of Bessel function J'_n(x)
 * 
 * J'_n(x) = (J_{n-1}(x) - J_{n+1}(x)) / 2
 */
double bessel_jn_prime(int n, double x) {
    if (n == 0) {
        // J'_0(x) = -J_1(x)
        return -bessel_jn(1, x);
    }
    
    // Recurrence relation: J'_n(x) = (J_{n-1}(x) - J_{n+1}(x)) / 2
    return 0.5 * (bessel_jn(n - 1, x) - bessel_jn(n + 1, x));
}

/**
 * @brief Find n-th root of J_m(x) = 0 (for TE modes)
 * Uses Newton-Raphson method with initial guess
 */
double bessel_jn_root(int m, int n) {
    if (n <= 0) return 0.0;
    
    // Initial guess based on asymptotic formula
    // For large n: j_{m,n} ≈ (n + m/2 - 1/4) * π
    double initial_guess = (n + m * 0.5 - 0.25) * M_PI;
    
    // Refine using Newton-Raphson
    double x = initial_guess;
    for (int iter = 0; iter < 50; iter++) {
        double J = bessel_jn(m, x);
        double J_prime = bessel_jn_prime(m, x);
        
        if (fabs(J_prime) < NUMERICAL_EPSILON) {
            break;
        }
        
        double x_new = x - J / J_prime;
        
        if (fabs(x_new - x) < NUMERICAL_EPSILON) {
            return x_new;
        }
        
        x = x_new;
    }
    
    return x;
}

/**
 * @brief Find n-th root of J'_m(x) = 0 (for TM modes)
 * Uses Newton-Raphson method with initial guess
 */
double bessel_jn_prime_root(int m, int n) {
    if (n <= 0) return 0.0;
    
    // Initial guess based on asymptotic formula
    double initial_guess = (n + m * 0.5 - 0.75) * M_PI;
    
    // Refine using Newton-Raphson
    double x = initial_guess;
    for (int iter = 0; iter < 50; iter++) {
        double J_prime = bessel_jn_prime(m, x);
        double J_double_prime = 0.5 * (bessel_jn_prime(m - 1, x) - bessel_jn_prime(m + 1, x));
        
        if (fabs(J_double_prime) < NUMERICAL_EPSILON) {
            break;
        }
        
        double x_new = x - J_prime / J_double_prime;
        
        if (fabs(x_new - x) < NUMERICAL_EPSILON) {
            return x_new;
        }
        
        x = x_new;
    }
    
    return x;
}

/*********************************************************************
 * Circular Cavity Green's Function
 *********************************************************************/

/**
 * @brief Convert Cartesian to cylindrical coordinates
 */
static void cartesian_to_cylindrical(const geom_point_t* r, double* rho, double* phi, double* z) {
    *rho = sqrt(r->x * r->x + r->y * r->y);
    *phi = atan2(r->y, r->x);
    *z = r->z;
}

/**
 * @brief Compute circular cavity Green's function
 */
complex_t kernel_cavity_circular_green(const geom_point_t* r,
                                       const geom_point_t* r_prime,
                                       const circular_cavity_params_t* params,
                                       int m, int n, int p,
                                       double k) {
    if (!r || !r_prime || !params || m < 0 || n < 0 || p < 0) {
        return complex_zero();
    }
    
    if (params->radius <= 0.0 || params->height <= 0.0) {
        return complex_zero();
    }
    
    // Convert to cylindrical coordinates
    double rho, phi, z;
    double rho_prime, phi_prime, z_prime;
    cartesian_to_cylindrical(r, &rho, &phi, &z);
    cartesian_to_cylindrical(r_prime, &rho_prime, &phi_prime, &z_prime);
    
    // Normalize radial coordinates
    double rho_norm = rho / params->radius;
    double rho_prime_norm = rho_prime / params->radius;
    
    if (rho_norm > 1.0 || rho_prime_norm > 1.0) {
        return complex_zero();  // Outside cavity
    }
    
    // Compute mode wavenumber
    // k_mn is the n-th root of J_m(k_mn * a) = 0 for TE modes
    // For cavity, we use TE mode roots
    double k_mn = bessel_jn_root(m, n) / params->radius;
    double k_p = p * M_PI / params->height;
    double k_mnp_sq = k_mn * k_mn + k_p * k_p;
    double k_mnp = sqrt(k_mnp_sq);
    
    // Compute Bessel functions
    double J_m_rho = bessel_jn(m, k_mn * rho);
    double J_m_rho_prime = bessel_jn(m, k_mn * rho_prime);
    
    // Compute mode functions
    double cos_m_phi = cos(m * (phi - phi_prime));
    double sin_p_z = sin(p * M_PI * z / params->height);
    double sin_p_z_prime = sin(p * M_PI * z_prime / params->height);
    
    // Compute mode product
    double mode_product = J_m_rho * J_m_rho_prime * cos_m_phi * 
                         sin_p_z * sin_p_z_prime;
    
    // Compute denominator: k² - k_mnp²
    double k_sq = k * k;
    double denom = k_sq - k_mnp_sq;
    
    // Avoid division by zero
    if (fabs(denom) < NUMERICAL_EPSILON) {
        denom = NUMERICAL_EPSILON;
    }
    
    // Compute Green's function normalization
    // Normalization factor: 2 / (π * a² * h * J_{m+1}²(k_mn * a))
    double J_m_plus_1_at_boundary = bessel_jn(m + 1, bessel_jn_root(m, n));
    double J_m_plus_1_sq = J_m_plus_1_at_boundary * J_m_plus_1_at_boundary;
    double normalization = 2.0 / (M_PI * params->radius * params->radius * 
                                   params->height * J_m_plus_1_sq);
    
    double green_re = normalization * mode_product / denom;
    
    complex_t result;
    result.re = green_re;
    result.im = 0.0;  // Real for lossless cavity
    
    return result;
}

/*********************************************************************
 * Circular Waveguide Green's Function
 *********************************************************************/

/**
 * @brief Compute circular waveguide Green's function
 */
complex_t kernel_waveguide_circular_green(const geom_point_t* r,
                                          const geom_point_t* r_prime,
                                          const circular_waveguide_params_t* params,
                                          int m, int n,
                                          mode_type_t mode_type,
                                          double k) {
    if (!r || !r_prime || !params || m < 0 || n < 0) {
        return complex_zero();
    }
    
    if (params->radius <= 0.0) {
        return complex_zero();
    }
    
    // Convert to cylindrical coordinates
    double rho, phi, z;
    double rho_prime, phi_prime, z_prime;
    cartesian_to_cylindrical(r, &rho, &phi, &z);
    cartesian_to_cylindrical(r_prime, &rho_prime, &phi_prime, &z_prime);
    
    // Normalize radial coordinates
    double rho_norm = rho / params->radius;
    double rho_prime_norm = rho_prime / params->radius;
    
    if (rho_norm > 1.0 || rho_prime_norm > 1.0) {
        return complex_zero();  // Outside waveguide
    }
    
    // Compute cutoff wavenumber based on mode type
    double k_c;
    if (mode_type == MODE_TYPE_TE) {
        // TE modes: k_c is n-th root of J_m(k_c * a) = 0
        k_c = bessel_jn_root(m, n) / params->radius;
    } else if (mode_type == MODE_TYPE_TM) {
        // TM modes: k_c is n-th root of J'_m(k_c * a) = 0
        k_c = bessel_jn_prime_root(m, n) / params->radius;
    } else {
        return complex_zero();
    }
    
    // Compute Bessel functions
    double J_m_rho, J_m_rho_prime;
    if (mode_type == MODE_TYPE_TE) {
        J_m_rho = bessel_jn(m, k_c * rho);
        J_m_rho_prime = bessel_jn(m, k_c * rho_prime);
    } else {
        // For TM modes, use J_m but with different roots
        J_m_rho = bessel_jn(m, k_c * rho);
        J_m_rho_prime = bessel_jn(m, k_c * rho_prime);
    }
    
    // Compute mode functions
    double cos_m_phi = cos(m * (phi - phi_prime));
    
    // Compute mode product
    double mode_product = J_m_rho * J_m_rho_prime * cos_m_phi;
    
    // Compute denominator: k² - k_c²
    double k_sq = k * k;
    double denom = k_sq - k_c * k_c;
    
    // Avoid division by zero
    if (fabs(denom) < NUMERICAL_EPSILON) {
        denom = NUMERICAL_EPSILON;
    }
    
    // Compute Green's function normalization
    double normalization;
    if (mode_type == MODE_TYPE_TE) {
        double J_m_plus_1_at_boundary = bessel_jn(m + 1, bessel_jn_root(m, n));
        double J_m_plus_1_sq = J_m_plus_1_at_boundary * J_m_plus_1_at_boundary;
        normalization = 2.0 / (M_PI * params->radius * params->radius * J_m_plus_1_sq);
    } else {
        // TM mode normalization
        double J_m_at_boundary = bessel_jn(m, bessel_jn_prime_root(m, n));
        double J_m_sq = J_m_at_boundary * J_m_at_boundary;
        normalization = 2.0 / (M_PI * params->radius * params->radius * J_m_sq);
    }
    
    double green_re = normalization * mode_product / denom;
    
    complex_t result;
    result.re = green_re;
    result.im = 0.0;  // Real for lossless waveguide
    
    return result;
}

/*********************************************************************
 * Circular Cavity/Waveguide Frequency Calculations
 *********************************************************************/

/**
 * @brief Compute circular cavity resonance frequency
 */
double kernel_cavity_circular_resonance_frequency(const circular_cavity_params_t* params,
                                                  int m, int n, int p) {
    if (!params || m < 0 || n < 0 || p < 0) {
        return 0.0;
    }
    
    if (params->radius <= 0.0 || params->height <= 0.0) {
        return 0.0;
    }
    
    // Compute mode wavenumber
    double k_mn = bessel_jn_root(m, n) / params->radius;
    double k_p = p * M_PI / params->height;
    double k_mnp_sq = k_mn * k_mn + k_p * k_p;
    double k_mnp = sqrt(k_mnp_sq);
    
    // Compute resonance frequency
    double eps_mu = params->eps_r * params->mu_r;
    if (eps_mu < NUMERICAL_EPSILON) {
        eps_mu = 1.0;  // Default to free space
    }
    
    double freq = (C0 / (2.0 * sqrt(eps_mu))) * k_mnp / M_PI;
    
    return freq;
}

/**
 * @brief Compute circular waveguide cutoff frequency
 */
double kernel_waveguide_circular_cutoff_frequency(const circular_waveguide_params_t* params,
                                                  int m, int n,
                                                  mode_type_t mode_type) {
    if (!params || m < 0 || n < 0) {
        return 0.0;
    }
    
    if (params->radius <= 0.0) {
        return 0.0;
    }
    
    // Compute cutoff wavenumber
    double k_c;
    if (mode_type == MODE_TYPE_TE) {
        k_c = bessel_jn_root(m, n) / params->radius;
    } else if (mode_type == MODE_TYPE_TM) {
        k_c = bessel_jn_prime_root(m, n) / params->radius;
    } else {
        return 0.0;
    }
    
    // Compute cutoff frequency
    double eps_mu = params->eps_r * params->mu_r;
    if (eps_mu < NUMERICAL_EPSILON) {
        eps_mu = 1.0;  // Default to free space
    }
    
    double freq_c = (C0 / (2.0 * sqrt(eps_mu))) * k_c / M_PI;
    
    return freq_c;
}

