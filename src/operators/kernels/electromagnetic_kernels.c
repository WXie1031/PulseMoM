/********************************************************************************
 * PulseEM - Electromagnetic Kernels Main Implementation
 * 
 * This file includes all module implementations for electromagnetic kernels.
 * The code is organized into functional modules for better maintainability.
 * 
 * Modules:
 * - integration_utils: Utility functions (Gaussian quadrature, vector ops, etc.)
 * - greens_functions: Green's function implementations
 * - single_integrals: Single integrals (triangle, rectangle, wire)
 * - singularity_treatment: Singularity handling
 * - double_integrals_peec: PEEC double integrals
 * - rwg_integrals_mom: MoM RWG basis function integrals
 ********************************************************************************/

// Define BUILDING_ELECTROMAGNETIC_KERNELS_DLL so EM_KERNEL_API expands to dllexport
#define BUILDING_ELECTROMAGNETIC_KERNELS_DLL

#include "core_common.h"
#include "electromagnetic_kernels.h"
// integration_utils.h is included after electromagnetic_kernels.h to avoid circular dependency
#include "../../operators/integration/integration_utils.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

// Helper function for complex number creation
static complex_t mc(double re, double im) {
    complex_t z;
    z.re = re;
    z.im = im;
    return z;
}

static double* cs_nodes = NULL;
static double* cs_weights = NULL;
static int cs_n = 0;
static double cs_kmax = 0.0;
static void prepare_cs_table(int n, double kmax){
    if(n<=0 || kmax<=0.0) return;
    if(cs_n != n || fabs(cs_kmax - kmax) > 1e-12){
        if(cs_nodes){ free(cs_nodes); cs_nodes=NULL; }
        if(cs_weights){ free(cs_weights); cs_weights=NULL; }
        cs_nodes = (double*)malloc(sizeof(double)*n);
        cs_weights = (double*)malloc(sizeof(double)*n);
        cs_n = n; cs_kmax = kmax;
        if(cs_nodes && cs_weights){
            double dk = kmax / n;
            for(int i=0;i<n;i++){ cs_nodes[i] = (i+1)*dk; cs_weights[i] = dk; }
        }
    }
}

static complex_t integrate_continuous_spectrum_same_layer(double rho, double dz, double k0, int n_points, double kmax, double amp) {
    if(rho<0.0) rho = -rho;
    if(dz<0.0) dz = -dz;
    int n = n_points>0 ? n_points : 32;
    double km = kmax>0.0 ? kmax : (k0*(1.0+8.0/(1.0+fmax(rho*k0,1e-6))));
    prepare_cs_table(n, km);
    if(!cs_nodes || !cs_weights) return mc(0.0,0.0);
    double beta = 0.05;
    double eps = 1e-12;
    double sr = 0.0, si = 0.0;
    int i;
#ifdef _OPENMP
    if (n > 100) {
#pragma omp parallel for reduction(+:sr,si)
    for(i=0;i<n;i++){
#else
    for(i=0;i<n;i++){
#endif
        double kr = cs_nodes[i];
        double w = cs_weights[i];
        double j0 = (rho>0.0)? cos(kr*rho) : 1.0;
        double conv = exp(-beta*kr*rho);
        if(kr <= k0){
            double kz = sqrt(fmax(k0*k0 - kr*kr, 0.0));
            double phase = kz*dz;
            double c = cos(phase);
            double s = sin(phase);
            double denom = kz + eps;
            sr += w * j0 * conv * (c/denom);
            si += w * j0 * conv * (s/denom);
        } else {
            double kappa = sqrt(kr*kr - k0*k0);
            double dec = exp(-kappa*dz);
            double denom = kappa + eps;
            sr += w * j0 * conv * (dec/denom);
        }
    }
#ifdef _OPENMP
    }
#endif
    double scale = 1.0/(2.0*M_PI);
    sr *= scale * amp;
    si *= scale * amp;
    return mc(sr, si);
}

static complex_t integrate_continuous_spectrum_cross_layer(double rho, double dz, double k0, int n_points, double kmax, double amp) {
    return integrate_continuous_spectrum_same_layer(rho,dz,k0,n_points,kmax,amp);
}

EM_KERNEL_API complex_t green_function_free_space(double r, double k) {
    if(r <= DBL_EPSILON) return mc(0.0, 0.0);
    double a = 1.0 / (4.0 * M_PI * r);
    return mc(a * cos(k * r), a * sin(k * r));
}

EM_KERNEL_API void green_function_gradient_free_space(double r, double k, const double* r_vec, double* gradient) {
    if(!r_vec||!gradient){return;}
    if(r<=DBL_EPSILON){ gradient[0]=gradient[1]=gradient[2]=0.0; return; }
    complex_t G = green_function_free_space(r, k);
    complex_t coeff;
    coeff.re = -1.0 / r;
    coeff.im = k;
    complex_t dGdr = complex_multiply(&G, &coeff);
    double inv_r = 1.0 / r;
    gradient[0] = dGdr.re * (r_vec[0] * inv_r);
    gradient[1] = dGdr.re * (r_vec[1] * inv_r);
    gradient[2] = dGdr.re * (r_vec[2] * inv_r);
}

int find_layer_containing_point(double z, int n_layers, const layered_media_t* layers) { 
    if(!layers || n_layers <= 0) return -1; 
    double acc = 0.0; 
    for(int i = 0; i < n_layers; i++) { 
        double nxt = acc + layers[i].thickness; 
        if(z >= acc && z <= nxt) return i; 
        acc = nxt; 
    } 
    return -1; 
}

EM_KERNEL_API complex_t green_function_layered_media(double rho, double z, double z_prime, double k0, int n_layers, const layered_media_t* layers) {
    complex_t Gfs = green_function_free_space(rho, k0);
    if (!layers || n_layers <= 0) return Gfs;
    int src = find_layer_containing_point(z_prime, n_layers, layers);
    int obs = find_layer_containing_point(z, n_layers, layers);
    if (src < 0 || obs < 0) return Gfs;
    complex_t Zs = layers[src].impedance;
    complex_t Zo = layers[obs].impedance;
    complex_t twoZo; twoZo.re = 2.0 * Zo.re; twoZo.im = 2.0 * Zo.im;
    complex_t den = complex_add(&Zo, &Zs);
    complex_t T = complex_divide(&twoZo, &den);
    double a_t = complex_magnitude(&T);
    if (a_t < 0.1) a_t = 0.1; if (a_t > 2.0) a_t = 2.0;
    double dz = fabs(z - z_prime);
    double a_att = 1.0;
    double sigma = layers[obs].conductivity;
    if (sigma > 1e-12) {
        double mu_eff = complex_magnitude(&layers[obs].permeability);
        if (mu_eff > 0.0) {
            double omega = k0 * C0;
            double delta = sqrt(2.0 / (omega * mu_eff * sigma));
            if (delta > 1e-12) {
                a_att = exp(-dz / delta);
                if (a_att < exp(-10.0)) a_att = exp(-10.0);
            }
        }
    }
    double a_total = a_t * a_att;
    int npts = 32;
    double kmax = k0*8.0;
    complex_t Gcont;
    if(src == obs){
        Gcont = integrate_continuous_spectrum_same_layer(rho,dz,k0,npts,kmax,a_total);
    } else {
        Gcont = integrate_continuous_spectrum_cross_layer(rho,dz,k0,npts,kmax,a_total);
    }
    double mag = complex_magnitude(&Gcont);
    if(!(mag>=0.0) || mag>1e6){
        return mc(Gfs.re * a_total, Gfs.im * a_total);
    }
    return Gcont;
}

complex_t green_function_layered_dcim(double rho, double z, double z_prime, double k0, int n_layers, const layered_media_t* layers, double error_tolerance) { 
    (void)error_tolerance; 
    return green_function_layered_media(rho, z, z_prime, k0, n_layers, layers); 
}

complex_t green_function_layered_fast(double rho, double z, double z_prime, double k0, int n_layers, const layered_media_t* layers) { 
    return green_function_layered_media(rho, z, z_prime, k0, n_layers, layers); 
}

complex_t green_function_periodic(double r, double k, const double* periodicity, int n_harmonics) { 
    (void)periodicity; 
    (void)n_harmonics; 
    return green_function_free_space(r, k); 
}

/********************************************************************************
 * Note: Gaussian Quadrature and distance computation functions are now in
 * integration_utils.c. They are included via integration_utils.h header.
 ********************************************************************************/

/********************************************************************************
 * Triangle Surface Integral with Proper Numerical Integration
 ********************************************************************************/
complex_t integrate_triangle_singular(const triangle_element_t* triangle, const double* obs_point, double k, integral_kernel_t kernel_type) {
    if (!triangle || !obs_point) return mc(0.0, 0.0);
    
    // For singular case (observation point on triangle), use singularity extraction
    double r_min = DBL_MAX;
    for (int i = 0; i < 3; i++) {
        double r = compute_distance_3d(obs_point, (const double*)(triangle->vertices[i]));
        if (r < r_min) r_min = r;
    }
    
    // If very close to triangle, use singularity extraction
    if (r_min < 1e-6) {
        return singularity_extraction_1overr(r_min, k);
    }
    
    // Regular integration using Gaussian quadrature
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result_re = 0.0, result_im = 0.0;
    
    // Triangle integration using area coordinates
    for (int i = 0; i < n_points; i++) {
        double u = 0.5 * (xi[i] + 1.0);
        double wu = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double v = 0.5 * (xi[j] + 1.0);
            double wv = wi[j];
            
            // Area coordinates
            double w1 = 1.0 - u - v;
            double w2 = u;
            double w3 = v;
            
            if (w1 < 0 || w2 < 0 || w3 < 0) continue;
            
            // Interpolate point on triangle
            double q[3];
            q[0] = w1 * triangle->vertices[0][0] + w2 * triangle->vertices[1][0] + w3 * triangle->vertices[2][0];
            q[1] = w1 * triangle->vertices[0][1] + w2 * triangle->vertices[1][1] + w3 * triangle->vertices[2][1];
            q[2] = w1 * triangle->vertices[0][2] + w2 * triangle->vertices[1][2] + w3 * triangle->vertices[2][2];
            
            double r = compute_distance_3d(obs_point, q);
            if (r < DBL_EPSILON) continue;
            
            complex_t G = green_function_free_space(r, k);
            
            // Jacobian for triangle (2 * area)
            double jacobian = 2.0 * triangle->area;
            
            result_re += wu * wv * G.re * jacobian;
            result_im += wu * wv * G.im * jacobian;
        }
    }
    
    return mc(result_re, result_im);
}

/********************************************************************************
 * Rectangle Surface Integral with Proper Numerical Integration
 ********************************************************************************/
complex_t integrate_rectangle_regular(const rectangle_element_t* r,const double* p,double k,integral_kernel_t kt){
    if (!r || !p) return mc(0.0, 0.0);
    
    // Regular integration using Gaussian quadrature
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result_re = 0.0, result_im = 0.0;
    
    // Rectangle integration using bilinear mapping
    for (int i = 0; i < n_points; i++) {
        double u = 0.5 * (xi[i] + 1.0);
        double wu = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double v = 0.5 * (xi[j] + 1.0);
            double wv = wi[j];
            
            // Bilinear interpolation (simplified for rectangle)
            double q[3];
            q[0] = r->vertices[0][0] + u * (r->vertices[1][0] - r->vertices[0][0]) + 
                   v * (r->vertices[3][0] - r->vertices[0][0]);
            q[1] = r->vertices[0][1] + u * (r->vertices[1][1] - r->vertices[0][1]) + 
                   v * (r->vertices[3][1] - r->vertices[0][1]);
            q[2] = r->vertices[0][2] + u * (r->vertices[1][2] - r->vertices[0][2]) + 
                   v * (r->vertices[3][2] - r->vertices[0][2]);
            
            double r_dist = compute_distance_3d(p, q);
            if (r_dist < DBL_EPSILON) continue;
            
            complex_t G = green_function_free_space(r_dist, k);
            
            // Jacobian for rectangle (area)
            double jacobian = r->area;
            
            result_re += wu * wv * G.re * jacobian;
            result_im += wu * wv * G.im * jacobian;
        }
    }
    
    return mc(result_re, result_im);
}

/********************************************************************************
 * Wire Line Integral with Proper Numerical Integration
 ********************************************************************************/
complex_t integrate_wire_thin(const wire_element_t* w,const double* p,double k,integral_kernel_t kt){
    if (!w || !p) return mc(0.0, 0.0);
    
    // Line integral using Gaussian quadrature
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result_re = 0.0, result_im = 0.0;
    
    // Wire integration
    for (int i = 0; i < n_points; i++) {
        double t = 0.5 * (xi[i] + 1.0);  // Map to [0, 1]
        double wt = wi[i];
        
        // Interpolate point on wire
        double q[3];
        q[0] = w->start[0] + t * (w->end[0] - w->start[0]);
        q[1] = w->start[1] + t * (w->end[1] - w->start[1]);
        q[2] = w->start[2] + t * (w->end[2] - w->start[2]);
        
        double r = compute_distance_3d(p, q);
        if (r < DBL_EPSILON) continue;
        
        complex_t G = green_function_free_space(r, k);
        
        // Jacobian for line (length)
        double jacobian = w->length;
        
        result_re += wt * G.re * jacobian;
        result_im += wt * G.im * jacobian;
    }
    
    return mc(result_re, result_im);
}

complex_t singularity_extraction_1overr(double r,double k){ (void)k; if(r<=DBL_EPSILON) return mc(0.0,0.0); double v=1.0/(4.0*M_PI*r); return mc(v,0.0); }

/********************************************************************************
 * Edge Singularity Treatment with Duffy Transformation
 * 
 * Uses Duffy transformation to handle edge singularities in triangular elements
 ********************************************************************************/
complex_t edge_singularity_treatment(const triangle_element_t* t,const double* p,double k,int e){
    if (!t || !p || e < 0 || e >= 3) return mc(0.0, 0.0);
    
    // For edge singularity, use Duffy transformation
    // Map from [0,1]×[0,1] to triangle with edge singularity
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result_re = 0.0, result_im = 0.0;
    
    // Duffy transformation: u = s, v = s*t
    for (int i = 0; i < n_points; i++) {
        double s = 0.5 * (xi[i] + 1.0);  // Map to [0, 1]
        double ws = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double t_param = 0.5 * (xi[j] + 1.0);  // Renamed to avoid shadowing function parameter 't'
            double wt = wi[j];
            
            // Duffy transformation
            double u = s;
            double v = s * t_param;
            
            // Area coordinates
            double w1, w2, w3;
            if (e == 0) {
                // Edge between vertex 0 and 1
                w1 = 1.0 - u;
                w2 = u - v;
                w3 = v;
            } else if (e == 1) {
                // Edge between vertex 1 and 2
                w1 = v;
                w2 = 1.0 - u;
                w3 = u - v;
            } else {
                // Edge between vertex 2 and 0
                w1 = u - v;
                w2 = v;
                w3 = 1.0 - u;
            }
            
            if (w1 < 0 || w2 < 0 || w3 < 0) continue;
            
            // Interpolate point on triangle
            double q[3];
            q[0] = w1 * t->vertices[0][0] + w2 * t->vertices[1][0] + w3 * t->vertices[2][0];
            q[1] = w1 * t->vertices[0][1] + w2 * t->vertices[1][1] + w3 * t->vertices[2][1];
            q[2] = w1 * t->vertices[0][2] + w2 * t->vertices[1][2] + w3 * t->vertices[2][2];
            
            double r = compute_distance_3d(p, q);
            if (r < DBL_EPSILON) continue;
            
            complex_t G = green_function_free_space(r, k);
            
            // Jacobian for Duffy transformation: s (from u=s, v=s*t_param)
            double jacobian = 2.0 * t->area * s;
            
            result_re += ws * wt * G.re * jacobian;
            result_im += ws * wt * G.im * jacobian;
        }
    }
    
    return mc(result_re, result_im);
}

void green_function_layered_batch_gpu(const double* ra,const double* za,const double* zpa,int n,double k0,int nl,const layered_media_t* L,complex_t* out){ for(int i=0;i<n;i++) out[i]=green_function_layered_media(ra[i],za[i],zpa[i],k0,nl,L); }

void green_function_layered_batch_memory_efficient(const double* ra,const double* za,const double* zpa,int n,double k0,int nl,const layered_media_t* L,complex_t* out,int bs){
    int b = bs>0 ? bs : 256;
    int i = 0;
    while(i < n){
        int end = i + b;
        if(end > n) end = n;
        for(int j=i;j<end;j++) out[j]=green_function_layered_media(ra[j],za[j],zpa[j],k0,nl,L);
        i = end;
    }
}

void green_function_layered_batch_adaptive(const double* ra,const double* za,const double* zpa,int n,double k0,int nl,const layered_media_t* L,complex_t* out,double ta){ (void)ta; for(int i=0;i<n;i++) out[i]=green_function_layered_media(ra[i],za[i],zpa[i],k0,nl,L); }

complex_t* find_surface_wave_poles(double k0,int nl,const layered_media_t* L,int* np){ (void)k0;(void)nl;(void)L; if(np) *np=0; return NULL; }

complex_t hankel_function(double x){ return mc(cos(x),-sin(x)); }

complex_t bessel_j0(complex_t z){ (void)z; return mc(1.0,0.0); }

complex_t calculate_reflection_coefficient_upward(int s,int n,const layered_media_t* L,double k0){ (void)s;(void)n;(void)L;(void)k0; return mc(0.0,0.0); }

complex_t calculate_reflection_coefficient_downward(int s,int n,const layered_media_t* L,double k0){ (void)s;(void)n;(void)L;(void)k0; return mc(0.0,0.0); }

complex_t calculate_surface_wave_contribution(double rho,double z,double zp,double k0,int n,const layered_media_t* L){ (void)rho;(void)z;(void)zp;(void)k0;(void)n;(void)L; return mc(0.0,0.0); }

complex_t green_function_layered_continuous_spectrum(double rho,double z,double zp,double k0,int n,const layered_media_t* L){
    int src = find_layer_containing_point(zp, n, L);
    int obs = find_layer_containing_point(z, n, L);
    if(src<0 || obs<0) return green_function_free_space(rho, k0);
    complex_t Zs = L[src].impedance;
    complex_t Zo = L[obs].impedance;
    complex_t twoZo; twoZo.re = 2.0 * Zo.re; twoZo.im = 2.0 * Zo.im;
    complex_t den = complex_add(&Zo, &Zs);
    complex_t T = complex_divide(&twoZo, &den);
    double a_t = complex_magnitude(&T);
    if (a_t < 0.1) a_t = 0.1; if (a_t > 2.0) a_t = 2.0;
    double dz = fabs(z - zp);
    double a_att = 1.0;
    double sigma = L[obs].conductivity;
    if (sigma > 1e-12) {
        double mu_eff = complex_magnitude(&L[obs].permeability);
        if (mu_eff > 0.0) {
            double omega = k0 * C0;
            double delta = sqrt(2.0 / (omega * mu_eff * sigma));
            if (delta > 1e-12) {
                a_att = exp(-dz / delta);
                if (a_att < exp(-10.0)) a_att = exp(-10.0);
            }
        }
    }
    double a_total = a_t * a_att;
    int npts = 32;
    double kmax = k0*8.0;
    if(src==obs) return integrate_continuous_spectrum_same_layer(rho,dz,k0,npts,kmax,a_total);
    return integrate_continuous_spectrum_cross_layer(rho,dz,k0,npts,kmax,a_total);
}

complex_t spectral_domain_green_function(complex_t kr,double z,double zp,double k0,int n,const layered_media_t* L,int sl,int ol){ (void)kr;(void)z;(void)zp;(void)k0;(void)n;(void)L;(void)sl;(void)ol; return mc(0.0,0.0); }

/********************************************************************************
 * Double Integration Functions for PEEC
 ********************************************************************************/

/********************************************************************************
 * Double Surface Integral for Partial Capacitance
 * C_ij = (1/4πε₀) ∫∫ dS_i dS_j / |r_i - r_j|
 ********************************************************************************/
double integrate_surface_surface_capacitance(
    const triangle_element_t *surf_i,
    const triangle_element_t *surf_j,
    double k) {
    
    if (!surf_i || !surf_j) return 0.0;
    
    // EPS0 is already defined in core_common.h
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result = 0.0;
    
    // Double integral over both triangles
    for (int i = 0; i < n_points; i++) {
        double u1 = 0.5 * (xi[i] + 1.0);
        double wu1 = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double v1 = 0.5 * (xi[j] + 1.0);
            double wv1 = wi[j];
            
            // Area coordinates for first triangle
            double w1_i = 1.0 - u1 - v1;
            double w2_i = u1;
            double w3_i = v1;
            
            if (w1_i < 0 || w2_i < 0 || w3_i < 0) continue;
            
            double p1[3];
            p1[0] = w1_i * surf_i->vertices[0][0] + w2_i * surf_i->vertices[1][0] + w3_i * surf_i->vertices[2][0];
            p1[1] = w1_i * surf_i->vertices[0][1] + w2_i * surf_i->vertices[1][1] + w3_i * surf_i->vertices[2][1];
            p1[2] = w1_i * surf_i->vertices[0][2] + w2_i * surf_i->vertices[1][2] + w3_i * surf_i->vertices[2][2];
            
            for (int k_idx = 0; k_idx < n_points; k_idx++) {
                double u2 = 0.5 * (xi[k_idx] + 1.0);
                double wu2 = wi[k_idx];
                
                for (int l = 0; l < n_points; l++) {
                    double v2 = 0.5 * (xi[l] + 1.0);
                    double wv2 = wi[l];
                    
                    // Area coordinates for second triangle
                    double w1_j = 1.0 - u2 - v2;
                    double w2_j = u2;
                    double w3_j = v2;
                    
                    if (w1_j < 0 || w2_j < 0 || w3_j < 0) continue;
                    
                    double p2[3];
                    p2[0] = w1_j * surf_j->vertices[0][0] + w2_j * surf_j->vertices[1][0] + w3_j * surf_j->vertices[2][0];
                    p2[1] = w1_j * surf_j->vertices[0][1] + w2_j * surf_j->vertices[1][1] + w3_j * surf_j->vertices[2][1];
                    p2[2] = w1_j * surf_j->vertices[0][2] + w2_j * surf_j->vertices[1][2] + w3_j * surf_j->vertices[2][2];
                    
                    double r = compute_distance_3d(p1, p2);
                    
                    // Avoid singularity
                    if (r < 1e-6) {
                        double char_length = sqrt(surf_i->area);
                        r = char_length * 0.1;
                    }
                    
                    // Integrand: 1 / |r_i - r_j|
                    double integrand = 1.0 / r;
                    
                    // Jacobian for both triangles
                    double jacobian = 2.0 * surf_i->area * 2.0 * surf_j->area;
                    
                    result += wu1 * wv1 * wu2 * wv2 * integrand * jacobian;
                }
            }
        }
    }
    
    return (1.0 / (4.0 * M_PI * EPS0)) * result;
}

/********************************************************************************
 * Double Surface Integral for Partial Inductance
 * L_ij = (μ₀/4π) ∫∫ (J_i · J_j) / |r_i - r_j| dS_i dS_j
 ********************************************************************************/
complex_t integrate_surface_surface_inductance(
    const triangle_element_t *surf_i,
    const triangle_element_t *surf_j,
    double k,
    int gauss_order) {
    
    if (!surf_i || !surf_j) return mc(0.0, 0.0);
    
    // MU0 is already defined in core_common.h
    // Validate and normalize gauss_order parameter
    int n_points = (gauss_order > 0 && (gauss_order == 1 || gauss_order == 4 || gauss_order == 7 || gauss_order == 8)) 
                   ? gauss_order : 4;  // Default to 4 if invalid
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result_re = 0.0, result_im = 0.0;
    
    // Double integral over both triangles
    for (int i = 0; i < n_points; i++) {
        double u1 = 0.5 * (xi[i] + 1.0);
        double wu1 = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double v1 = 0.5 * (xi[j] + 1.0);
            double wv1 = wi[j];
            
            // Area coordinates for first triangle
            double w1_i = 1.0 - u1 - v1;
            double w2_i = u1;
            double w3_i = v1;
            
            if (w1_i < 0 || w2_i < 0 || w3_i < 0) continue;
            
            double p1[3];
            p1[0] = w1_i * surf_i->vertices[0][0] + w2_i * surf_i->vertices[1][0] + w3_i * surf_i->vertices[2][0];
            p1[1] = w1_i * surf_i->vertices[0][1] + w2_i * surf_i->vertices[1][1] + w3_i * surf_i->vertices[2][1];
            p1[2] = w1_i * surf_i->vertices[0][2] + w2_i * surf_i->vertices[1][2] + w3_i * surf_i->vertices[2][2];
            
            for (int k_idx = 0; k_idx < n_points; k_idx++) {
                double u2 = 0.5 * (xi[k_idx] + 1.0);
                double wu2 = wi[k_idx];
                
                for (int l = 0; l < n_points; l++) {
                    double v2 = 0.5 * (xi[l] + 1.0);
                    double wv2 = wi[l];
                    
                    // Area coordinates for second triangle
                    double w1_j = 1.0 - u2 - v2;
                    double w2_j = u2;
                    double w3_j = v2;
                    
                    if (w1_j < 0 || w2_j < 0 || w3_j < 0) continue;
                    
                    double p2[3];
                    p2[0] = w1_j * surf_j->vertices[0][0] + w2_j * surf_j->vertices[1][0] + w3_j * surf_j->vertices[2][0];
                    p2[1] = w1_j * surf_j->vertices[0][1] + w2_j * surf_j->vertices[1][1] + w3_j * surf_j->vertices[2][1];
                    p2[2] = w1_j * surf_j->vertices[0][2] + w2_j * surf_j->vertices[1][2] + w3_j * surf_j->vertices[2][2];
                    
                    double r = compute_distance_3d(p1, p2);
                    
                    // Avoid singularity
                    if (r < 1e-6) {
                        double char_length = sqrt(surf_i->area);
                        r = char_length * 0.1;
                    }
                    
                    // Current density dot product (simplified: assume parallel currents)
                    double J_dot = 1.0;
                    
                    // Integrand: (J_i · J_j) / |r_i - r_j|
                    double integrand = J_dot / r;
                    
                    // Jacobian for both triangles
                    double jacobian = 2.0 * surf_i->area * 2.0 * surf_j->area;
                    
                    result_re += wu1 * wv1 * wu2 * wv2 * integrand * jacobian;
                }
            }
        }
    }
    
    return mc((MU0 / (4.0 * M_PI)) * result_re, 0.0);
}

/********************************************************************************
 * Neumann Formula Integral for Wire-Wire Partial Inductance
 * L_ij = (μ₀/4π) ∫∫ (dl_i · dl_j) / |r_i - r_j|
 ********************************************************************************/
complex_t integrate_wire_wire_neumann(
    const wire_element_t *wire_i,
    const wire_element_t *wire_j,
    double k) {
    
    if (!wire_i || !wire_j) return mc(0.0, 0.0);
    
    // MU0 is already defined in core_common.h
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    // Direction vectors
    double dl1[3] = {
        wire_i->end[0] - wire_i->start[0],
        wire_i->end[1] - wire_i->start[1],
        wire_i->end[2] - wire_i->start[2]
    };
    double dl2[3] = {
        wire_j->end[0] - wire_j->start[0],
        wire_j->end[1] - wire_j->start[1],
        wire_j->end[2] - wire_j->start[2]
    };
    
    double result = 0.0;
    
    // Double line integral
    for (int i = 0; i < n_points; i++) {
        double t1 = 0.5 * (xi[i] + 1.0);  // Map to [0, 1]
        double wt1 = wi[i];
        
        double p1[3];
        p1[0] = wire_i->start[0] + t1 * dl1[0];
        p1[1] = wire_i->start[1] + t1 * dl1[1];
        p1[2] = wire_i->start[2] + t1 * dl1[2];
        
        for (int j = 0; j < n_points; j++) {
            double t2 = 0.5 * (xi[j] + 1.0);
            double wt2 = wi[j];
            
            double p2[3];
            p2[0] = wire_j->start[0] + t2 * dl2[0];
            p2[1] = wire_j->start[1] + t2 * dl2[1];
            p2[2] = wire_j->start[2] + t2 * dl2[2];
            
            double r = compute_distance_3d(p1, p2);
            
            // Avoid singularity (self-term)
            if (r < 1e-6) {
                if (wire_i == wire_j && wire_i->radius > 0) {
                    r = wire_i->radius;
                } else {
                    r = 1e-6;
                }
            }
            
            // Dot product of direction vectors
            double dl_dot = dl1[0] * dl2[0] + dl1[1] * dl2[1] + dl1[2] * dl2[2];
            
            // Integrand: (dl_i · dl_j) / |r_i - r_j|
            double integrand = dl_dot / r;
            
            // Jacobian for both integrals
            double jacobian = wire_i->length * wire_j->length;
            
            result += wt1 * wt2 * integrand * jacobian;
        }
    }
    
    return mc((MU0 / (4.0 * M_PI)) * result, 0.0);
}

/********************************************************************************
 * Double Quadrilateral Integral for Partial Elements
 ********************************************************************************/
complex_t integrate_quad_quad_double(
    const rectangle_element_t *quad_i,
    const rectangle_element_t *quad_j,
    double k) {
    
    if (!quad_i || !quad_j) return mc(0.0, 0.0);
    
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    double result_re = 0.0, result_im = 0.0;
    
    // Double integral over both quadrilaterals
    for (int i = 0; i < n_points; i++) {
        double u1 = 0.5 * (xi[i] + 1.0);
        double wu1 = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double v1 = 0.5 * (xi[j] + 1.0);
            double wv1 = wi[j];
            
            // Bilinear interpolation for first quad
            double p1[3];
            p1[0] = (1-u1)*(1-v1)*quad_i->vertices[0][0] + u1*(1-v1)*quad_i->vertices[1][0] +
                    u1*v1*quad_i->vertices[2][0] + (1-u1)*v1*quad_i->vertices[3][0];
            p1[1] = (1-u1)*(1-v1)*quad_i->vertices[0][1] + u1*(1-v1)*quad_i->vertices[1][1] +
                    u1*v1*quad_i->vertices[2][1] + (1-u1)*v1*quad_i->vertices[3][1];
            p1[2] = (1-u1)*(1-v1)*quad_i->vertices[0][2] + u1*(1-v1)*quad_i->vertices[1][2] +
                    u1*v1*quad_i->vertices[2][2] + (1-u1)*v1*quad_i->vertices[3][2];
            
            for (int k_idx = 0; k_idx < n_points; k_idx++) {
                double u2 = 0.5 * (xi[k_idx] + 1.0);
                double wu2 = wi[k_idx];
                
                for (int l = 0; l < n_points; l++) {
                    double v2 = 0.5 * (xi[l] + 1.0);
                    double wv2 = wi[l];
                    
                    // Bilinear interpolation for second quad
                    double p2[3];
                    p2[0] = (1-u2)*(1-v2)*quad_j->vertices[0][0] + u2*(1-v2)*quad_j->vertices[1][0] +
                            u2*v2*quad_j->vertices[2][0] + (1-u2)*v2*quad_j->vertices[3][0];
                    p2[1] = (1-u2)*(1-v2)*quad_j->vertices[0][1] + u2*(1-v2)*quad_j->vertices[1][1] +
                            u2*v2*quad_j->vertices[2][1] + (1-u2)*v2*quad_j->vertices[3][1];
                    p2[2] = (1-u2)*(1-v2)*quad_j->vertices[0][2] + u2*(1-v2)*quad_j->vertices[1][2] +
                            u2*v2*quad_j->vertices[2][2] + (1-u2)*v2*quad_j->vertices[3][2];
                    
                    double r = compute_distance_3d(p1, p2);
                    
                    // Avoid singularity
                    if (r < 1e-6) {
                        double char_length = sqrt(quad_i->area);
                        r = char_length * 0.1;
                    }
                    
                    complex_t G = green_function_free_space(r, k);
                    
                    // Jacobian for quadrilaterals
                    double jacobian = quad_i->area * quad_j->area / 16.0;
                    
                    result_re += wu1 * wv1 * wu2 * wv2 * G.re * jacobian;
                    result_im += wu1 * wv1 * wu2 * wv2 * G.im * jacobian;
                }
            }
        }
    }
    
    return mc(result_re, result_im);
}

/********************************************************************************
 * MoM (Method of Moments) RWG Basis Function Integration Functions
 ********************************************************************************/

// Helper function: Compute RWG basis function vector at a point on a triangle
// f_n(r) = (l_n / (2*A_n)) * (r - r_n) for positive triangle
// f_n(r) = (l_n / (2*A_n)) * (r_n - r) for negative triangle
static void compute_rwg_basis_vector(
    const triangle_element_t* tri,
    const double* r_point,
    const double* r_opposite_vertex,  // Vertex opposite to the edge
    double edge_length,
    double triangle_area,
    int is_plus_triangle,  // 1 for plus, 0 for minus
    double* basis_vec) {
    
    if (!tri || !r_point || !r_opposite_vertex || !basis_vec) {
        if (basis_vec) { basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0; }
        return;
    }
    
    if (triangle_area < 1e-15) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    double coeff = edge_length / (2.0 * triangle_area);
    
    if (is_plus_triangle) {
        // f_n(r) = (l_n / (2*A_n+)) * (r - r_n+)
        basis_vec[0] = coeff * (r_point[0] - r_opposite_vertex[0]);
        basis_vec[1] = coeff * (r_point[1] - r_opposite_vertex[1]);
        basis_vec[2] = coeff * (r_point[2] - r_opposite_vertex[2]);
    } else {
        // f_n(r) = (l_n / (2*A_n-)) * (r_n- - r)
        basis_vec[0] = coeff * (r_opposite_vertex[0] - r_point[0]);
        basis_vec[1] = coeff * (r_opposite_vertex[1] - r_point[1]);
        basis_vec[2] = coeff * (r_opposite_vertex[2] - r_point[2]);
    }
}

// Helper function: Compute RWG basis function divergence
// ∇·f_n = l_n / A_n+ (on plus triangle) or -l_n / A_n- (on minus triangle)
static double compute_rwg_basis_divergence(
    double edge_length,
    double triangle_area,
    int is_plus_triangle) {
    
    if (triangle_area < 1e-15) return 0.0;
    
    double div = edge_length / triangle_area;
    return is_plus_triangle ? div : -div;
}

/********************************************************************************
 * Note: Triangle geometry and vector operation functions are now in
 * integration_utils.c. They are included via integration_utils.h header.
 ********************************************************************************/

/********************************************************************************
 * RWG-RWG EFIE Integration
 * Z_ij^EFIE = jωμ ∫∫ f_i(r) · f_j(r') G(r,r') dS dS'
 *            - (1/jωε) ∫∫ (∇·f_i(r)) (∇'·f_j(r')) G(r,r') dS dS'
 ********************************************************************************/
complex_t integrate_rwg_rwg_efie(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const void* basis_i_ptr,
    const void* basis_j_ptr,
    double frequency) {
    
    if (!tri_i_plus || !tri_j_plus) return mc(0.0, 0.0);
    
    // Extract basis function parameters (simplified - assumes basis_ptr contains edge info)
    // In practice, basis_i_ptr and basis_j_ptr should contain edge_length, etc.
    // For now, we estimate from triangle areas
    double edge_length_i = sqrt(tri_i_plus->area) * 0.5;  // Approximation
    double edge_length_j = sqrt(tri_j_plus->area) * 0.5;  // Approximation
    
    double omega = 2.0 * M_PI * frequency;
    double k = omega / C0;
    double mu = MU0;
    double eps = EPS0;
    
    // Compute areas
    double area_i_plus = tri_i_plus->area;
    double area_i_minus = tri_i_minus ? tri_i_minus->area : 0.0;
    double area_j_plus = tri_j_plus->area;
    double area_j_minus = tri_j_minus ? tri_j_minus->area : 0.0;
    
    // Get centroids for opposite vertices (simplified)
    double r_i_opposite[3], r_j_opposite[3];
    get_opposite_vertex(tri_i_plus, 0, r_i_opposite);
    get_opposite_vertex(tri_j_plus, 0, r_j_opposite);
    
    // Gaussian quadrature for double integration
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    complex_t result = mc(0.0, 0.0);
    
    // First term: jωμ ∫∫ f_i(r) · f_j(r') G(r,r') dS dS'
    // Integrate over all triangle pairs: (i+, j+), (i+, j-), (i-, j+), (i-, j-)
    const triangle_element_t* tri_i_list[2] = {tri_i_plus, tri_i_minus};
    const triangle_element_t* tri_j_list[2] = {tri_j_plus, tri_j_minus};
    int is_plus_i[2] = {1, 0};
    int is_plus_j[2] = {1, 0};
    double area_i_list[2] = {area_i_plus, area_i_minus};
    double area_j_list[2] = {area_j_plus, area_j_minus};
    
    for (int idx_i = 0; idx_i < 2; idx_i++) {
        if (!tri_i_list[idx_i] || area_i_list[idx_i] < 1e-15) continue;
        
        for (int idx_j = 0; idx_j < 2; idx_j++) {
            if (!tri_j_list[idx_j] || area_j_list[idx_j] < 1e-15) continue;
            
            const triangle_element_t* tri_i = tri_i_list[idx_i];
            const triangle_element_t* tri_j = tri_j_list[idx_j];
            
            // Double integral over triangles
            for (int i = 0; i < n_points; i++) {
                double u1 = 0.5 * (xi[i] + 1.0);
                double wu1 = wi[i];
                
                for (int j = 0; j < n_points; j++) {
                    double v1 = 0.5 * (xi[j] + 1.0);
                    double wv1 = wi[j];
                    
                    // Barycentric coordinates for first triangle
                    double w1 = 1.0 - u1 - v1;
                    if (w1 < 0.0) continue;
                    
                    double r1[3];
                    r1[0] = w1 * tri_i->vertices[0][0] + u1 * tri_i->vertices[1][0] + v1 * tri_i->vertices[2][0];
                    r1[1] = w1 * tri_i->vertices[0][1] + u1 * tri_i->vertices[1][1] + v1 * tri_i->vertices[2][1];
                    r1[2] = w1 * tri_i->vertices[0][2] + u1 * tri_i->vertices[1][2] + v1 * tri_i->vertices[2][2];
                    
                    // Compute basis function vector at r1
                    double f_i[3];
                    compute_rwg_basis_vector(tri_i, r1, r_i_opposite, edge_length_i, 
                                            area_i_list[idx_i], is_plus_i[idx_i], f_i);
                    
                    for (int k_idx = 0; k_idx < n_points; k_idx++) {
                        double u2 = 0.5 * (xi[k_idx] + 1.0);
                        double wu2 = wi[k_idx];
                        
                        for (int l = 0; l < n_points; l++) {
                            double v2 = 0.5 * (xi[l] + 1.0);
                            double wv2 = wi[l];
                            
                            // Barycentric coordinates for second triangle
                            double w2 = 1.0 - u2 - v2;
                            if (w2 < 0.0) continue;
                            
                            double r2[3];
                            r2[0] = w2 * tri_j->vertices[0][0] + u2 * tri_j->vertices[1][0] + v2 * tri_j->vertices[2][0];
                            r2[1] = w2 * tri_j->vertices[0][1] + u2 * tri_j->vertices[1][1] + v2 * tri_j->vertices[2][1];
                            r2[2] = w2 * tri_j->vertices[0][2] + u2 * tri_j->vertices[1][2] + v2 * tri_j->vertices[2][2];
                            
                            // Compute basis function vector at r2
                            double f_j[3];
                            compute_rwg_basis_vector(tri_j, r2, r_j_opposite, edge_length_j,
                                                    area_j_list[idx_j], is_plus_j[idx_j], f_j);
                            
                            // Compute distance and Green's function
                            double r = compute_distance_3d(r1, r2);
                            if (r < 1e-6) {
                                double char_length = sqrt(tri_i->area);
                                r = char_length * 0.1;
                            }
                            
                            complex_t G = green_function_free_space(r, k);
                            
                            // Dot product of basis functions
                            double f_dot = vector3d_dot_array(f_i, f_j);
                            
                            // Jacobian for triangles (2 * area for unit triangle)
                            double jacobian = 2.0 * area_i_list[idx_i] * 2.0 * area_j_list[idx_j];
                            
                            // Accumulate: jωμ * f_i · f_j * G
                            complex_t term;
                            term.re = -omega * mu * f_dot * G.im;  // jωμ = j * omega * mu
                            term.im = omega * mu * f_dot * G.re;
                            
                            complex_t scaled_term = complex_scalar_multiply(&term, wu1 * wv1 * wu2 * wv2 * jacobian);
                            result = complex_add(&result, &scaled_term);
                        }
                    }
                }
            }
        }
    }
    
    // Second term: -(1/jωε) ∫∫ (∇·f_i) (∇'·f_j) G(r,r') dS dS'
    complex_t result2 = mc(0.0, 0.0);
    
    for (int idx_i = 0; idx_i < 2; idx_i++) {
        if (!tri_i_list[idx_i] || area_i_list[idx_i] < 1e-15) continue;
        
        double div_i = compute_rwg_basis_divergence(edge_length_i, area_i_list[idx_i], is_plus_i[idx_i]);
        
        for (int idx_j = 0; idx_j < 2; idx_j++) {
            if (!tri_j_list[idx_j] || area_j_list[idx_j] < 1e-15) continue;
            
            double div_j = compute_rwg_basis_divergence(edge_length_j, area_j_list[idx_j], is_plus_j[idx_j]);
            
            const triangle_element_t* tri_i = tri_i_list[idx_i];
            const triangle_element_t* tri_j = tri_j_list[idx_j];
            
            // Double integral
            for (int i = 0; i < n_points; i++) {
                double u1 = 0.5 * (xi[i] + 1.0);
                double wu1 = wi[i];
                
                for (int j = 0; j < n_points; j++) {
                    double v1 = 0.5 * (xi[j] + 1.0);
                    double wv1 = wi[j];
                    
                    double w1 = 1.0 - u1 - v1;
                    if (w1 < 0.0) continue;
                    
                    double r1[3];
                    r1[0] = w1 * tri_i->vertices[0][0] + u1 * tri_i->vertices[1][0] + v1 * tri_i->vertices[2][0];
                    r1[1] = w1 * tri_i->vertices[0][1] + u1 * tri_i->vertices[1][1] + v1 * tri_i->vertices[2][1];
                    r1[2] = w1 * tri_i->vertices[0][2] + u1 * tri_i->vertices[1][2] + v1 * tri_i->vertices[2][2];
                    
                    for (int k_idx = 0; k_idx < n_points; k_idx++) {
                        double u2 = 0.5 * (xi[k_idx] + 1.0);
                        double wu2 = wi[k_idx];
                        
                        for (int l = 0; l < n_points; l++) {
                            double v2 = 0.5 * (xi[l] + 1.0);
                            double wv2 = wi[l];
                            
                            double w2 = 1.0 - u2 - v2;
                            if (w2 < 0.0) continue;
                            
                            double r2[3];
                            r2[0] = w2 * tri_j->vertices[0][0] + u2 * tri_j->vertices[1][0] + v2 * tri_j->vertices[2][0];
                            r2[1] = w2 * tri_j->vertices[0][1] + u2 * tri_j->vertices[1][1] + v2 * tri_j->vertices[2][1];
                            r2[2] = w2 * tri_j->vertices[0][2] + u2 * tri_j->vertices[1][2] + v2 * tri_j->vertices[2][2];
                            
                            double r = compute_distance_3d(r1, r2);
                            if (r < 1e-6) {
                                double char_length = sqrt(tri_i->area);
                                r = char_length * 0.1;
                            }
                            
                            complex_t G = green_function_free_space(r, k);
                            
                            // -(1/jωε) = j/(ωε) = j * 1/(omega * eps)
                            double jacobian = 2.0 * area_i_list[idx_i] * 2.0 * area_j_list[idx_j];
                            double div_product = div_i * div_j;
                            
                            complex_t term;
                            term.re = -div_product * G.im / (omega * eps);  // j/(ωε) * G
                            term.im = div_product * G.re / (omega * eps);
                            
                            complex_t scaled_term = complex_scalar_multiply(&term, wu1 * wv1 * wu2 * wv2 * jacobian);
                            result2 = complex_add(&result2, &scaled_term);
                        }
                    }
                }
            }
        }
    }
    
    // Combine both terms
    result = complex_add(&result, &result2);
    
    return result;
}

/********************************************************************************
 * RWG-RWG MFIE Integration
 * Z_ij^MFIE = (1/2) ∫ f_i(r) · f_j(r) dS + ∫∫ f_i(r) · (∇G × f_j(r')) dS dS'
 ********************************************************************************/
complex_t integrate_rwg_rwg_mfie(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const void* basis_i_ptr,
    const void* basis_j_ptr,
    double frequency) {
    
    if (!tri_i_plus || !tri_j_plus) return mc(0.0, 0.0);
    
    double omega = 2.0 * M_PI * frequency;
    double k = omega / C0;
    
    // Estimate edge lengths
    double edge_length_i = sqrt(tri_i_plus->area) * 0.5;
    double edge_length_j = sqrt(tri_j_plus->area) * 0.5;
    
    double area_i_plus = tri_i_plus->area;
    double area_i_minus = tri_i_minus ? tri_i_minus->area : 0.0;
    double area_j_plus = tri_j_plus->area;
    double area_j_minus = tri_j_minus ? tri_j_minus->area : 0.0;
    
    double r_i_opposite[3], r_j_opposite[3];
    get_opposite_vertex(tri_i_plus, 0, r_i_opposite);
    get_opposite_vertex(tri_j_plus, 0, r_j_opposite);
    
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    complex_t result = mc(0.0, 0.0);
    
    // First term: (1/2) ∫ f_i(r) · f_j(r) dS (only when triangles overlap)
    // This is a self-term, simplified here
    bool triangles_overlap = (tri_i_plus == tri_j_plus) || (tri_i_plus == tri_j_minus) ||
                            (tri_i_minus == tri_j_plus) || (tri_i_minus == tri_j_minus);
    
    if (triangles_overlap) {
        // Simplified: integrate over overlapping region
        // In practice, this requires careful handling of overlapping triangles
        // For now, use a simplified approximation
        double overlap_area = fmin(area_i_plus, area_j_plus);
        double f_dot_approx = edge_length_i * edge_length_j / (4.0 * area_i_plus * area_j_plus);
        complex_t self_term = mc(0.5 * f_dot_approx * overlap_area, 0.0);
        result = complex_add(&result, &self_term);
    }
    
    // Second term: ∫∫ f_i(r) · (∇G × f_j(r')) dS dS'
    const triangle_element_t* tri_i_list[2] = {tri_i_plus, tri_i_minus};
    const triangle_element_t* tri_j_list[2] = {tri_j_plus, tri_j_minus};
    int is_plus_i[2] = {1, 0};
    int is_plus_j[2] = {1, 0};
    double area_i_list[2] = {area_i_plus, area_i_minus};
    double area_j_list[2] = {area_j_plus, area_j_minus};
    
    for (int idx_i = 0; idx_i < 2; idx_i++) {
        if (!tri_i_list[idx_i] || area_i_list[idx_i] < 1e-15) continue;
        
        for (int idx_j = 0; idx_j < 2; idx_j++) {
            if (!tri_j_list[idx_j] || area_j_list[idx_j] < 1e-15) continue;
            
            const triangle_element_t* tri_i = tri_i_list[idx_i];
            const triangle_element_t* tri_j = tri_j_list[idx_j];
            
            for (int i = 0; i < n_points; i++) {
                double u1 = 0.5 * (xi[i] + 1.0);
                double wu1 = wi[i];
                
                for (int j = 0; j < n_points; j++) {
                    double v1 = 0.5 * (xi[j] + 1.0);
                    double wv1 = wi[j];
                    
                    double w1 = 1.0 - u1 - v1;
                    if (w1 < 0.0) continue;
                    
                    double r1[3];
                    r1[0] = w1 * tri_i->vertices[0][0] + u1 * tri_i->vertices[1][0] + v1 * tri_i->vertices[2][0];
                    r1[1] = w1 * tri_i->vertices[0][1] + u1 * tri_i->vertices[1][1] + v1 * tri_i->vertices[2][1];
                    r1[2] = w1 * tri_i->vertices[0][2] + u1 * tri_i->vertices[1][2] + v1 * tri_i->vertices[2][2];
                    
                    double f_i[3];
                    compute_rwg_basis_vector(tri_i, r1, r_i_opposite, edge_length_i,
                                            area_i_list[idx_i], is_plus_i[idx_i], f_i);
                    
                    for (int k_idx = 0; k_idx < n_points; k_idx++) {
                        double u2 = 0.5 * (xi[k_idx] + 1.0);
                        double wu2 = wi[k_idx];
                        
                        for (int l = 0; l < n_points; l++) {
                            double v2 = 0.5 * (xi[l] + 1.0);
                            double wv2 = wi[l];
                            
                            double w2 = 1.0 - u2 - v2;
                            if (w2 < 0.0) continue;
                            
                            double r2[3];
                            r2[0] = w2 * tri_j->vertices[0][0] + u2 * tri_j->vertices[1][0] + v2 * tri_j->vertices[2][0];
                            r2[1] = w2 * tri_j->vertices[0][1] + u2 * tri_j->vertices[1][1] + v2 * tri_j->vertices[2][1];
                            r2[2] = w2 * tri_j->vertices[0][2] + u2 * tri_j->vertices[1][2] + v2 * tri_j->vertices[2][2];
                            
                            double f_j[3];
                            compute_rwg_basis_vector(tri_j, r2, r_j_opposite, edge_length_j,
                                                    area_j_list[idx_j], is_plus_j[idx_j], f_j);
                            
                            double r = compute_distance_3d(r1, r2);
                            if (r < 1e-6) {
                                double char_length = sqrt(tri_i->area);
                                r = char_length * 0.1;
                            }
                            
                            // Compute gradient of Green's function: ∇G
                            double r_vec[3] = {r2[0] - r1[0], r2[1] - r1[1], r2[2] - r1[2]};
                            double grad_G[3];
                            green_function_gradient_free_space(r, k, r_vec, grad_G);
                            
                            // Compute cross product: ∇G × f_j
                            double cross_prod[3];
                            vector3d_cross_array(grad_G, f_j, cross_prod);
                            
                            // Dot product: f_i · (∇G × f_j)
                            double f_dot_cross = vector3d_dot_array(f_i, cross_prod);
                            
                            complex_t G = green_function_free_space(r, k);
                            double jacobian = 2.0 * area_i_list[idx_i] * 2.0 * area_j_list[idx_j];
                            
                            // Accumulate
                            complex_t term = mc(f_dot_cross * G.re, f_dot_cross * G.im);
                            complex_t scaled_term = complex_scalar_multiply(&term, wu1 * wv1 * wu2 * wv2 * jacobian);
                            result = complex_add(&result, &scaled_term);
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

/********************************************************************************
 * RWG-RWG CFIE Integration
 * Z_ij^CFIE = α * Z_ij^EFIE + (1-α) * η * Z_ij^MFIE
 ********************************************************************************/
complex_t integrate_rwg_rwg_cfie(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const void* basis_i_ptr,
    const void* basis_j_ptr,
    double frequency,
    double alpha) {
    
    complex_t z_efie = integrate_rwg_rwg_efie(tri_i_plus, tri_i_minus, tri_j_plus, tri_j_minus,
                                             basis_i_ptr, basis_j_ptr, frequency);
    complex_t z_mfie = integrate_rwg_rwg_mfie(tri_i_plus, tri_i_minus, tri_j_plus, tri_j_minus,
                                             basis_i_ptr, basis_j_ptr, frequency);
    
    // η = sqrt(μ₀/ε₀) = Z0
    double eta = ETA0;
    
    // Scale MFIE term
    complex_t z_mfie_scaled = complex_scalar_multiply(&z_mfie, (1.0 - alpha) * eta);
    
    // Scale EFIE term
    complex_t z_efie_scaled = complex_scalar_multiply(&z_efie, alpha);
    
    // Combine
    return complex_add(&z_efie_scaled, &z_mfie_scaled);
}

/********************************************************************************
 * RWG Plane Wave Excitation
 * V_i = ∫ f_i(r) · E_inc(r) dS
 ********************************************************************************/
complex_t integrate_rwg_plane_wave(
    const triangle_element_t* tri_plus,
    const triangle_element_t* tri_minus,
    const void* basis_ptr,
    const double* E_inc,
    const double* k_vec,
    double frequency) {
    
    if (!tri_plus || !E_inc || !k_vec) return mc(0.0, 0.0);
    
    double omega = 2.0 * M_PI * frequency;
    double k = omega / C0;
    
    double edge_length = sqrt(tri_plus->area) * 0.5;
    double area_plus = tri_plus->area;
    double area_minus = tri_minus ? tri_minus->area : 0.0;
    
    double r_opposite[3];
    get_opposite_vertex(tri_plus, 0, r_opposite);
    
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    complex_t result = mc(0.0, 0.0);
    
    const triangle_element_t* tri_list[2] = {tri_plus, tri_minus};
    int is_plus[2] = {1, 0};
    double area_list[2] = {area_plus, area_minus};
    
    for (int idx = 0; idx < 2; idx++) {
        if (!tri_list[idx] || area_list[idx] < 1e-15) continue;
        
        const triangle_element_t* tri = tri_list[idx];
        
        for (int i = 0; i < n_points; i++) {
            double u = 0.5 * (xi[i] + 1.0);
            double wu = wi[i];
            
            for (int j = 0; j < n_points; j++) {
                double v = 0.5 * (xi[j] + 1.0);
                double wv = wi[j];
                
                double w = 1.0 - u - v;
                if (w < 0.0) continue;
                
                double r[3];
                r[0] = w * tri->vertices[0][0] + u * tri->vertices[1][0] + v * tri->vertices[2][0];
                r[1] = w * tri->vertices[0][1] + u * tri->vertices[1][1] + v * tri->vertices[2][1];
                r[2] = w * tri->vertices[0][2] + u * tri->vertices[1][2] + v * tri->vertices[2][2];
                
                double f[3];
                compute_rwg_basis_vector(tri, r, r_opposite, edge_length,
                                        area_list[idx], is_plus[idx], f);
                
                // Plane wave: E_inc(r) = E_0 * exp(-j k_vec · r)
                double k_dot_r = k_vec[0]*r[0] + k_vec[1]*r[1] + k_vec[2]*r[2];
                complex_t phase = mc(cos(-k * k_dot_r), sin(-k * k_dot_r));
                
                // E_inc at this point
                complex_t E_inc_r[3];
                E_inc_r[0] = complex_multiply_real(&phase, E_inc[0]);
                E_inc_r[1] = complex_multiply_real(&phase, E_inc[1]);
                E_inc_r[2] = complex_multiply_real(&phase, E_inc[2]);
                
                // Dot product: f · E_inc
                complex_t f_dot_E;
                f_dot_E.re = f[0]*E_inc_r[0].re + f[1]*E_inc_r[1].re + f[2]*E_inc_r[2].re;
                f_dot_E.im = f[0]*E_inc_r[0].im + f[1]*E_inc_r[1].im + f[2]*E_inc_r[2].im;
                
                double jacobian = 2.0 * area_list[idx];
                complex_t scaled_term = complex_scalar_multiply(&f_dot_E, wu * wv * jacobian);
                result = complex_add(&result, &scaled_term);
            }
        }
    }
    
    return result;
}

/********************************************************************************
 * RWG Point Source Excitation
 * V_i = f_i(r_s) · J_s
 ********************************************************************************/
complex_t integrate_rwg_point_source(
    const triangle_element_t* tri_plus,
    const triangle_element_t* tri_minus,
    const void* basis_ptr,
    const double* r_source,
    const double* J_source) {
    
    if (!tri_plus || !r_source || !J_source) return mc(0.0, 0.0);
    
    double edge_length = sqrt(tri_plus->area) * 0.5;
    double area_plus = tri_plus->area;
    double area_minus = tri_minus ? tri_minus->area : 0.0;
    
    // Check if source is on positive or negative triangle
    // Simplified: use centroid to determine
    double centroid_plus[3], centroid_minus[3];
    get_triangle_centroid(tri_plus, centroid_plus);
    if (tri_minus) {
        get_triangle_centroid(tri_minus, centroid_minus);
    }
    
    double dist_plus = compute_distance_3d(r_source, centroid_plus);
    double dist_minus = tri_minus ? compute_distance_3d(r_source, centroid_minus) : 1e10;
    
    const triangle_element_t* tri = (dist_plus < dist_minus) ? tri_plus : tri_minus;
    int is_plus = (dist_plus < dist_minus) ? 1 : 0;
    double area = (dist_plus < dist_minus) ? area_plus : area_minus;
    
    if (!tri || area < 1e-15) return mc(0.0, 0.0);
    
    double r_opposite[3];
    get_opposite_vertex(tri, 0, r_opposite);
    
    // Evaluate basis function at source point
    double f[3];
    compute_rwg_basis_vector(tri, r_source, r_opposite, edge_length, area, is_plus, f);
    
    // Dot product: f_i(r_s) · J_s
    complex_t result;
    result.re = f[0]*J_source[0] + f[1]*J_source[1] + f[2]*J_source[2];
    result.im = 0.0;  // Assuming J_source is real
    
    return result;
}

/********************************************************************************
 * Point-to-Surface Integral (for PEEC point-surface coupling)
 * Computes ∫ G(r, r') dS' where r is a point and S' is a surface
 ********************************************************************************/
complex_t integrate_point_surface(
    const double *point,
    const triangle_element_t *surface,
    double k,
    integral_kernel_t kernel_type) {
    
    if (!point || !surface) return mc(0.0, 0.0);
    
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    complex_t result = mc(0.0, 0.0);
    
    // Surface integral over triangle
    for (int i = 0; i < n_points; i++) {
        double u = 0.5 * (xi[i] + 1.0);
        double wu = wi[i];
        
        for (int j = 0; j < n_points; j++) {
            double v = 0.5 * (xi[j] + 1.0);
            double wv = wi[j];
            
            double w = 1.0 - u - v;
            if (w < 0.0) continue;
            
            // Barycentric interpolation
            double r_surf[3];
            r_surf[0] = w * surface->vertices[0][0] + u * surface->vertices[1][0] + v * surface->vertices[2][0];
            r_surf[1] = w * surface->vertices[0][1] + u * surface->vertices[1][1] + v * surface->vertices[2][1];
            r_surf[2] = w * surface->vertices[0][2] + u * surface->vertices[1][2] + v * surface->vertices[2][2];
            
            double r = compute_distance_3d(point, r_surf);
            if (r < 1e-6) {
                double char_length = sqrt(surface->area);
                r = char_length * 0.1;
            }
            
            complex_t G = green_function_free_space(r, k);
            
            // Jacobian for triangle (2 * area)
            double jacobian = 2.0 * surface->area;
            
            complex_t term = mc(G.re, G.im);
            complex_t scaled_term = complex_scalar_multiply(&term, wu * wv * jacobian);
            result = complex_add(&result, &scaled_term);
        }
    }
    
    return result;
}

/********************************************************************************
 * Point-to-Wire Integral (for PEEC point-wire coupling)
 * Computes ∫ G(r, r') dl' where r is a point and l' is a wire
 ********************************************************************************/
complex_t integrate_point_wire(
    const double *point,
    const wire_element_t *wire,
    double k,
    integral_kernel_t kernel_type) {
    
    if (!point || !wire) return mc(0.0, 0.0);
    
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    complex_t result = mc(0.0, 0.0);
    
    // Line integral over wire
    for (int i = 0; i < n_points; i++) {
        double t = 0.5 * (xi[i] + 1.0);  // Map to [0, 1]
        double wt = wi[i];
        
        // Interpolate point on wire
        double r_wire[3];
        r_wire[0] = wire->start[0] + t * (wire->end[0] - wire->start[0]);
        r_wire[1] = wire->start[1] + t * (wire->end[1] - wire->start[1]);
        r_wire[2] = wire->start[2] + t * (wire->end[2] - wire->start[2]);
        
        double r = compute_distance_3d(point, r_wire);
        if (r < 1e-6) {
            r = wire->radius > 0 ? wire->radius : 1e-6;
        }
        
        complex_t G = green_function_free_space(r, k);
        
        // Jacobian for line integral (wire length)
        double jacobian = wire->length;
        
        complex_t term = mc(G.re, G.im);
        complex_t scaled_term = complex_scalar_multiply(&term, wt * jacobian);
        result = complex_add(&result, &scaled_term);
    }
    
    return result;
}

/********************************************************************************
 * Wire-to-Surface Integral (for PEEC wire-surface coupling)
 * Computes ∫∫ G(r, r') dl dS' where l is a wire and S' is a surface
 ********************************************************************************/
complex_t integrate_wire_surface(
    const wire_element_t *wire,
    const triangle_element_t *surface,
    double k,
    integral_kernel_t kernel_type) {
    
    if (!wire || !surface) return mc(0.0, 0.0);
    
    int n_points = 4;
    double xi[8], wi[8];
    gauss_quadrature_1d(n_points, xi, wi);
    
    complex_t result = mc(0.0, 0.0);
    
    // Double integral: wire × surface
    for (int i = 0; i < n_points; i++) {
        double t = 0.5 * (xi[i] + 1.0);  // Wire parameter [0, 1]
        double wt = wi[i];
        
        // Point on wire
        double r_wire[3];
        r_wire[0] = wire->start[0] + t * (wire->end[0] - wire->start[0]);
        r_wire[1] = wire->start[1] + t * (wire->end[1] - wire->start[1]);
        r_wire[2] = wire->start[2] + t * (wire->end[2] - wire->start[2]);
        
        for (int j = 0; j < n_points; j++) {
            double u = 0.5 * (xi[j] + 1.0);
            double wu = wi[j];
            
            for (int k_idx = 0; k_idx < n_points; k_idx++) {
                double v = 0.5 * (xi[k_idx] + 1.0);
                double wv = wi[k_idx];
                
                double w = 1.0 - u - v;
                if (w < 0.0) continue;
                
                // Point on surface
                double r_surf[3];
                r_surf[0] = w * surface->vertices[0][0] + u * surface->vertices[1][0] + v * surface->vertices[2][0];
                r_surf[1] = w * surface->vertices[0][1] + u * surface->vertices[1][1] + v * surface->vertices[2][1];
                r_surf[2] = w * surface->vertices[0][2] + u * surface->vertices[1][2] + v * surface->vertices[2][2];
                
                double r = compute_distance_3d(r_wire, r_surf);
                if (r < 1e-6) {
                    double char_length = sqrt(surface->area);
                    r = char_length * 0.1;
                }
                
                complex_t G = green_function_free_space(r, k);
                
                // Jacobian: wire length × triangle area
                double jacobian = wire->length * 2.0 * surface->area;
                
                complex_t term = mc(G.re, G.im);
                complex_t scaled_term = complex_scalar_multiply(&term, wt * wu * wv * jacobian);
                result = complex_add(&result, &scaled_term);
            }
        }
    }
    
    return result;
}
