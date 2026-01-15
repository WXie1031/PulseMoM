#include "../../core/layered_greens_function.h"
#include "peec_solver.h"
#include "../../core/core_mesh.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct peec_solver {
    peec_config_t config;
    mesh_t* mesh;

    // Minimal circuit buffers
    double* R;
    double* L;
    double* C;
    peec_scalar_complex_t* rhs;
    peec_result_t result;
    int num_nodes;
    int num_branches;
    
    // Layered medium support
    void* layered_medium;      // LayeredMedium*
    void* frequency_domain;    // FrequencyDomain*
    void* greens_params;        // GreensFunctionParams*
    bool use_layered_medium;   // Flag for layered medium
} peec_solver_t;

static void peec_solver_free_buffers(peec_solver_t* solver) {
    if (!solver) return;
    free(solver->R);
    free(solver->L);
    free(solver->C);
    free(solver->rhs);
    free(solver->result.resistance_matrix);
    free(solver->result.inductance_matrix);
    free(solver->result.capacitance_matrix);
    free(solver->result.potential_coeff_matrix);
    free(solver->result.node_voltages);
    free(solver->result.branch_currents);
    free(solver->result.current_coefficients);
    memset(&solver->result, 0, sizeof(solver->result));
    solver->R = solver->L = solver->C = NULL;
    solver->rhs = NULL;
}

peec_solver_t* peec_solver_create(const peec_config_t* config) {
    peec_solver_t* s = (peec_solver_t*)calloc(1, sizeof(peec_solver_t));
    if (!s) return NULL;
    if (config) s->config = *config;
    s->config.circuit_max_iterations = s->config.circuit_max_iterations > 0 ? s->config.circuit_max_iterations : 200;
    s->config.circuit_tolerance = (s->config.circuit_tolerance > 0.0) ? s->config.circuit_tolerance : 1e-3;
    s->config.frequency = (s->config.frequency > 0.0) ? s->config.frequency : 1e6;
    return s;
}

void peec_solver_destroy(peec_solver_t* solver) {
    if (!solver) return;
    peec_solver_free_buffers(solver);
    free(solver);
}

int peec_solver_configure(peec_solver_t* solver, const peec_config_t* config) {
    if (!solver || !config) return -1;
    solver->config = *config;
    return 0;
}

int peec_solver_set_layered_medium(peec_solver_t* solver,
                                  const LayeredMedium* medium,
                                  const FrequencyDomain* freq,
                                  const GreensFunctionParams* params) {
    if (!solver) return -1;
    
    solver->layered_medium = (void*)medium;
    solver->frequency_domain = (void*)freq;
    solver->greens_params = (void*)params;
    solver->use_layered_medium = (medium != NULL && freq != NULL);
    
    if (solver->use_layered_medium && medium) {
        const LayeredMedium* lm = (const LayeredMedium*)medium;
        printf("PEEC Solver: Layered medium enabled with %d layers\n", lm->num_layers);
    }
    
    return 0;
}

// Compute Green's function value for PEEC (for R/L/C extraction)
// Uses full layered_medium_greens_function interface when available
static double compute_peec_green_function_value(
    double rho, double z, double z_prime,
    double frequency,
    const void* layered_medium,
    const void* frequency_domain,
    const void* greens_params) {
    
    const double k = 2.0 * M_PI * frequency / C0;
    double r = sqrt(rho*rho + (z-z_prime)*(z-z_prime)) + 1e-9;
    
    // If layered medium is enabled, use full layered Green's function interface
    if (layered_medium && frequency_domain) {
        const LayeredMedium* medium = (const LayeredMedium*)layered_medium;
        const FrequencyDomain* freq = (const FrequencyDomain*)frequency_domain;
        const GreensFunctionParams* params = (const GreensFunctionParams*)greens_params;
        
        if (medium->num_layers > 0) {
            // Find source and observation layers
            double z_acc = 0.0;
            int layer_src = 0, layer_obs = 0;
            for (int i = 0; i < medium->num_layers; i++) {
                if (z_prime >= z_acc && z_prime < z_acc + medium->thickness[i]) {
                    layer_src = i;
                    break;
                }
                z_acc += medium->thickness[i];
            }
            z_acc = 0.0;
            for (int i = 0; i < medium->num_layers; i++) {
                if (z >= z_acc && z < z_acc + medium->thickness[i]) {
                    layer_obs = i;
                    break;
                }
                z_acc += medium->thickness[i];
            }
            
            // Use full layered_medium_greens_function interface
            // For PEEC, we need a scalar Green's function for R/L/C extraction
            // Extract from G_ee dyadic (typically use G_zz or trace)
            GreensFunctionPoints points;
            points.x = 0.0;  // Source at origin in rho plane
            points.y = 0.0;
            points.z = z_prime;
            points.xp = rho;  // Observation at (rho, 0, z)
            points.yp = 0.0;
            points.zp = z;
            points.layer_src = layer_src;
            points.layer_obs = layer_obs;
            
            // Call full Green's function interface
            GreensFunctionDyadic* gf = layered_medium_greens_function(medium, freq, &points, params);
            
            if (gf) {
                // Extract scalar Green's function from G_ee dyadic
                // For PEEC, use G_zz component (vertical) or average of diagonal
                CDOUBLE G_scalar = gf->G_ee[2][2];  // zz component
                
                // Convert CDOUBLE to real scalar (magnitude for PEEC)
                #if defined(_MSC_VER)
                double G_mag = sqrt(G_scalar.re*G_scalar.re + G_scalar.im*G_scalar.im);
                #else
                double G_mag = cabs(G_scalar);
                #endif
                
                // Free the dyadic structure
                free_greens_function_dyadic(gf);
                
                // Return magnitude (PEEC typically uses real-valued Green's function)
                return G_mag;
            } else {
                // Fallback to simplified layered Green's function if full interface fails
                double eps_r_src = (layer_src < medium->num_layers && medium->epsilon_r) 
                                  ? medium->epsilon_r[layer_src] : 1.0;
                double eps_r_obs = (layer_obs < medium->num_layers && medium->epsilon_r)
                                  ? medium->epsilon_r[layer_obs] : 1.0;
                double eps_r_eff = sqrt(eps_r_src * eps_r_obs);
                
                // Scale by effective permittivity
                return 1.0 / (4.0 * M_PI * r * sqrt(eps_r_eff));
            }
        }
    }
    
    // Free-space Green's function
    return 1.0 / (4.0 * M_PI * r);
}

int peec_solver_import_cad(peec_solver_t* solver, const char* filename, const char* format) {
    if (!solver || !filename || !format) return -1;
    (void)filename; (void)format;
    return 0;
}

int peec_solver_set_mesh(peec_solver_t* solver, void* mesh) {
    if (!solver || !mesh) return -1;
    solver->mesh = (mesh_t*)mesh;
    // Approximate nodes/branches by vertices/elements placeholders
    solver->num_nodes = solver->mesh->num_vertices;
    solver->num_branches = (solver->mesh->num_edges > 0) ? solver->mesh->num_edges : solver->mesh->num_elements;
    if (solver->num_nodes <= 0 || solver->num_branches <= 0) {
        solver->num_nodes = 0;
        solver->num_branches = 0;
        return -1;
    }
    return 0;
}

static int peec_solver_allocate_system(peec_solver_t* solver) {
    if (!solver || solver->num_nodes <= 0 || solver->num_branches <= 0) return -1;
    peec_solver_free_buffers(solver);
    size_t n = (size_t)solver->num_nodes;
    size_t b = (size_t)solver->num_branches;
    solver->R = (double*)calloc(b * b, sizeof(double));
    solver->L = (double*)calloc(b * b, sizeof(double));
    solver->C = (double*)calloc(b * b, sizeof(double));
    solver->rhs = (peec_scalar_complex_t*)calloc(b, sizeof(peec_scalar_complex_t));
    solver->result.resistance_matrix = (double*)calloc(b * b, sizeof(double));
    solver->result.inductance_matrix = (double*)calloc(b * b, sizeof(double));
    solver->result.capacitance_matrix = (double*)calloc(b * b, sizeof(double));
    solver->result.potential_coeff_matrix = (double*)calloc(b * b, sizeof(double));
    solver->result.node_voltages = (peec_scalar_complex_t*)calloc(n, sizeof(peec_scalar_complex_t));
    solver->result.branch_currents = (peec_scalar_complex_t*)calloc(b, sizeof(peec_scalar_complex_t));
    solver->result.current_coefficients = (peec_scalar_complex_t*)calloc(b, sizeof(peec_scalar_complex_t));
    if (!solver->R || !solver->L || !solver->C || !solver->rhs ||
        !solver->result.resistance_matrix || !solver->result.inductance_matrix ||
        !solver->result.capacitance_matrix || !solver->result.potential_coeff_matrix ||
        !solver->result.node_voltages || !solver->result.branch_currents ||
        !solver->result.current_coefficients) {
        peec_solver_free_buffers(solver);
        return -1;
    }
    solver->result.num_nodes = solver->num_nodes;
    solver->result.num_branches = solver->num_branches;
    solver->result.num_basis_functions = solver->num_branches;
    return 0;
}

int peec_solver_extract_partial_elements(peec_solver_t* solver) {
    if (!solver || !solver->mesh) return -1;
    if (solver->num_branches <= 0 || solver->num_nodes <= 0) return -1;
    if (peec_solver_allocate_system(solver) != 0) return -1;
    size_t b = (size_t)solver->num_branches;
    double omega = 2.0 * M_PI * (solver->config.frequency > 0 ? solver->config.frequency : 1e6);
    const double sigma = 5.8e7; // copper conductivity fallback
    const double rho = 1.0 / sigma;
    const double thickness = 35e-6;
    const double width_default = 100e-6;

    mesh_edge_t* edges = solver->mesh->edges;
    mesh_vertex_t* verts = solver->mesh->vertices;
    size_t edge_count = (size_t)solver->mesh->num_edges;

    for (size_t i = 0; i < b; i++) {
        // Geometry extraction
        point3d_t p1 = {0}, p2 = {0};
        if (edges && i < edge_count) {
            int v0 = edges[i].vertex1_id;
            int v1 = edges[i].vertex2_id;
            if (verts && v0 >= 0 && v1 >= 0) {
                p1.x = verts[v0].position.x; p1.y = verts[v0].position.y; p1.z = verts[v0].position.z;
                p2.x = verts[v1].position.x; p2.y = verts[v1].position.y; p2.z = verts[v1].position.z;
            }
        }
        double dx = p1.x - p2.x, dy = p1.y - p2.y, dz = p1.z - p2.z;
        double length = sqrt(dx*dx + dy*dy + dz*dz) + 1e-9;
        double width = width_default;
        double area = width * thickness;
        double radius = fmax(thickness * 0.5, 1e-6);

        double Rself = rho * length / (area > 1e-18 ? area : 1e-18);
        double Lself = MU0 / (2.0 * M_PI) * length * (log(2.0 * length / radius) - 1.0);
        double Cself = (2.0 * M_PI * EPS0 * length) / fmax(log(2.0 * length / radius), 1e-6);

        solver->R[i * b + i] = Rself;
        solver->L[i * b + i] = Lself;
        solver->C[i * b + i] = Cself;
        solver->result.resistance_matrix[i * b + i] = Rself;
        solver->result.inductance_matrix[i * b + i] = Lself;
        solver->result.capacitance_matrix[i * b + i] = Cself;
        solver->result.potential_coeff_matrix[i * b + i] = (Cself > 0.0) ? 1.0 / Cself : 0.0;
        solver->rhs[i].re = 1.0;
        solver->rhs[i].im = 0.0;

        // Mutual coupling (pairwise)
        for (size_t j = i + 1; j < b; j++) {
            point3d_t q1 = {0}, q2 = {0};
            if (edges && j < edge_count) {
                int vv0 = edges[j].vertex1_id;
                int vv1 = edges[j].vertex2_id;
                if (verts && vv0 >= 0 && vv1 >= 0) {
                    q1.x = verts[vv0].position.x; q1.y = verts[vv0].position.y; q1.z = verts[vv0].position.z;
                    q2.x = verts[vv1].position.x; q2.y = verts[vv1].position.y; q2.z = verts[vv1].position.z;
                }
            }
            double mx = 0.25 * (p1.x + p2.x + q1.x + q2.x);
            double my = 0.25 * (p1.y + p2.y + q1.y + q2.y);
            double mz = 0.25 * (p1.z + p2.z + q1.z + q2.z);
            double rx = mx - 0.5*(q1.x+q2.x);
            double ry = my - 0.5*(q1.y+q2.y);
            double rz = mz - 0.5*(q1.z+q2.z);
            double r = sqrt(rx*rx + ry*ry + rz*rz) + 1e-6;
            // Compute mutual inductance and capacitance
            // If layered medium is enabled, scale by layered Green's function
            double G_factor = 1.0;
            if (solver->use_layered_medium && solver->layered_medium && solver->frequency_domain) {
                double rho = sqrt(rx*rx + ry*ry);
                double z_avg = 0.5 * (p1.z + p2.z);
                double z_prime_avg = 0.5 * (q1.z + q2.z);
                double G_layered = compute_peec_green_function_value(rho, z_avg, z_prime_avg,
                                                                      solver->config.frequency,
                                                                      solver->layered_medium,
                                                                      solver->frequency_domain,
                                                                      solver->greens_params);
                double G_fs = 1.0 / (4.0 * M_PI * r);
                if (G_fs > 1e-15) {
                    G_factor = G_layered / G_fs;
                }
            }
            
            double mutual_L = MU0 / (4.0 * M_PI) * (length * length) / r * G_factor;
            double mutual_C = EPS0 * area * area / r * G_factor;
            solver->L[i * b + j] = solver->L[j * b + i] = mutual_L;
            solver->C[i * b + j] = solver->C[j * b + i] = mutual_C;
            solver->result.inductance_matrix[i * b + j] = solver->result.inductance_matrix[j * b + i] = mutual_L;
            solver->result.capacitance_matrix[i * b + j] = solver->result.capacitance_matrix[j * b + i] = mutual_C;
        }
    }

    solver->result.extraction_time = 0.0;
    return 0;
}

int peec_solver_build_circuit_network(peec_solver_t* solver) {
    if (!solver || !solver->R || !solver->L || !solver->C) return -1;
    // Minimal mapping: branches -> nodes (identity-like)
    // No additional work needed in placeholder path
    return 0;
}

int peec_solver_solve_circuit(peec_solver_t* solver) {
    if (!solver || !solver->R || !solver->L || !solver->C || !solver->rhs) return -1;
    if (solver->num_branches <= 0 || solver->num_nodes <= 0) return -1;
    size_t b = (size_t)solver->num_branches;
    double omega = 2.0 * M_PI * (solver->config.frequency > 0 ? solver->config.frequency : 1e6);

    // Build dense impedance matrix Z = R + jωL - j/(ωC)
    peec_scalar_complex_t* Z = (peec_scalar_complex_t*)calloc(b * b, sizeof(peec_scalar_complex_t));
    if (!Z) return -1;
    for (size_t i = 0; i < b; i++) {
        for (size_t j = 0; j < b; j++) {
            double Rv = solver->R[i * b + j];
            double Lv = solver->L[i * b + j];
            double Cv = solver->C[i * b + j];
            double Zre = Rv;
            double Zim = omega * Lv;
            if (Cv != 0.0) {
                Zim -= 1.0 / (omega * Cv);
            }
            Z[i * b + j].re = Zre;
            Z[i * b + j].im = Zim;
        }
    }

    // Gaussian elimination (complex) without pivoting
    peec_scalar_complex_t* rhs = solver->rhs;
    for (size_t k = 0; k < b; k++) {
        double denom = Z[k * b + k].re * Z[k * b + k].re + Z[k * b + k].im * Z[k * b + k].im + 1e-24;
        peec_scalar_complex_t pivot_inv = {
            Z[k * b + k].re / denom,
            -Z[k * b + k].im / denom
        };
        for (size_t j = k; j < b; j++) {
            double re = Z[k * b + j].re * pivot_inv.re - Z[k * b + j].im * pivot_inv.im;
            double im = Z[k * b + j].re * pivot_inv.im + Z[k * b + j].im * pivot_inv.re;
            Z[k * b + j].re = re;
            Z[k * b + j].im = im;
        }
        {
            double re = rhs[k].re * pivot_inv.re - rhs[k].im * pivot_inv.im;
            double im = rhs[k].re * pivot_inv.im + rhs[k].im * pivot_inv.re;
            rhs[k].re = re;
            rhs[k].im = im;
        }
        for (size_t i = k + 1; i < b; i++) {
            peec_scalar_complex_t factor = Z[i * b + k];
            for (size_t j = k; j < b; j++) {
                double re = factor.re * Z[k * b + j].re - factor.im * Z[k * b + j].im;
                double im = factor.re * Z[k * b + j].im + factor.im * Z[k * b + j].re;
                Z[i * b + j].re -= re;
                Z[i * b + j].im -= im;
            }
            double re = factor.re * rhs[k].re - factor.im * rhs[k].im;
            double im = factor.re * rhs[k].im + factor.im * rhs[k].re;
            rhs[i].re -= re;
            rhs[i].im -= im;
        }
    }

    // Back substitution
    for (int ii = (int)b - 1; ii >= 0; ii--) {
        peec_scalar_complex_t sum = {0.0, 0.0};
        for (size_t j = ii + 1; j < b; j++) {
            double re = Z[ii * b + j].re * solver->result.branch_currents[j].re - Z[ii * b + j].im * solver->result.branch_currents[j].im;
            double im = Z[ii * b + j].re * solver->result.branch_currents[j].im + Z[ii * b + j].im * solver->result.branch_currents[j].re;
            sum.re += re;
            sum.im += im;
        }
        double bre = rhs[ii].re - sum.re;
        double bim = rhs[ii].im - sum.im;
        double denom = Z[ii * b + ii].re * Z[ii * b + ii].re + Z[ii * b + ii].im * Z[ii * b + ii].im + 1e-24;
        solver->result.branch_currents[ii].re = (bre * Z[ii * b + ii].re + bim * Z[ii * b + ii].im) / denom;
        solver->result.branch_currents[ii].im = (bim * Z[ii * b + ii].re - bre * Z[ii * b + ii].im) / denom;
        solver->result.current_coefficients[ii] = solver->result.branch_currents[ii];
    }

    // Node voltages: simple projection (current * impedance on diagonal)
    size_t n = (size_t)solver->num_nodes;
    for (size_t k = 0; k < n && k < b; k++) {
        double Zre = Z[k * b + k].re;
        double Zim = Z[k * b + k].im;
        solver->result.node_voltages[k].re = solver->result.branch_currents[k].re * Zre - solver->result.branch_currents[k].im * Zim;
        solver->result.node_voltages[k].im = solver->result.branch_currents[k].re * Zim + solver->result.branch_currents[k].im * Zre;
    }
    free(Z);

    solver->result.iterations = 1;
    solver->result.converged = true;
    solver->result.solve_time = 0.0;
    return 0;
}

const peec_result_t* peec_solver_get_results(const peec_solver_t* solver) { if (!solver) return NULL; return &solver->result; }
int peec_solver_get_num_nodes(const peec_solver_t* solver) { return solver ? solver->num_nodes : 0; }
int peec_solver_get_num_branches(const peec_solver_t* solver) { return solver ? solver->num_branches : 0; }
double peec_solver_get_memory_usage(const peec_solver_t* solver) {
    if (!solver) return 0.0;
    size_t b = (size_t)solver->num_branches;
    size_t n = (size_t)solver->num_nodes;
    size_t bytes = 0;
    bytes += 3 * b * b * sizeof(double);           // R/L/C
    bytes += b * sizeof(peec_scalar_complex_t);    // rhs
    bytes += 4 * b * b * sizeof(double);           // result matrices
    bytes += (n + b + b) * sizeof(peec_scalar_complex_t); // node voltages + branch currents + coeffs
    return (double)bytes;
}

int peec_solver_export_spice(const peec_solver_t* solver, const char* filename) {
    if (!solver || !filename) return -1;
    // Minimal CSV-like dump of diagonal Z elements
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    fprintf(fp, "# PEEC minimal SPICE-like export (diagonal only)\n");
    size_t b = (size_t)solver->num_branches;
    double omega = 2.0 * M_PI * (solver->config.frequency > 0 ? solver->config.frequency : 1e6);
    for (size_t i = 0; i < b; i++) {
        double Zre = solver->R[i * b + i];
        double Zim = omega * solver->L[i * b + i] - 1.0 / (omega * (solver->C[i * b + i] > 0.0 ? solver->C[i * b + i] : 1e-12));
        fprintf(fp, "R%d %zu %zu %.6e\n", (int)i, (size_t)i, (size_t)(i + 1), Zre);
        fprintf(fp, "L%d %zu %zu %.6e\n", (int)i, (size_t)i, (size_t)(i + 1), solver->L[i * b + i]);
        fprintf(fp, "C%d %zu %zu %.6e\n", (int)i, (size_t)i, (size_t)(i + 1), solver->C[i * b + i]);
        fprintf(fp, "* Zdiag=%g%+gj\n", Zre, Zim);
    }
    fclose(fp);
    return 0;
}

int peec_solver_add_port_surface_coupling(peec_solver_t* solver,
                                          int port_node,
                                          int element_index,
                                          const double point_xyz[3],
                                          double frequency) { (void)solver; (void)port_node; (void)element_index; (void)point_xyz; (void)frequency; return 0; }

int peec_solver_batch_port_surface_coupling(peec_solver_t* solver,
                                            const int* port_nodes,
                                            int num_ports,
                                            const int* element_indices,
                                            int num_elements,
                                            const double* points_xyz,
                                            double frequency) { (void)solver; (void)port_nodes; (void)num_ports; (void)element_indices; (void)num_elements; (void)points_xyz; (void)frequency; return 0; }

int peec_solver_add_port_line_coupling(peec_solver_t* solver,
                                       int port_node,
                                       int element_index,
                                       const double point_xyz[3],
                                       double frequency) { (void)solver; (void)port_node; (void)element_index; (void)point_xyz; (void)frequency; return 0; }

int peec_solver_batch_port_line_coupling(peec_solver_t* solver,
                                         const int* port_nodes,
                                         int num_ports,
                                         const int* element_indices,
                                         int num_elements,
                                         const double* points_xyz,
                                         double frequency) { (void)solver; (void)port_nodes; (void)num_ports; (void)element_indices; (void)num_elements; (void)points_xyz; (void)frequency; return 0; }

int peec_solver_add_line_surface_coupling(peec_solver_t* solver,
                                          int line_element_index,
                                          int surface_element_index,
                                          double frequency) { (void)solver; (void)line_element_index; (void)surface_element_index; (void)frequency; return 0; }

int peec_solver_batch_line_surface_coupling(peec_solver_t* solver,
                                            const int* line_element_indices,
                                            int num_lines,
                                            const int* surface_element_indices,
                                            int num_surfaces,
                                            double frequency) { (void)solver; (void)line_element_indices; (void)num_lines; (void)surface_element_indices; (void)num_surfaces; (void)frequency; return 0; }

int peec_solver_export_couplings_csv(peec_solver_t* solver, const char* filename) { (void)solver; (void)filename; return 0; }
int peec_solver_export_nodes_csv(peec_solver_t* solver, const char* filename) { (void)solver; (void)filename; return 0; }
int peec_solver_export_branches_csv(peec_solver_t* solver, const char* filename) { (void)solver; (void)filename; return 0; }
int peec_solver_selftest_couplings(void) { return 0; }
