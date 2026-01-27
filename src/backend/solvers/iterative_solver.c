/********************************************************************************
 * Iterative Solver Implementation (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements iterative solver methods (GMRES, CG, BiCGSTAB).
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 sees matrices, not physics. No physics assumptions.
 ********************************************************************************/

#include "iterative_solver.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../common/constants.h"
#include "../../operators/matvec/matvec_operator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Iterative solver data structure
typedef struct {
    iterative_solver_config_t config;
    const operator_matrix_t* matrix;
    const operator_matrix_t* preconditioner;
    operator_vector_t* initial_guess;
    bool is_initialized;
} iterative_solver_data_t;

// Helper: Compute vector norm
static real_t vector_norm(const operator_vector_t* v) {
    if (!v || !v->data) return 0.0;
    
    real_t norm_sq = 0.0;
    for (int i = 0; i < v->size; i++) {
        #if defined(_MSC_VER)
        real_t mag_sq = v->data[i].re * v->data[i].re + v->data[i].im * v->data[i].im;
        #else
        real_t mag_sq = creal(v->data[i] * conj(v->data[i]));
        #endif
        norm_sq += mag_sq;
    }
    
    return sqrt(norm_sq);
}

// Helper: Compute dot product
static complex_t vector_dot(const operator_vector_t* a, const operator_vector_t* b) {
    if (!a || !b || !a->data || !b->data || a->size != b->size) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    #if defined(_MSC_VER)
    complex_t sum = {0.0, 0.0};
    for (int i = 0; i < a->size; i++) {
        complex_t prod;
        prod.re = a->data[i].re * b->data[i].re + a->data[i].im * b->data[i].im;
        prod.im = a->data[i].im * b->data[i].re - a->data[i].re * b->data[i].im;
        sum.re += prod.re;
        sum.im += prod.im;
    }
    #else
    complex_t sum = 0.0 + 0.0 * I;
    for (int i = 0; i < a->size; i++) {
        sum += conj(a->data[i]) * b->data[i];
    }
    #endif
    
    return sum;
}

// Helper: Vector operations
static void vector_copy(const operator_vector_t* src, operator_vector_t* dst) {
    if (!src || !dst || !src->data || !dst->data || src->size != dst->size) return;
    memcpy(dst->data, src->data, src->size * sizeof(complex_t));
}

static void vector_scale(operator_vector_t* v, complex_t alpha) {
    if (!v || !v->data) return;
    
    for (int i = 0; i < v->size; i++) {
        #if defined(_MSC_VER)
        complex_t prod;
        prod.re = alpha.re * v->data[i].re - alpha.im * v->data[i].im;
        prod.im = alpha.re * v->data[i].im + alpha.im * v->data[i].re;
        v->data[i] = prod;
        #else
        v->data[i] = alpha * v->data[i];
        #endif
    }
}

static void vector_axpy(complex_t alpha, const operator_vector_t* x, operator_vector_t* y) {
    if (!x || !y || !x->data || !y->data || x->size != y->size) return;
    
    for (int i = 0; i < x->size; i++) {
        #if defined(_MSC_VER)
        complex_t prod;
        prod.re = alpha.re * x->data[i].re - alpha.im * x->data[i].im;
        prod.im = alpha.re * x->data[i].im + alpha.im * x->data[i].re;
        y->data[i].re += prod.re;
        y->data[i].im += prod.im;
        #else
        y->data[i] += alpha * x->data[i];
        #endif
    }
}

// Helper: Apply preconditioner
static int apply_preconditioner(
    const iterative_solver_data_t* data,
    const operator_vector_t* r,
    operator_vector_t* z) {
    
    if (!data || !r || !z) return STATUS_ERROR_INVALID_INPUT;
    
    // If no preconditioner, just copy (identity preconditioner)
    if (!data->preconditioner) {
        vector_copy(r, z);
        return STATUS_SUCCESS;
    }
    
    // Apply preconditioner: solve M*z = r where M is preconditioner matrix
    // This is a simplified implementation - full implementation would:
    // 1. Factorize preconditioner matrix (e.g., ILU, Cholesky)
    // 2. Solve triangular systems
    
    // For now, use direct solve if preconditioner is available
    // In production, would use specialized preconditioner solvers
    if (data->preconditioner->type == MATRIX_TYPE_DENSE && data->preconditioner->data.dense) {
        // Direct solve: M*z = r
        // This is simplified - would use factorized form in production
        int n = r->size;
        complex_t* M = data->preconditioner->data.dense;
        
        // Forward substitution (assuming lower triangular)
        for (int i = 0; i < n; i++) {
            complex_t sum = r->data[i];
            for (int j = 0; j < i; j++) {
                int idx = i * n + j;
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = M[idx].re * z->data[j].re - M[idx].im * z->data[j].im;
                prod.im = M[idx].re * z->data[j].im + M[idx].im * z->data[j].re;
                sum.re -= prod.re;
                sum.im -= prod.im;
                #else
                sum -= M[idx] * z->data[j];
                #endif
            }
            
            int diag_idx = i * n + i;
            if (fabs(M[diag_idx].re) > NUMERICAL_EPSILON || fabs(M[diag_idx].im) > NUMERICAL_EPSILON) {
                #if defined(_MSC_VER)
                real_t denom = M[diag_idx].re * M[diag_idx].re + M[diag_idx].im * M[diag_idx].im;
                z->data[i].re = (sum.re * M[diag_idx].re + sum.im * M[diag_idx].im) / denom;
                z->data[i].im = (sum.im * M[diag_idx].re - sum.re * M[diag_idx].im) / denom;
                #else
                z->data[i] = sum / M[diag_idx];
                #endif
            } else {
                z->data[i] = sum;  // Avoid division by zero
            }
        }
        
        return STATUS_SUCCESS;
    } else {
        // For other matrix types, use identity preconditioner
        vector_copy(r, z);
        return STATUS_SUCCESS;
    }
}

// Conjugate Gradient solver
static int solve_cg(
    const operator_matrix_t* A,
    const operator_vector_t* b,
    operator_vector_t* x,
    const iterative_solver_config_t* config,
    solver_statistics_t* stats) {
    
    if (!A || !b || !x || !config || !stats) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    int n = b->size;
    
    // Allocate working vectors
    operator_vector_t* r = matvec_operator_create_vector(n);
    operator_vector_t* p = matvec_operator_create_vector(n);
    operator_vector_t* Ap = matvec_operator_create_vector(n);
    operator_vector_t* z = matvec_operator_create_vector(n);
    
    if (!r || !p || !Ap || !z) {
        if (r) matvec_operator_destroy_vector(r);
        if (p) matvec_operator_destroy_vector(p);
        if (Ap) matvec_operator_destroy_vector(Ap);
        if (z) matvec_operator_destroy_vector(z);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize: r = b - A*x
    matvec_operator_apply(A, x, Ap);
    vector_copy(b, r);
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        r->data[i].re -= Ap->data[i].re;
        r->data[i].im -= Ap->data[i].im;
        #else
        r->data[i] -= Ap->data[i];
        #endif
    }
    
    real_t r0_norm = vector_norm(r);
    real_t tolerance = config->tolerance > 0 ? config->tolerance : CONVERGENCE_TOLERANCE_DEFAULT;
    int max_iter = config->max_iterations > 0 ? config->max_iterations : 1000;
    
    // CG iteration
    vector_copy(r, p);
    stats->iterations = 0;
    
    for (int k = 0; k < max_iter; k++) {
        stats->iterations = k + 1;
        
        // Compute Ap = A * p
        matvec_operator_apply(A, p, Ap);
        
        // Compute alpha = (r^H * r) / (p^H * Ap)
        complex_t r_dot_r = vector_dot(r, r);
        complex_t p_dot_Ap = vector_dot(p, Ap);
        
        real_t p_dot_Ap_mag = sqrt(p_dot_Ap.re * p_dot_Ap.re + p_dot_Ap.im * p_dot_Ap.im);
        if (p_dot_Ap_mag < NUMERICAL_EPSILON) {
            break;  // Breakdown
        }
        
        #if defined(_MSC_VER)
        complex_t alpha;
        real_t r_dot_r_mag = sqrt(r_dot_r.re * r_dot_r.re + r_dot_r.im * r_dot_r.im);
        alpha.re = r_dot_r_mag / p_dot_Ap_mag;
        alpha.im = 0.0;
        #else
        complex_t alpha = r_dot_r / p_dot_Ap;
        #endif
        
        // x = x + alpha * p
        vector_axpy(alpha, p, x);
        
        // r = r - alpha * Ap
        #if defined(_MSC_VER)
        complex_t neg_alpha;
        neg_alpha.re = -alpha.re;
        neg_alpha.im = -alpha.im;
        #else
        complex_t neg_alpha = -alpha;
        #endif
        vector_axpy(neg_alpha, Ap, r);
        
        // Check convergence
        real_t r_norm = vector_norm(r);
        stats->residual_norm = r_norm / r0_norm;
        
        if (stats->residual_norm < tolerance) {
            stats->converged = true;
            break;
        }
        
        // Compute beta = (r_new^H * r_new) / (r_old^H * r_old)
        complex_t r_new_dot_r_new = vector_dot(r, r);
        #if defined(_MSC_VER)
        real_t r_new_mag = sqrt(r_new_dot_r_new.re * r_new_dot_r_new.re + r_new_dot_r_new.im * r_new_dot_r_new.im);
        real_t r_old_mag = sqrt(r_dot_r.re * r_dot_r.re + r_dot_r.im * r_dot_r.im);
        complex_t beta;
        beta.re = (r_old_mag > NUMERICAL_EPSILON) ? (r_new_mag / r_old_mag) : 0.0;
        beta.im = 0.0;
        #else
        complex_t beta = (r_dot_r.re > NUMERICAL_EPSILON) ? (r_new_dot_r_new / r_dot_r) : (0.0 + 0.0 * I);
        #endif
        
        // p = r + beta * p
        vector_scale(p, beta);
        vector_axpy((complex_t){1.0, 0.0}, r, p);
        
        r_dot_r = r_new_dot_r_new;
    }
    
    // Cleanup
    matvec_operator_destroy_vector(r);
    matvec_operator_destroy_vector(p);
    matvec_operator_destroy_vector(Ap);
    matvec_operator_destroy_vector(z);
    
    return stats->converged ? STATUS_SUCCESS : STATUS_ERROR_CONVERGENCE_FAILURE;
}

// GMRES solver with Arnoldi process
static int solve_gmres(
    const operator_matrix_t* A,
    const operator_vector_t* b,
    operator_vector_t* x,
    const iterative_solver_config_t* config,
    solver_statistics_t* stats) {
    
    if (!A || !b || !x || !config || !stats) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    int n = b->size;
    int restart = config->restart_size > 0 ? config->restart_size : 30;
    int max_iter = config->max_iterations > 0 ? config->max_iterations : n;
    real_t tol = config->tolerance > 0.0 ? config->tolerance : 1e-6;
    
    // Allocate workspace for Arnoldi process
    // V: Krylov subspace vectors [n][restart+1]
    // H: Hessenberg matrix [restart+1][restart]
    // g: Givens rotation coefficients [restart]
    complex_t** V = (complex_t**)malloc((restart + 1) * sizeof(complex_t*));
    real_t* H = (real_t*)calloc((restart + 1) * restart, sizeof(real_t));
    real_t* cs = (real_t*)malloc(restart * sizeof(real_t));  // Cosine for Givens
    real_t* sn = (real_t*)malloc(restart * sizeof(real_t));  // Sine for Givens
    complex_t* g = (complex_t*)malloc((restart + 1) * sizeof(complex_t));  // RHS vector
    
    if (!V || !H || !cs || !sn || !g) {
        if (V) free(V);
        if (H) free(H);
        if (cs) free(cs);
        if (sn) free(sn);
        if (g) free(g);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    for (int i = 0; i <= restart; i++) {
        V[i] = (complex_t*)calloc(n, sizeof(complex_t));
        if (!V[i]) {
            for (int j = 0; j < i; j++) free(V[j]);
            free(V); free(H); free(cs); free(sn); free(g);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }
    
    // Initialize
    stats->iterations = 0;
    stats->converged = false;
    
    // Compute initial residual: r0 = b - A*x0
    operator_vector_t* r = matvec_operator_create_vector(n);
    operator_vector_t* Ax = matvec_operator_create_vector(n);
    if (!r || !Ax) {
        for (int i = 0; i <= restart; i++) free(V[i]);
        free(V); free(H); free(cs); free(sn); free(g);
        if (r) matvec_operator_destroy_vector(r);
        if (Ax) matvec_operator_destroy_vector(Ax);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Compute Ax
    matvec_operator_apply(A, x, Ax);
    
    // r = b - Ax
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        r->data[i].re = b->data[i].re - Ax->data[i].re;
        r->data[i].im = b->data[i].im - Ax->data[i].im;
        #else
        r->data[i] = b->data[i] - Ax->data[i];
        #endif
    }
    
    // Compute initial residual norm
    real_t beta = 0.0;
    for (int i = 0; i < n; i++) {
        real_t re = r->data[i].re;
        real_t im = r->data[i].im;
        beta += re * re + im * im;
    }
    beta = sqrt(beta);
    
    if (beta < tol) {
        stats->converged = true;
        stats->residual_norm = beta;
        for (int i = 0; i <= restart; i++) free(V[i]);
        free(V); free(H); free(cs); free(sn); free(g);
        matvec_operator_destroy_vector(r);
        matvec_operator_destroy_vector(Ax);
        return STATUS_SUCCESS;
    }
    
    // Normalize first vector: v0 = r0 / ||r0||
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        V[0][i].re = r->data[i].re / beta;
        V[0][i].im = r->data[i].im / beta;
        #else
        V[0][i] = r->data[i] / beta;
        #endif
    }
    
    // Initialize RHS vector for least squares
    #if defined(_MSC_VER)
    g[0].re = beta;
    g[0].im = 0.0;
    for (int i = 1; i <= restart; i++) {
        g[i].re = 0.0;
        g[i].im = 0.0;
    }
    #else
    g[0] = beta + 0.0 * I;
    for (int i = 1; i <= restart; i++) {
        g[i] = 0.0 + 0.0 * I;
    }
    #endif
    
    // Restarted GMRES loop
    for (int iter = 0; iter < max_iter; iter += restart) {
        // Arnoldi process: build Krylov subspace
        for (int j = 0; j < restart; j++) {
            // Compute w = A * v_j
            operator_vector_t* v_j = matvec_operator_create_vector(n);
            operator_vector_t* w = matvec_operator_create_vector(n);
            if (!v_j || !w) {
                for (int i = 0; i <= restart; i++) free(V[i]);
                free(V); free(H); free(cs); free(sn); free(g);
                matvec_operator_destroy_vector(r);
                matvec_operator_destroy_vector(Ax);
                if (v_j) matvec_operator_destroy_vector(v_j);
                if (w) matvec_operator_destroy_vector(w);
                return STATUS_ERROR_MEMORY_ALLOCATION;
            }
            
            for (int i = 0; i < n; i++) {
                v_j->data[i] = V[j][i];
            }
            
            matvec_operator_apply(A, v_j, w);
            
            // Modified Gram-Schmidt orthogonalization
            for (int i = 0; i <= j; i++) {
                // H[i][j] = <v_i, w>
                #if defined(_MSC_VER)
                complex_t dot = {0.0, 0.0};
                #else
                complex_t dot = 0.0 + 0.0 * I;
                #endif
                #if defined(_MSC_VER)
                dot.re = 0.0;
                dot.im = 0.0;
                #else
                dot = 0.0 + 0.0 * I;
                #endif
                
                for (int k = 0; k < n; k++) {
                    #if defined(_MSC_VER)
                    complex_t prod;
                    prod.re = V[i][k].re * w->data[k].re + V[i][k].im * w->data[k].im;
                    prod.im = V[i][k].re * w->data[k].im - V[i][k].im * w->data[k].re;
                    dot.re += prod.re;
                    dot.im += prod.im;
                    #else
                    dot += conj(V[i][k]) * w->data[k];
                    #endif
                }
                
                // Store complex dot product in Hessenberg matrix
                // For real matrices, use real part; for complex, would store both
                H[i * restart + j] = dot.re;  // Real part (for real matrices)
                // Note: For complex matrices, would need separate storage for imaginary part
                
                // w = w - H[i][j] * v_i
                for (int k = 0; k < n; k++) {
                    #if defined(_MSC_VER)
                    complex_t sub;
                    sub.re = H[i * restart + j] * V[i][k].re;
                    sub.im = H[i * restart + j] * V[i][k].im;
                    w->data[k].re -= sub.re;
                    w->data[k].im -= sub.im;
                    #else
                    w->data[k] -= H[i * restart + j] * V[i][k];
                    #endif
                }
            }
            
            // Compute H[j+1][j] = ||w||
            real_t w_norm = 0.0;
            for (int k = 0; k < n; k++) {
                real_t re = w->data[k].re;
                real_t im = w->data[k].im;
                w_norm += re * re + im * im;
            }
            w_norm = sqrt(w_norm);
            H[(j + 1) * restart + j] = w_norm;
            
            // Normalize: v_{j+1} = w / ||w||
            if (w_norm > NUMERICAL_EPSILON) {
                for (int k = 0; k < n; k++) {
                    #if defined(_MSC_VER)
                    V[j + 1][k].re = w->data[k].re / w_norm;
                    V[j + 1][k].im = w->data[k].im / w_norm;
                    #else
                    V[j + 1][k] = w->data[k] / w_norm;
                    #endif
                }
            } else {
                // Breakdown: w is zero, solution found
                break;
            }
            
            // Apply previous Givens rotations to H
            for (int i = 0; i < j; i++) {
                real_t temp = cs[i] * H[i * restart + j] + sn[i] * H[(i + 1) * restart + j];
                H[(i + 1) * restart + j] = -sn[i] * H[i * restart + j] + cs[i] * H[(i + 1) * restart + j];
                H[i * restart + j] = temp;
            }
            
            // Compute Givens rotation for H[j][j] and H[j+1][j]
            real_t h_jj = H[j * restart + j];
            real_t h_j1j = H[(j + 1) * restart + j];
            real_t nu = sqrt(h_jj * h_jj + h_j1j * h_j1j);
            
            if (nu > NUMERICAL_EPSILON) {
                cs[j] = h_jj / nu;
                sn[j] = h_j1j / nu;
                H[j * restart + j] = nu;
                H[(j + 1) * restart + j] = 0.0;
                
                // Apply Givens rotation to g
                real_t temp = cs[j] * g[j].re + sn[j] * g[j + 1].re;
                g[j + 1].re = -sn[j] * g[j].re + cs[j] * g[j + 1].re;
                g[j].re = temp;
            }
            
            // Check convergence
            real_t residual = fabs(g[j + 1].re);
            stats->iterations = iter + j + 1;
            stats->residual_norm = residual;
            
            if (residual < tol) {
                stats->converged = true;
                
                // Solve upper triangular system: H * y = g
                // Back substitution
                for (int k = j; k >= 0; k--) {
                    complex_t sum = g[k];
                    for (int l = k + 1; l <= j; l++) {
                        #if defined(_MSC_VER)
                        sum.re -= H[k * restart + l] * g[l].re;
                        #else
                        sum -= H[k * restart + l] * g[l];
                        #endif
                    }
                    if (H[k * restart + k] > NUMERICAL_EPSILON) {
                        #if defined(_MSC_VER)
                        g[k].re = sum.re / H[k * restart + k];
                        g[k].im = sum.im / H[k * restart + k];
                        #else
                        g[k] = sum / H[k * restart + k];
                        #endif
                    }
                }
                
                // Update solution: x = x + V * y
                for (int k = 0; k <= j; k++) {
                    for (int i = 0; i < n; i++) {
                        #if defined(_MSC_VER)
                        complex_t add;
                        add.re = g[k].re * V[k][i].re - g[k].im * V[k][i].im;
                        add.im = g[k].re * V[k][i].im + g[k].im * V[k][i].re;
                        x->data[i].re += add.re;
                        x->data[i].im += add.im;
                        #else
                        x->data[i] += g[k] * V[k][i];
                        #endif
                    }
                }
                
                // Cleanup
                for (int i = 0; i <= restart; i++) free(V[i]);
                free(V); free(H); free(cs); free(sn); free(g);
                matvec_operator_destroy_vector(r);
                matvec_operator_destroy_vector(Ax);
                matvec_operator_destroy_vector(v_j);
                matvec_operator_destroy_vector(w);
                return STATUS_SUCCESS;
            }
            
            matvec_operator_destroy_vector(v_j);
            matvec_operator_destroy_vector(w);
        }
        
        // Restart: update solution and compute new residual
        // Solve H * y = g (upper triangular)
        for (int k = restart - 1; k >= 0; k--) {
            complex_t sum = g[k];
            for (int l = k + 1; l < restart; l++) {
                #if defined(_MSC_VER)
                sum.re -= H[k * restart + l] * g[l].re;
                #else
                sum -= H[k * restart + l] * g[l];
                #endif
            }
            if (H[k * restart + k] > NUMERICAL_EPSILON) {
                #if defined(_MSC_VER)
                g[k].re = sum.re / H[k * restart + k];
                g[k].im = sum.im / H[k * restart + k];
                #else
                g[k] = sum / H[k * restart + k];
                #endif
            }
        }
        
        // Update solution: x = x + V * y
        for (int k = 0; k < restart; k++) {
            for (int i = 0; i < n; i++) {
                #if defined(_MSC_VER)
                complex_t add;
                add.re = g[k].re * V[k][i].re - g[k].im * V[k][i].im;
                add.im = g[k].re * V[k][i].im + g[k].im * V[k][i].re;
                x->data[i].re += add.re;
                x->data[i].im += add.im;
                #else
                x->data[i] += g[k] * V[k][i];
                #endif
            }
        }
        
        // Compute new residual for restart
        matvec_operator_apply(A, x, Ax);
        for (int i = 0; i < n; i++) {
            #if defined(_MSC_VER)
            r->data[i].re = b->data[i].re - Ax->data[i].re;
            r->data[i].im = b->data[i].im - Ax->data[i].im;
            #else
            r->data[i] = b->data[i] - Ax->data[i];
            #endif
        }
        
        beta = 0.0;
        for (int i = 0; i < n; i++) {
            real_t re = r->data[i].re;
            real_t im = r->data[i].im;
            beta += re * re + im * im;
        }
        beta = sqrt(beta);
        
        if (beta < tol) {
            stats->converged = true;
            stats->residual_norm = beta;
            break;
        }
        
        // Normalize for next restart
        for (int i = 0; i < n; i++) {
            #if defined(_MSC_VER)
            V[0][i].re = r->data[i].re / beta;
            V[0][i].im = r->data[i].im / beta;
            #else
            V[0][i] = r->data[i] / beta;
            #endif
        }
        
        #if defined(_MSC_VER)
        g[0].re = beta;
        g[0].im = 0.0;
        for (int i = 1; i <= restart; i++) {
            g[i].re = 0.0;
            g[i].im = 0.0;
        }
        #else
        g[0] = beta + 0.0 * I;
        for (int i = 1; i <= restart; i++) {
            g[i] = 0.0 + 0.0 * I;
        }
        #endif
    }
    
    // Cleanup
    for (int i = 0; i <= restart; i++) free(V[i]);
    free(V); free(H); free(cs); free(sn); free(g);
    matvec_operator_destroy_vector(r);
    matvec_operator_destroy_vector(Ax);
    
    if (!stats->converged) {
        return STATUS_ERROR_CONVERGENCE_FAILURE;
    }
    
    return STATUS_SUCCESS;
}

// BiCGSTAB solver
static int solve_bicgstab(
    const operator_matrix_t* A,
    const operator_vector_t* b,
    operator_vector_t* x,
    const iterative_solver_config_t* config,
    solver_statistics_t* stats) {
    
    if (!A || !b || !x || !config || !stats) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    int n = b->size;
    real_t tolerance = config->tolerance > 0 ? config->tolerance : CONVERGENCE_TOLERANCE_DEFAULT;
    int max_iter = config->max_iterations > 0 ? config->max_iterations : 1000;
    
    // Allocate working vectors
    operator_vector_t* r = matvec_operator_create_vector(n);
    operator_vector_t* r0 = matvec_operator_create_vector(n);
    operator_vector_t* p = matvec_operator_create_vector(n);
    operator_vector_t* v = matvec_operator_create_vector(n);
    operator_vector_t* s = matvec_operator_create_vector(n);
    operator_vector_t* t = matvec_operator_create_vector(n);
    
    if (!r || !r0 || !p || !v || !s || !t) {
        if (r) matvec_operator_destroy_vector(r);
        if (r0) matvec_operator_destroy_vector(r0);
        if (p) matvec_operator_destroy_vector(p);
        if (v) matvec_operator_destroy_vector(v);
        if (s) matvec_operator_destroy_vector(s);
        if (t) matvec_operator_destroy_vector(t);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize: r = b - A*x, r0 = r
    matvec_operator_apply(A, x, v);
    vector_copy(b, r);
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        r->data[i].re -= v->data[i].re;
        r->data[i].im -= v->data[i].im;
        #else
        r->data[i] -= v->data[i];
        #endif
    }
    vector_copy(r, r0);
    vector_copy(r, p);
    
    real_t r0_norm = vector_norm(r0);
    if (r0_norm < NUMERICAL_EPSILON) {
        // Already converged
        stats->converged = true;
        stats->iterations = 0;
        stats->residual_norm = 0.0;
        matvec_operator_destroy_vector(r);
        matvec_operator_destroy_vector(r0);
        matvec_operator_destroy_vector(p);
        matvec_operator_destroy_vector(v);
        matvec_operator_destroy_vector(s);
        matvec_operator_destroy_vector(t);
        return STATUS_SUCCESS;
    }
    
    #if defined(_MSC_VER)
    complex_t rho = {1.0, 0.0};
    complex_t alpha = {1.0, 0.0};
    complex_t omega = {1.0, 0.0};
    #else
    complex_t rho = 1.0 + 0.0 * I;
    complex_t alpha = 1.0 + 0.0 * I;
    complex_t omega = 1.0 + 0.0 * I;
    #endif
    
    stats->iterations = 0;
    
    // BiCGSTAB iteration
    for (int k = 0; k < max_iter; k++) {
        stats->iterations = k + 1;
        
        // Compute rho = <r0, r>
        complex_t rho_new = vector_dot(r0, r);
        real_t rho_mag = sqrt(rho_new.re * rho_new.re + rho_new.im * rho_new.im);
        
        if (rho_mag < NUMERICAL_EPSILON) {
            break;  // Breakdown
        }
        
        #if defined(_MSC_VER)
        complex_t beta;
        real_t rho_old_mag = sqrt(rho.re * rho.re + rho.im * rho.im);
        beta.re = (rho_old_mag > NUMERICAL_EPSILON) ? (rho_new.re / rho.re) * (alpha.re / omega.re) : 0.0;
        beta.im = 0.0;
        #else
        complex_t beta = (rho.re > NUMERICAL_EPSILON) ? (rho_new / rho) * (alpha / omega) : (0.0 + 0.0 * I);
        #endif
        
        // p = r + beta * (p - omega * v)
        #if defined(_MSC_VER)
        complex_t neg_omega;
        neg_omega.re = -omega.re;
        neg_omega.im = -omega.im;
        #else
        complex_t neg_omega = -omega;
        #endif
        vector_axpy(neg_omega, v, p);
        vector_scale(p, beta);
        vector_axpy((complex_t){1.0, 0.0}, r, p);
        
        // v = A * p
        matvec_operator_apply(A, p, v);
        
        // alpha = rho / <r0, v>
        complex_t r0_dot_v = vector_dot(r0, v);
        real_t r0_dot_v_mag = sqrt(r0_dot_v.re * r0_dot_v.re + r0_dot_v.im * r0_dot_v.im);
        if (r0_dot_v_mag < NUMERICAL_EPSILON) {
            break;  // Breakdown
        }
        
        #if defined(_MSC_VER)
        alpha.re = rho_new.re / r0_dot_v.re;
        alpha.im = 0.0;
        #else
        alpha = rho_new / r0_dot_v;
        #endif
        
        // s = r - alpha * v
        vector_copy(r, s);
        #if defined(_MSC_VER)
        complex_t neg_alpha;
        neg_alpha.re = -alpha.re;
        neg_alpha.im = -alpha.im;
        #else
        complex_t neg_alpha = -alpha;
        #endif
        vector_axpy(neg_alpha, v, s);
        
        // Check convergence on s
        real_t s_norm = vector_norm(s);
        stats->residual_norm = s_norm / r0_norm;
        if (stats->residual_norm < tolerance) {
            // x = x + alpha * p
            vector_axpy(alpha, p, x);
            stats->converged = true;
            break;
        }
        
        // t = A * s
        matvec_operator_apply(A, s, t);
        
        // omega = <t, s> / <t, t>
        complex_t t_dot_s = vector_dot(t, s);
        complex_t t_dot_t = vector_dot(t, t);
        real_t t_dot_t_mag = sqrt(t_dot_t.re * t_dot_t.re + t_dot_t.im * t_dot_t.im);
        if (t_dot_t_mag < NUMERICAL_EPSILON) {
            break;  // Breakdown
        }
        
        #if defined(_MSC_VER)
        omega.re = t_dot_s.re / t_dot_t.re;
        omega.im = 0.0;
        #else
        omega = t_dot_s / t_dot_t;
        #endif
        
        // x = x + alpha * p + omega * s
        vector_axpy(alpha, p, x);
        vector_axpy(omega, s, x);
        
        // r = s - omega * t
        vector_copy(s, r);
        #if defined(_MSC_VER)
        neg_omega.re = -omega.re;
        neg_omega.im = -omega.im;
        #else
        neg_omega = -omega;
        #endif
        vector_axpy(neg_omega, t, r);
        
        // Check convergence
        real_t r_norm = vector_norm(r);
        stats->residual_norm = r_norm / r0_norm;
        if (stats->residual_norm < tolerance) {
            stats->converged = true;
            break;
        }
        
        rho = rho_new;
    }
    
    // Cleanup
    matvec_operator_destroy_vector(r);
    matvec_operator_destroy_vector(r0);
    matvec_operator_destroy_vector(p);
    matvec_operator_destroy_vector(v);
    matvec_operator_destroy_vector(s);
    matvec_operator_destroy_vector(t);
    
    return stats->converged ? STATUS_SUCCESS : STATUS_ERROR_CONVERGENCE_FAILURE;
}

// TFQMR solver (Transpose-Free Quasi-Minimal Residual)
static int solve_tfqmr(
    const operator_matrix_t* A,
    const operator_vector_t* b,
    operator_vector_t* x,
    const iterative_solver_config_t* config,
    solver_statistics_t* stats) {
    
    if (!A || !b || !x || !config || !stats) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    int n = b->size;
    int max_iter = config->max_iterations > 0 ? config->max_iterations : n;
    real_t tol = config->tolerance > 0.0 ? config->tolerance : 1e-6;
    
    // Allocate workspace
    operator_vector_t* r = matvec_operator_create_vector(n);
    operator_vector_t* r_tilde = matvec_operator_create_vector(n);
    operator_vector_t* w = matvec_operator_create_vector(n);
    operator_vector_t* y = matvec_operator_create_vector(n);
    operator_vector_t* v = matvec_operator_create_vector(n);
    operator_vector_t* d = matvec_operator_create_vector(n);
    operator_vector_t* p = matvec_operator_create_vector(n);
    operator_vector_t* u = matvec_operator_create_vector(n);
    operator_vector_t* q = matvec_operator_create_vector(n);
    operator_vector_t* Ax = matvec_operator_create_vector(n);
    
    if (!r || !r_tilde || !w || !y || !v || !d || !p || !u || !q || !Ax) {
        if (r) matvec_operator_destroy_vector(r);
        if (r_tilde) matvec_operator_destroy_vector(r_tilde);
        if (w) matvec_operator_destroy_vector(w);
        if (y) matvec_operator_destroy_vector(y);
        if (v) matvec_operator_destroy_vector(v);
        if (d) matvec_operator_destroy_vector(d);
        if (p) matvec_operator_destroy_vector(p);
        if (u) matvec_operator_destroy_vector(u);
        if (q) matvec_operator_destroy_vector(q);
        if (Ax) matvec_operator_destroy_vector(Ax);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize
    stats->iterations = 0;
    stats->converged = false;
    
    // Compute initial residual: r0 = b - A*x0
    matvec_operator_apply(A, x, Ax);
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        r->data[i].re = b->data[i].re - Ax->data[i].re;
        r->data[i].im = b->data[i].im - Ax->data[i].im;
        r_tilde->data[i] = r->data[i];
        #else
        r->data[i] = b->data[i] - Ax->data[i];
        r_tilde->data[i] = r->data[i];
        #endif
    }
    
    // Compute initial residual norm
    real_t rho = 0.0;
    for (int i = 0; i < n; i++) {
        real_t re = r->data[i].re;
        real_t im = r->data[i].im;
        rho += re * re + im * im;
    }
    rho = sqrt(rho);
    
    if (rho < tol) {
        stats->converged = true;
        stats->residual_norm = rho;
        matvec_operator_destroy_vector(r);
        matvec_operator_destroy_vector(r_tilde);
        matvec_operator_destroy_vector(w);
        matvec_operator_destroy_vector(y);
        matvec_operator_destroy_vector(v);
        matvec_operator_destroy_vector(d);
        matvec_operator_destroy_vector(p);
        matvec_operator_destroy_vector(u);
        matvec_operator_destroy_vector(q);
        matvec_operator_destroy_vector(Ax);
        return STATUS_SUCCESS;
    }
    
    // Initialize TFQMR variables
    real_t tau = rho;
    real_t theta = 0.0;
    real_t eta = 0.0;
    
    // Initialize vectors
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        w->data[i].re = r->data[i].re;
        w->data[i].im = r->data[i].im;
        y->data[i].re = r->data[i].re;
        y->data[i].im = r->data[i].im;
        v->data[i].re = 0.0;
        v->data[i].im = 0.0;
        d->data[i].re = 0.0;
        d->data[i].im = 0.0;
        p->data[i].re = 0.0;
        p->data[i].im = 0.0;
        u->data[i].re = 0.0;
        u->data[i].im = 0.0;
        q->data[i].re = 0.0;
        q->data[i].im = 0.0;
        #else
        w->data[i] = r->data[i];
        y->data[i] = r->data[i];
        v->data[i] = 0.0 + 0.0 * I;
        d->data[i] = 0.0 + 0.0 * I;
        p->data[i] = 0.0 + 0.0 * I;
        u->data[i] = 0.0 + 0.0 * I;
        q->data[i] = 0.0 + 0.0 * I;
        #endif
    }
    
    // TFQMR iteration
    for (int m = 0; m < max_iter; m++) {
        // Compute v = A * y
        matvec_operator_apply(A, y, v);
        
        // Compute sigma = <r_tilde, v>
        #if defined(_MSC_VER)
        complex_t sigma = {0.0, 0.0};
        #else
        complex_t sigma = 0.0 + 0.0 * I;
        #endif
        #if defined(_MSC_VER)
        sigma.re = 0.0;
        sigma.im = 0.0;
        #else
        sigma = 0.0 + 0.0 * I;
        #endif
        
        for (int i = 0; i < n; i++) {
            #if defined(_MSC_VER)
            complex_t prod;
            prod.re = r_tilde->data[i].re * v->data[i].re + r_tilde->data[i].im * v->data[i].im;
            prod.im = r_tilde->data[i].re * v->data[i].im - r_tilde->data[i].im * v->data[i].re;
            sigma.re += prod.re;
            sigma.im += prod.im;
            #else
            sigma += conj(r_tilde->data[i]) * v->data[i];
            #endif
        }
        
        if (fabs(sigma.re) < NUMERICAL_EPSILON && fabs(sigma.im) < NUMERICAL_EPSILON) {
            // Breakdown
            break;
        }
        
        // Compute alpha = rho / sigma
        real_t alpha = rho / sigma.re;  // Use real part
        
        // Compute p = y - (alpha * theta^2 / eta) * p
        if (m > 0 && eta > NUMERICAL_EPSILON) {
            real_t coeff = alpha * theta * theta / eta;
            for (int i = 0; i < n; i++) {
                #if defined(_MSC_VER)
                p->data[i].re = y->data[i].re - coeff * p->data[i].re;
                p->data[i].im = y->data[i].im - coeff * p->data[i].im;
                #else
                p->data[i] = y->data[i] - coeff * p->data[i];
                #endif
            }
        } else {
            for (int i = 0; i < n; i++) {
                p->data[i] = y->data[i];
            }
        }
        
        // Compute u = A * p
        matvec_operator_apply(A, p, u);
        
        // Compute theta = ||u|| / tau
        real_t u_norm = 0.0;
        for (int i = 0; i < n; i++) {
            real_t re = u->data[i].re;
            real_t im = u->data[i].im;
            u_norm += re * re + im * im;
        }
        u_norm = sqrt(u_norm);
        theta = u_norm / tau;
        
        // Compute c = 1 / sqrt(1 + theta^2)
        real_t c = 1.0 / sqrt(1.0 + theta * theta);
        
        // Compute tau = tau * theta * c
        tau = tau * theta * c;
        
        // Compute eta = c^2 * alpha
        eta = c * c * alpha;
        
        // Update solution: x = x + eta * d
        for (int i = 0; i < n; i++) {
            #if defined(_MSC_VER)
            complex_t add;
            add.re = eta * d->data[i].re;
            add.im = eta * d->data[i].im;
            x->data[i].re += add.re;
            x->data[i].im += add.im;
            #else
            x->data[i] += eta * d->data[i];
            #endif
        }
        
        // Update d = p + (theta^2 * eta / alpha) * d
        if (alpha > NUMERICAL_EPSILON) {
            real_t coeff = theta * theta * eta / alpha;
            for (int i = 0; i < n; i++) {
                #if defined(_MSC_VER)
                complex_t add;
                add.re = coeff * d->data[i].re;
                add.im = coeff * d->data[i].im;
                d->data[i].re = p->data[i].re + add.re;
                d->data[i].im = p->data[i].im + add.im;
                #else
                d->data[i] = p->data[i] + coeff * d->data[i];
                #endif
            }
        } else {
            for (int i = 0; i < n; i++) {
                d->data[i] = p->data[i];
            }
        }
        
        // Update w = w - alpha * v
        for (int i = 0; i < n; i++) {
            #if defined(_MSC_VER)
            complex_t sub;
            sub.re = alpha * v->data[i].re;
            sub.im = alpha * v->data[i].im;
            w->data[i].re -= sub.re;
            w->data[i].im -= sub.im;
            #else
            w->data[i] -= alpha * v->data[i];
            #endif
        }
        
        // Update y = w - (theta^2 * eta / alpha) * y
        if (alpha > NUMERICAL_EPSILON) {
            real_t coeff = theta * theta * eta / alpha;
            for (int i = 0; i < n; i++) {
                #if defined(_MSC_VER)
                complex_t sub;
                sub.re = coeff * y->data[i].re;
                sub.im = coeff * y->data[i].im;
                y->data[i].re = w->data[i].re - sub.re;
                y->data[i].im = w->data[i].im - sub.im;
                #else
                y->data[i] = w->data[i] - coeff * y->data[i];
                #endif
            }
        } else {
            for (int i = 0; i < n; i++) {
                y->data[i] = w->data[i];
            }
        }
        
        // Check convergence using tau (quasi-residual norm)
        stats->iterations = m + 1;
        stats->residual_norm = tau;
        
        if (tau < tol) {
            stats->converged = true;
            break;
        }
        
        // Update rho for next iteration
        // rho = <r_tilde, w>
        rho = 0.0;
        for (int i = 0; i < n; i++) {
            #if defined(_MSC_VER)
            real_t re = r_tilde->data[i].re * w->data[i].re + r_tilde->data[i].im * w->data[i].im;
            rho += re;
            #else
            rho += creal(conj(r_tilde->data[i]) * w->data[i]);
            #endif
        }
        
        if (fabs(rho) < NUMERICAL_EPSILON) {
            // Breakdown
            break;
        }
    }
    
    // Cleanup
    matvec_operator_destroy_vector(r);
    matvec_operator_destroy_vector(r_tilde);
    matvec_operator_destroy_vector(w);
    matvec_operator_destroy_vector(y);
    matvec_operator_destroy_vector(v);
    matvec_operator_destroy_vector(d);
    matvec_operator_destroy_vector(p);
    matvec_operator_destroy_vector(u);
    matvec_operator_destroy_vector(q);
    matvec_operator_destroy_vector(Ax);
    
    if (!stats->converged) {
        return STATUS_ERROR_CONVERGENCE_FAILURE;
    }
    
    return STATUS_SUCCESS;
}

solver_interface_t* iterative_solver_create(
    const iterative_solver_config_t* config) {
    
    if (!config) return NULL;
    
    iterative_solver_data_t* data = (iterative_solver_data_t*)calloc(1, sizeof(iterative_solver_data_t));
    if (!data) return NULL;
    
    memcpy(&data->config, config, sizeof(iterative_solver_config_t));
    data->is_initialized = false;
    
    // Cast to solver_interface_t for return
    return (solver_interface_t*)data;
}

int iterative_solver_set_preconditioner(
    solver_interface_t* solver,
    const operator_matrix_t* preconditioner_matrix) {
    
    if (!solver) return STATUS_ERROR_INVALID_INPUT;
    
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver;
    data->preconditioner = preconditioner_matrix;
    
    return STATUS_SUCCESS;
}

int iterative_solver_solve(
    solver_interface_t* solver,
    const operator_matrix_t* matrix,
    const operator_vector_t* rhs,
    operator_vector_t* solution,
    solver_statistics_t* statistics) {
    
    if (!solver || !matrix || !rhs || !solution || !statistics) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver;
    
    // Initialize statistics
    memset(statistics, 0, sizeof(solver_statistics_t));
    statistics->converged = false;
    
    // Initialize solution (use zero or provided initial guess)
    if (data->initial_guess) {
        vector_copy(data->initial_guess, solution);
    } else {
        // Zero initial guess
        for (int i = 0; i < solution->size; i++) {
            #if defined(_MSC_VER)
            solution->data[i].re = 0.0;
            solution->data[i].im = 0.0;
            #else
            solution->data[i] = 0.0 + 0.0 * I;
            #endif
        }
    }
    
    // Solve based on method
    status_t status;
    switch (data->config.method) {
        case ITERATIVE_METHOD_CG:
            status = solve_cg(matrix, rhs, solution, &data->config, statistics);
            break;
            
        case ITERATIVE_METHOD_GMRES:
            status = solve_gmres(matrix, rhs, solution, &data->config, statistics);
            break;
            
        case ITERATIVE_METHOD_BICGSTAB:
            status = solve_bicgstab(matrix, rhs, solution, &data->config, statistics);
            break;
            
        case ITERATIVE_METHOD_TFQMR:
            status = solve_tfqmr(matrix, rhs, solution, &data->config, statistics);
            break;
            
        default:
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    return status;
}
