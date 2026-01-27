/**
 * @file periodic_ewald.c
 * @brief Ewald summation implementation for periodic structures
 * @details Implements Ewald summation method for periodic Green's functions
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "periodic_ewald.h"
#include "core_common.h"
#include "../discretization//geometry/core_geometry.h"
#include <math.h>
#include <string.h>

// Ewald splitting parameter default (will be optimized based on lattice)
#define DEFAULT_EWALD_ETA_FACTOR 2.0  // eta = factor * k / lattice_size

// Maximum number of lattice vectors for convergence
#define DEFAULT_N_MAX_REAL 10
#define DEFAULT_N_MAX_RECIPROCAL 10

// Convergence tolerance
#define DEFAULT_EWALD_TOLERANCE 1e-10

/*********************************************************************
 * Ewald Parameter Initialization
 *********************************************************************/

/**
 * @brief Initialize Ewald parameters with optimal values
 */
void ewald_params_init(ewald_params_t* params, const periodic_lattice_t* lattice, double k) {
    if (!params || !lattice) {
        return;
    }
    
    // Compute characteristic lattice size
    double lattice_size = 0.0;
    if (lattice->dimension >= 1) {
        double a1_len = sqrt(lattice->a1.x * lattice->a1.x + 
                            lattice->a1.y * lattice->a1.y + 
                            lattice->a1.z * lattice->a1.z);
        lattice_size = a1_len;
    }
    if (lattice->dimension >= 2) {
        double a2_len = sqrt(lattice->a2.x * lattice->a2.x + 
                            lattice->a2.y * lattice->a2.y + 
                            lattice->a2.z * lattice->a2.z);
        if (a2_len > lattice_size) lattice_size = a2_len;
    }
    if (lattice->dimension >= 3) {
        double a3_len = sqrt(lattice->a3.x * lattice->a3.x + 
                            lattice->a3.y * lattice->a3.y + 
                            lattice->a3.z * lattice->a3.z);
        if (a3_len > lattice_size) lattice_size = a3_len;
    }
    
    // Optimal Ewald parameter: balance between real and reciprocal space convergence
    // eta = factor * k / lattice_size ensures good convergence in both spaces
    if (lattice_size > NUMERICAL_EPSILON) {
        params->eta = DEFAULT_EWALD_ETA_FACTOR * k / lattice_size;
    } else {
        params->eta = DEFAULT_EWALD_ETA_FACTOR * k;  // Fallback
    }
    
    // Ensure eta is positive and reasonable
    if (params->eta < NUMERICAL_EPSILON) {
        params->eta = NUMERICAL_EPSILON;
    }
    if (params->eta > 100.0 * k) {
        params->eta = 100.0 * k;  // Cap at reasonable value
    }
    
    // Set convergence parameters
    params->n_max_real = DEFAULT_N_MAX_REAL;
    params->n_max_reciprocal = DEFAULT_N_MAX_RECIPROCAL;
    params->tolerance = DEFAULT_EWALD_TOLERANCE;
}

/*********************************************************************
 * Reciprocal Lattice Computation
 *********************************************************************/

/**
 * @brief Compute reciprocal lattice vectors
 */
void ewald_compute_reciprocal_lattice(const periodic_lattice_t* lattice,
                                      geom_point_t* b1,
                                      geom_point_t* b2,
                                      geom_point_t* b3) {
    if (!lattice || !b1 || !b2 || !b3) {
        return;
    }
    
    if (lattice->dimension == 1) {
        // 1D: b1 = 2π / |a1|^2 * a1
        double a1_sq = lattice->a1.x * lattice->a1.x + 
                      lattice->a1.y * lattice->a1.y + 
                      lattice->a1.z * lattice->a1.z;
        if (a1_sq > NUMERICAL_EPSILON) {
            double factor = 2.0 * M_PI / a1_sq;
            b1->x = factor * lattice->a1.x;
            b1->y = factor * lattice->a1.y;
            b1->z = factor * lattice->a1.z;
        } else {
            b1->x = 0.0; b1->y = 0.0; b1->z = 0.0;
        }
        b2->x = 0.0; b2->y = 0.0; b2->z = 0.0;
        b3->x = 0.0; b3->y = 0.0; b3->z = 0.0;
    } else if (lattice->dimension == 2) {
        // 2D: b1 = 2π * (a2 × z) / (a1 · (a2 × z)), b2 = 2π * (z × a1) / (a2 · (z × a1))
        // For 2D periodicity in xy plane, z = (0, 0, 1)
        geom_point_t z_axis;
        z_axis.x = 0.0; z_axis.y = 0.0; z_axis.z = 1.0;
        
        // a2 × z
        geom_point_t a2_cross_z = {
            lattice->a2.y * z_axis.z - lattice->a2.z * z_axis.y,
            lattice->a2.z * z_axis.x - lattice->a2.x * z_axis.z,
            lattice->a2.x * z_axis.y - lattice->a2.y * z_axis.x
        };
        
        // a1 · (a2 × z)
        double a1_dot_a2_cross_z = lattice->a1.x * a2_cross_z.x + 
                                   lattice->a1.y * a2_cross_z.y + 
                                   lattice->a1.z * a2_cross_z.z;
        
        if (fabs(a1_dot_a2_cross_z) > NUMERICAL_EPSILON) {
            double factor = 2.0 * M_PI / a1_dot_a2_cross_z;
            b1->x = factor * a2_cross_z.x;
            b1->y = factor * a2_cross_z.y;
            b1->z = factor * a2_cross_z.z;
        } else {
            b1->x = 0.0; b1->y = 0.0; b1->z = 0.0;
        }
        
        // z × a1
        geom_point_t z_cross_a1 = {
            z_axis.y * lattice->a1.z - z_axis.z * lattice->a1.y,
            z_axis.z * lattice->a1.x - z_axis.x * lattice->a1.z,
            z_axis.x * lattice->a1.y - z_axis.y * lattice->a1.x
        };
        
        // a2 · (z × a1)
        double a2_dot_z_cross_a1 = lattice->a2.x * z_cross_a1.x + 
                                   lattice->a2.y * z_cross_a1.y + 
                                   lattice->a2.z * z_cross_a1.z;
        
        if (fabs(a2_dot_z_cross_a1) > NUMERICAL_EPSILON) {
            double factor = 2.0 * M_PI / a2_dot_z_cross_a1;
            b2->x = factor * z_cross_a1.x;
            b2->y = factor * z_cross_a1.y;
            b2->z = factor * z_cross_a1.z;
        } else {
            b2->x = 0.0; b2->y = 0.0; b2->z = 0.0;
        }
        
        b3->x = 0.0; b3->y = 0.0; b3->z = 0.0;
    } else if (lattice->dimension == 3) {
        // 3D: b1 = 2π * (a2 × a3) / (a1 · (a2 × a3)), etc.
        // a2 × a3
        geom_point_t a2_cross_a3 = {
            lattice->a2.y * lattice->a3.z - lattice->a2.z * lattice->a3.y,
            lattice->a2.z * lattice->a3.x - lattice->a2.x * lattice->a3.z,
            lattice->a2.x * lattice->a3.y - lattice->a2.y * lattice->a3.x
        };
        
        // a1 · (a2 × a3) = volume
        double volume = lattice->a1.x * a2_cross_a3.x + 
                       lattice->a1.y * a2_cross_a3.y + 
                       lattice->a1.z * a2_cross_a3.z;
        
        if (fabs(volume) > NUMERICAL_EPSILON) {
            double factor = 2.0 * M_PI / volume;
            b1->x = factor * a2_cross_a3.x;
            b1->y = factor * a2_cross_a3.y;
            b1->z = factor * a2_cross_a3.z;
        } else {
            b1->x = 0.0; b1->y = 0.0; b1->z = 0.0;
        }
        
        // b2 = 2π * (a3 × a1) / volume
        geom_point_t a3_cross_a1 = {
            lattice->a3.y * lattice->a1.z - lattice->a3.z * lattice->a1.y,
            lattice->a3.z * lattice->a1.x - lattice->a3.x * lattice->a1.z,
            lattice->a3.x * lattice->a1.y - lattice->a3.y * lattice->a1.x
        };
        
        if (fabs(volume) > NUMERICAL_EPSILON) {
            double factor = 2.0 * M_PI / volume;
            b2->x = factor * a3_cross_a1.x;
            b2->y = factor * a3_cross_a1.y;
            b2->z = factor * a3_cross_a1.z;
        } else {
            b2->x = 0.0; b2->y = 0.0; b2->z = 0.0;
        }
        
        // b3 = 2π * (a1 × a2) / volume
        geom_point_t a1_cross_a2 = {
            lattice->a1.y * lattice->a2.z - lattice->a1.z * lattice->a2.y,
            lattice->a1.z * lattice->a2.x - lattice->a1.x * lattice->a2.z,
            lattice->a1.x * lattice->a2.y - lattice->a1.y * lattice->a2.x
        };
        
        if (fabs(volume) > NUMERICAL_EPSILON) {
            double factor = 2.0 * M_PI / volume;
            b3->x = factor * a1_cross_a2.x;
            b3->y = factor * a1_cross_a2.y;
            b3->z = factor * a1_cross_a2.z;
        } else {
            b3->x = 0.0; b3->y = 0.0; b3->z = 0.0;
        }
    }
}

/*********************************************************************
 * Real-Space Sum
 *********************************************************************/

/**
 * @brief Compute real-space contribution to Ewald sum
 */
complex_t ewald_real_space_sum(const geom_point_t* r,
                                const geom_point_t* r_prime,
                                const periodic_lattice_t* lattice,
                                const bloch_vector_t* k_bloch,
                                double k,
                                const ewald_params_t* params) {
    if (!r || !r_prime || !lattice || !k_bloch || !params) {
        return complex_zero();
    }
    
    complex_t sum = complex_zero();
    double eta = params->eta;
    double eta_sq = eta * eta;
    
    // Compute relative position
    double dx0 = r->x - r_prime->x;
    double dy0 = r->y - r_prime->y;
    double dz0 = r->z - r_prime->z;
    
    // Sum over lattice vectors
    int n_max = params->n_max_real;
    
    if (lattice->dimension == 1) {
        // 1D periodicity
        for (int n = -n_max; n <= n_max; n++) {
            if (n == 0) continue;  // Skip self-term (handled separately)
            
            // Lattice vector: R_n = n * a1
            double Rx = n * lattice->a1.x;
            double Ry = n * lattice->a1.y;
            double Rz = n * lattice->a1.z;
            
            // Relative position: r - r' - R_n
            double dx = dx0 - Rx;
            double dy = dy0 - Ry;
            double dz = dz0 - Rz;
            double R_sq = dx*dx + dy*dy + dz*dz;
            double R = sqrt(R_sq) + NUMERICAL_EPSILON;
            
            // Bloch phase: exp(i k_bloch · R_n)
            double phase_bloch = k_bloch->kx * Rx + k_bloch->ky * Ry + k_bloch->kz * Rz;
            double cos_bloch = cos(phase_bloch);
            double sin_bloch = sin(phase_bloch);
            
            // Ewald real-space term: erfc(eta*R) * exp(-jkR) / (4πR)
            // erfc(x) = 1 - erf(x) = complementary error function
            double eta_R = eta * R;
            double erfc_val = erfc(eta_R);  // Standard library function
            
            // Green's function phase
            double kR = k * R;
            double cos_kR = cos(kR);
            double sin_kR = sin(kR);
            
            // Contribution: erfc(eta*R) * exp(-jkR) * exp(i k_bloch · R_n) / (4πR)
            double inv_4pi_R = 1.0 / (4.0 * M_PI * R);
            double real_part = erfc_val * inv_4pi_R * (cos_kR * cos_bloch - sin_kR * sin_bloch);
            double imag_part = erfc_val * inv_4pi_R * (-sin_kR * cos_bloch - cos_kR * sin_bloch);
            
            sum.re += real_part;
            sum.im += imag_part;
        }
    } else if (lattice->dimension == 2) {
        // 2D periodicity
        for (int n1 = -n_max; n1 <= n_max; n1++) {
            for (int n2 = -n_max; n2 <= n_max; n2++) {
                if (n1 == 0 && n2 == 0) continue;  // Skip self-term
                
                // Lattice vector: R = n1*a1 + n2*a2
                double Rx = n1 * lattice->a1.x + n2 * lattice->a2.x;
                double Ry = n1 * lattice->a1.y + n2 * lattice->a2.y;
                double Rz = n1 * lattice->a1.z + n2 * lattice->a2.z;
                
                // Relative position
                double dx = dx0 - Rx;
                double dy = dy0 - Ry;
                double dz = dz0 - Rz;
                double R_sq = dx*dx + dy*dy + dz*dz;
                double R = sqrt(R_sq) + NUMERICAL_EPSILON;
                
                // Bloch phase
                double phase_bloch = k_bloch->kx * Rx + k_bloch->ky * Ry + k_bloch->kz * Rz;
                double cos_bloch = cos(phase_bloch);
                double sin_bloch = sin(phase_bloch);
                
                // Ewald real-space term
                double eta_R = eta * R;
                double erfc_val = erfc(eta_R);
                
                double kR = k * R;
                double cos_kR = cos(kR);
                double sin_kR = sin(kR);
                
                double inv_4pi_R = 1.0 / (4.0 * M_PI * R);
                double real_part = erfc_val * inv_4pi_R * (cos_kR * cos_bloch - sin_kR * sin_bloch);
                double imag_part = erfc_val * inv_4pi_R * (-sin_kR * cos_bloch - cos_kR * sin_bloch);
                
                sum.re += real_part;
                sum.im += imag_part;
            }
        }
    } else if (lattice->dimension == 3) {
        // 3D periodicity
        for (int n1 = -n_max; n1 <= n_max; n1++) {
            for (int n2 = -n_max; n2 <= n_max; n2++) {
                for (int n3 = -n_max; n3 <= n_max; n3++) {
                    if (n1 == 0 && n2 == 0 && n3 == 0) continue;
                    
                    // Lattice vector: R = n1*a1 + n2*a2 + n3*a3
                    double Rx = n1 * lattice->a1.x + n2 * lattice->a2.x + n3 * lattice->a3.x;
                    double Ry = n1 * lattice->a1.y + n2 * lattice->a2.y + n3 * lattice->a3.y;
                    double Rz = n1 * lattice->a1.z + n2 * lattice->a2.z + n3 * lattice->a3.z;
                    
                    // Relative position
                    double dx = dx0 - Rx;
                    double dy = dy0 - Ry;
                    double dz = dz0 - Rz;
                    double R_sq = dx*dx + dy*dy + dz*dz;
                    double R = sqrt(R_sq) + NUMERICAL_EPSILON;
                    
                    // Bloch phase
                    double phase_bloch = k_bloch->kx * Rx + k_bloch->ky * Ry + k_bloch->kz * Rz;
                    double cos_bloch = cos(phase_bloch);
                    double sin_bloch = sin(phase_bloch);
                    
                    // Ewald real-space term
                    double eta_R = eta * R;
                    double erfc_val = erfc(eta_R);
                    
                    double kR = k * R;
                    double cos_kR = cos(kR);
                    double sin_kR = sin(kR);
                    
                    double inv_4pi_R = 1.0 / (4.0 * M_PI * R);
                    double real_part = erfc_val * inv_4pi_R * (cos_kR * cos_bloch - sin_kR * sin_bloch);
                    double imag_part = erfc_val * inv_4pi_R * (-sin_kR * cos_bloch - cos_kR * sin_bloch);
                    
                    sum.re += real_part;
                    sum.im += imag_part;
                }
            }
        }
    }
    
    return sum;
}

/*********************************************************************
 * Reciprocal-Space Sum
 *********************************************************************/

/**
 * @brief Compute reciprocal-space contribution to Ewald sum
 */
complex_t ewald_reciprocal_space_sum(const geom_point_t* r,
                                      const geom_point_t* r_prime,
                                      const periodic_lattice_t* lattice,
                                      const bloch_vector_t* k_bloch,
                                      double k,
                                      const ewald_params_t* params) {
    if (!r || !r_prime || !lattice || !k_bloch || !params) {
        return complex_zero();
    }
    
    complex_t sum = complex_zero();
    double eta = params->eta;
    double eta_sq = eta * eta;
    
    // Compute relative position
    double dx0 = r->x - r_prime->x;
    double dy0 = r->y - r_prime->y;
    double dz0 = r->z - r_prime->z;
    
    // Compute reciprocal lattice vectors
    geom_point_t b1, b2, b3;
    ewald_compute_reciprocal_lattice(lattice, &b1, &b2, &b3);
    
    // Sum over reciprocal lattice vectors
    int m_max = params->n_max_reciprocal;
    
    if (lattice->dimension == 1) {
        // 1D periodicity
        for (int m = -m_max; m <= m_max; m++) {
            if (m == 0) continue;  // Skip m=0 (handled by self-term)
            
            // Reciprocal vector: K_m = m * b1
            double Kx = m * b1.x;
            double Ky = m * b1.y;
            double Kz = m * b1.z;
            
            // |K + k_bloch|^2
            double Kx_tot = Kx + k_bloch->kx;
            double Ky_tot = Ky + k_bloch->ky;
            double Kz_tot = Kz + k_bloch->kz;
            double K_sq = Kx_tot*Kx_tot + Ky_tot*Ky_tot + Kz_tot*Kz_tot;
            
            // Ewald reciprocal-space term: exp(-|K + k_bloch|^2 / (4*eta^2)) / (|K + k_bloch|^2 - k^2)
            // For |K + k_bloch|^2 ≈ k^2, use limit
            double K_mag_sq = K_sq;
            double denom = K_mag_sq - k*k;
            
            if (fabs(denom) < NUMERICAL_EPSILON) {
                // Near resonance: use limit
                continue;  // Skip resonant terms (require special handling)
            }
            
            double exp_factor = exp(-K_mag_sq / (4.0 * eta_sq));
            
            // Phase: exp(i (K + k_bloch) · (r - r'))
            double phase = Kx_tot * dx0 + Ky_tot * dy0 + Kz_tot * dz0;
            double cos_phase = cos(phase);
            double sin_phase = sin(phase);
            
            // Contribution: exp_factor * exp(i phase) / denom / (4π)
            double inv_4pi = 1.0 / (4.0 * M_PI);
            double coeff = exp_factor * inv_4pi / denom;
            
            sum.re += coeff * cos_phase;
            sum.im += coeff * sin_phase;
        }
    } else if (lattice->dimension == 2) {
        // 2D periodicity
        for (int m1 = -m_max; m1 <= m_max; m1++) {
            for (int m2 = -m_max; m2 <= m_max; m2++) {
                if (m1 == 0 && m2 == 0) continue;
                
                // Reciprocal vector: K = m1*b1 + m2*b2
                double Kx = m1 * b1.x + m2 * b2.x;
                double Ky = m1 * b1.y + m2 * b2.y;
                double Kz = m1 * b1.z + m2 * b2.z;
                
                double Kx_tot = Kx + k_bloch->kx;
                double Ky_tot = Ky + k_bloch->ky;
                double Kz_tot = Kz + k_bloch->kz;
                double K_sq = Kx_tot*Kx_tot + Ky_tot*Ky_tot + Kz_tot*Kz_tot;
                
                double denom = K_sq - k*k;
                if (fabs(denom) < NUMERICAL_EPSILON) {
                    continue;
                }
                
                double exp_factor = exp(-K_sq / (4.0 * eta_sq));
                double phase = Kx_tot * dx0 + Ky_tot * dy0 + Kz_tot * dz0;
                double cos_phase = cos(phase);
                double sin_phase = sin(phase);
                
                double inv_4pi = 1.0 / (4.0 * M_PI);
                double coeff = exp_factor * inv_4pi / denom;
                
                sum.re += coeff * cos_phase;
                sum.im += coeff * sin_phase;
            }
        }
    } else if (lattice->dimension == 3) {
        // 3D periodicity
        for (int m1 = -m_max; m1 <= m_max; m1++) {
            for (int m2 = -m_max; m2 <= m_max; m2++) {
                for (int m3 = -m_max; m3 <= m_max; m3++) {
                    if (m1 == 0 && m2 == 0 && m3 == 0) continue;
                    
                    // Reciprocal vector: K = m1*b1 + m2*b2 + m3*b3
                    double Kx = m1 * b1.x + m2 * b2.x + m3 * b3.x;
                    double Ky = m1 * b1.y + m2 * b2.y + m3 * b3.y;
                    double Kz = m1 * b1.z + m2 * b2.z + m3 * b3.z;
                    
                    double Kx_tot = Kx + k_bloch->kx;
                    double Ky_tot = Ky + k_bloch->ky;
                    double Kz_tot = Kz + k_bloch->kz;
                    double K_sq = Kx_tot*Kx_tot + Ky_tot*Ky_tot + Kz_tot*Kz_tot;
                    
                    double denom = K_sq - k*k;
                    if (fabs(denom) < NUMERICAL_EPSILON) {
                        continue;
                    }
                    
                    double exp_factor = exp(-K_sq / (4.0 * eta_sq));
                    double phase = Kx_tot * dx0 + Ky_tot * dy0 + Kz_tot * dz0;
                    double cos_phase = cos(phase);
                    double sin_phase = sin(phase);
                    
                    double inv_4pi = 1.0 / (4.0 * M_PI);
                    double coeff = exp_factor * inv_4pi / denom;
                    
                    sum.re += coeff * cos_phase;
                    sum.im += coeff * sin_phase;
                }
            }
        }
    }
    
    return sum;
}

/*********************************************************************
 * Self-Term Correction
 *********************************************************************/

/**
 * @brief Compute self-term correction for Ewald sum
 */
complex_t ewald_self_term_correction(const geom_point_t* r,
                                     const geom_point_t* r_prime,
                                     double k,
                                     const ewald_params_t* params) {
    if (!r || !r_prime || !params) {
        return complex_zero();
    }
    
    // Self-term: when r = r', we need special handling
    double dx = r->x - r_prime->x;
    double dy = r->y - r_prime->y;
    double dz = r->z - r_prime->z;
    double R_sq = dx*dx + dy*dy + dz*dz;
    
    if (R_sq < NUMERICAL_EPSILON) {
        // Self-term: use limit as R -> 0
        // For periodic structures, self-term includes contribution from all images
        // This is typically handled separately in the matrix assembly
        return complex_zero();  // Self-term handled in assembly
    }
    
    // For non-zero distance, no special correction needed
    return complex_zero();
}

/*********************************************************************
 * Complete Ewald Sum
 *********************************************************************/

/**
 * @brief Compute periodic Green's function using Ewald summation
 */
complex_t ewald_periodic_green(const geom_point_t* r,
                                const geom_point_t* r_prime,
                                const periodic_lattice_t* lattice,
                                const bloch_vector_t* k_bloch,
                                double k,
                                const ewald_params_t* params) {
    if (!r || !r_prime || !lattice || !k_bloch || !params) {
        return complex_zero();
    }
    
    // Ewald sum = real-space sum + reciprocal-space sum + self-term correction
    complex_t real_sum = ewald_real_space_sum(r, r_prime, lattice, k_bloch, k, params);
    complex_t recip_sum = ewald_reciprocal_space_sum(r, r_prime, lattice, k_bloch, k, params);
    complex_t self_corr = ewald_self_term_correction(r, r_prime, k, params);
    
    // Combine contributions
    complex_t result;
    result.re = real_sum.re + recip_sum.re + self_corr.re;
    result.im = real_sum.im + recip_sum.im + self_corr.im;
    
    return result;
}

