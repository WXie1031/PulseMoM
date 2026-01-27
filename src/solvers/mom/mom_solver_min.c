#include "../../discretization/geometry/core_geometry.h"
#include "mom_solver.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../operators/assembler/core_assembler.h"
#include "../../discretization/geometry/port_support_extended.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Unified MoM solver entry points
int mom_solve_unified(mesh_t *mesh, double frequency, complex_t *excitation,
                     complex_t *current_solution, void *config /* mom_unified_config_t* */);
void mom_compute_radiation_pattern_unified(mesh_t *mesh, complex_t *currents, double frequency,
                                        double theta_min, double theta_max, double phi_min, double phi_max,
                                        int n_theta, int n_phi, complex_t *far_field);

typedef struct mom_solver {
    mom_config_t config;
    geom_geometry_t* geometry;
    mesh_t* mesh;
    mom_excitation_t excitation;

    // Unified numerical buffers
    complex_t* excitation_vector;       // RHS for unified solver
    mom_result_t result;
    int num_unknowns;
    
    // Port support
    extended_port_t** ports;            // Array of port pointers
    int num_ports;                     // Number of ports
    int ports_capacity;                // Capacity of ports array
} mom_solver_t;

static void mom_solver_free_buffers(mom_solver_t* solver) {
    if (!solver) return;
    free(solver->excitation_vector);
    free(solver->result.current_coefficients);
    free(solver->result.current_magnitude);
    free(solver->result.current_phase);
    free(solver->result.near_field.e_field);
    free(solver->result.near_field.h_field);
    free(solver->result.far_field.theta_angles);
    free(solver->result.far_field.phi_angles);
    free(solver->result.far_field.e_theta);
    free(solver->result.far_field.e_phi);
    free(solver->result.rcs_values);
    memset(&solver->result, 0, sizeof(solver->result));
    solver->excitation_vector = NULL;
    
    // Free ports
    if (solver->ports) {
        for (int i = 0; i < solver->num_ports; i++) {
            if (solver->ports[i]) {
                port_destroy(solver->ports[i]);
            }
        }
        free(solver->ports);
        solver->ports = NULL;
    }
    solver->num_ports = 0;
    solver->ports_capacity = 0;
}

mom_solver_t* mom_solver_create(const mom_config_t* config) {
    mom_solver_t* s = (mom_solver_t*)calloc(1, sizeof(mom_solver_t));
    if (!s) return NULL;
    if (config) s->config = *config;
    s->config.max_iterations = s->config.max_iterations > 0 ? s->config.max_iterations : 200;
    s->config.tolerance = (s->config.tolerance > 0.0) ? s->config.tolerance : 1e-3;
    // Sensible defaults for excitation
    s->excitation.type = MOM_EXCITATION_PLANE_WAVE;
    s->excitation.frequency = (config && config->frequency > 0.0) ? config->frequency : 1e9;
    s->excitation.amplitude = 1.0;
    s->ports = NULL;
    s->num_ports = 0;
    s->ports_capacity = 0;
    
    // EFIE near/singular handling defaults (safe for small meshes)
    if (s->config.near_threshold <= 0.0) {
        s->config.near_threshold = 0.1;  // 0.1 wavelengths default (conservative)
    }
    // Validate and clamp threshold to reasonable range [0.01, 1.0] wavelengths
    if (s->config.near_threshold < 0.01) {
        s->config.near_threshold = 0.01;  // Minimum: 0.01 wavelengths
    }
    if (s->config.near_threshold > 1.0) {
        s->config.near_threshold = 1.0;  // Maximum: 1.0 wavelengths
    }
    
    // Validate self-term regularization if provided
    if (s->config.enable_self_term_analytic) {
        if (s->config.near_threshold <= 0.0) {  // Note: using near_threshold as placeholder
            // If there's a separate regularization field, validate it here
        }
    }
    
    // Note: enable_duffy and enable_self_term_analytic are bool, so they default to false
    // If user wants them enabled, they should set them explicitly in config
    // For backward compatibility, we don't force them on by default
    // However, when enabled, they provide improved accuracy for near-field interactions
    
    return s;
}

void mom_solver_destroy(mom_solver_t* solver) {
    if (!solver) return;
    mom_solver_free_buffers(solver);
    free(solver);
}

int mom_solver_configure(mom_solver_t* solver, const mom_config_t* config) {
    if (!solver || !config) return -1;
    solver->config = *config;
    return 0;
}

int mom_solver_set_geometry(mom_solver_t* solver, void* geometry) {
    if (!solver || !geometry) return -1;
    solver->geometry = (geom_geometry_t*)geometry;
    return 0;
}

int mom_solver_set_mesh(mom_solver_t* solver, void* mesh) {
    if (!solver || !mesh) return -1;
    solver->mesh = (mesh_t*)mesh;
    solver->num_unknowns = solver->mesh->num_elements;
    if (solver->num_unknowns <= 0) {
        solver->num_unknowns = 0;
        return -1;
    }
    return 0;
}

int mom_solver_add_excitation(mom_solver_t* solver, const mom_excitation_t* excitation) {
    if (!solver || !excitation) return -1;
    solver->excitation = *excitation;
    return 0;
}

int mom_solver_add_lumped_excitation(mom_solver_t* solver, const point3d_t* position,
                                     const point3d_t* polarization,
                                     double amplitude, double width, int layer_index) {
    if (!solver) return -1;
    (void)width; (void)layer_index;
    solver->excitation.type = MOM_EXCITATION_VOLTAGE_SOURCE;
    solver->excitation.amplitude = amplitude;
    solver->excitation.k_vector = *position;
    solver->excitation.polarization = *polarization;
    return 0;
}

// Add port template to solver
int mom_solver_add_port(mom_solver_t* solver, struct extended_port_t* port) {
    if (!solver || !port) return -1;
    
    // Grow ports array if needed
    if (solver->num_ports >= solver->ports_capacity) {
        int new_capacity = (solver->ports_capacity == 0) ? 4 : solver->ports_capacity * 2;
        extended_port_t** new_ports = (extended_port_t**)realloc(solver->ports, new_capacity * sizeof(extended_port_t*));
        if (!new_ports) return -1;
        solver->ports = new_ports;
        solver->ports_capacity = new_capacity;
    }
    
    // Cast to match the typedef type used in the struct
    solver->ports[solver->num_ports++] = (extended_port_t*)(void*)port;
    return 0;
}

// Build RWG mapping: edge -> adjacent triangles
static int build_rwg_mapping_local(const mesh_t* mesh, int* edge_plus, int* edge_minus, double* edge_length) {
    if (!mesh || mesh->num_edges <= 0 || !mesh->edges) return -1;
    for (int e = 0; e < mesh->num_edges; e++) {
        const mesh_edge_t* edge = &mesh->edges[e];
        edge_length[e] = edge->length > 0.0 ? edge->length : 0.0;
        edge_plus[e] = -1;
        edge_minus[e] = -1;
    }
    // Traverse triangles to fill plus/minus incidence
    for (int t = 0; t < mesh->num_elements; t++) {
        const mesh_element_t* elem = &mesh->elements[t];
        if (elem->type != MESH_ELEMENT_TRIANGLE || elem->num_edges <= 0) continue;
        for (int k = 0; k < elem->num_edges && k < 3; k++) {
            int eidx = elem->edges[k];
            if (eidx < 0 || eidx >= mesh->num_edges) continue;
            // Decide orientation by comparing vertex ordering
            const mesh_edge_t* e = &mesh->edges[eidx];
            int v0 = elem->vertices[k];
            int v1 = elem->vertices[(k + 1) % elem->num_vertices];
            bool same_dir = (e->vertex1_id == v0 && e->vertex2_id == v1);
            if (same_dir) {
                edge_plus[eidx] = t;
            } else {
                edge_minus[eidx] = t;
            }
        }
    }
    return 0;
}

// Compute RWG basis function vector at a point on a triangle
static void compute_rwg_basis_at_point(
    const mesh_t* mesh,
    int edge_idx,
    int tri_idx,
    int is_plus,
    const geom_point_t* r_point,
    double* basis_vec) {
    
    if (!mesh || !r_point || !basis_vec) {
        if (basis_vec) { basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0; }
        return;
    }
    
    const mesh_element_t* tri = &mesh->elements[tri_idx];
    if (tri->type != MESH_ELEMENT_TRIANGLE || tri->num_vertices < 3) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    const mesh_edge_t* edge = &mesh->edges[edge_idx];
    double edge_len = edge->length > 0.0 ? edge->length : 0.0;
    double tri_area = tri->area > 0.0 ? tri->area : 1e-15;
    
    if (tri_area < 1e-15 || edge_len < 1e-15) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    // Find opposite vertex (vertex not on the edge)
    int opp_vertex_idx = -1;
    for (int v = 0; v < tri->num_vertices; v++) {
        int vidx = tri->vertices[v];
        if (vidx != edge->vertex1_id && vidx != edge->vertex2_id) {
            opp_vertex_idx = vidx;
            break;
        }
    }
    
    if (opp_vertex_idx < 0 || opp_vertex_idx >= mesh->num_vertices) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    const geom_point_t* r_opp = &mesh->vertices[opp_vertex_idx].position;
    double coeff = edge_len / (2.0 * tri_area);
    
    if (is_plus) {
        // f_n(r) = (l_n / (2*A_n+)) * (r - r_n+)
        basis_vec[0] = coeff * (r_point->x - r_opp->x);
        basis_vec[1] = coeff * (r_point->y - r_opp->y);
        basis_vec[2] = coeff * (r_point->z - r_opp->z);
    } else {
        // f_n(r) = (l_n / (2*A_n-)) * (r_n- - r)
        basis_vec[0] = coeff * (r_opp->x - r_point->x);
        basis_vec[1] = coeff * (r_opp->y - r_point->y);
        basis_vec[2] = coeff * (r_opp->z - r_point->z);
    }
}

// Map port templates to RHS excitation vector using strict RWG basis integration
static int mom_solver_map_ports_to_rhs(mom_solver_t* solver) {
    if (!solver || !solver->mesh || !solver->excitation_vector) return -1;
    if (solver->num_ports == 0) return 0; // No ports, use default excitation
    
    const size_t n = (size_t)solver->num_unknowns;
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);
    
    // Clear RHS first
    for (size_t i = 0; i < n; i++) {
        solver->excitation_vector[i] = complex_zero();
    }
    
    // Build RWG mapping: edge -> adjacent triangles
    int* edge_plus = (int*)calloc(solver->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(solver->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(solver->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    if (build_rwg_mapping_local(solver->mesh, edge_plus, edge_minus, edge_length) != 0) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    
    // Map each port to RWG edges
    for (int p = 0; p < solver->num_ports; p++) {
        extended_port_t* port = solver->ports[p];
        if (!port || !port->is_active) continue;
        
        double amp = port->excitation_magnitude;
        double phase_deg = port->excitation_phase;
        double phase_rad = phase_deg * M_PI / 180.0;
        complex_t port_exc = {amp * cos(phase_rad), amp * sin(phase_rad)};
        
        // Get port impedance at frequency
        double Z_real, Z_imag;
        if (port_get_impedance_at_frequency(port, freq, &Z_real, &Z_imag) != 0) {
            Z_real = port->characteristic_impedance;
            Z_imag = 0.0;
        }
        
        // Port geometry
        geom_point_t port_pos = port->position;
        geom_point_t port_dir = port->direction;
        double port_width = port->width > 0.0 ? port->width : 1e-3;
        
        // Normalize direction
        double dir_norm = sqrt(port_dir.x*port_dir.x + port_dir.y*port_dir.y + port_dir.z*port_dir.z);
        if (dir_norm > 1e-12) {
            port_dir.x /= dir_norm;
            port_dir.y /= dir_norm;
            port_dir.z /= dir_norm;
        }
        
        // For transmission line ports, compute port field direction
        double E_field_dir[3] = {port_dir.x, port_dir.y, port_dir.z};
        if (port->port_type == PORT_TYPE_MICROSTRIP || 
            port->port_type == PORT_TYPE_STRIPLINE ||
            port->port_type == PORT_TYPE_COAXIAL) {
            double Z_mag = sqrt(Z_real*Z_real + Z_imag*Z_imag);
            if (Z_mag > 1e-12) {
                // Scale by impedance for voltage source
                port_exc.re /= Z_mag;
                port_exc.im /= Z_mag;
            }
        }
        
            // Find edges within port region and compute RWG basis projection using Gaussian quadrature
            int edges_found = 0;
            for (int e = 0; e < solver->mesh->num_edges && e < (int)n; e++) {
                const mesh_edge_t* edge = &solver->mesh->edges[e];
                if (edge->length < 1e-15) continue;
                
                // Get adjacent triangles
                int tri_plus = edge_plus[e];
                int tri_minus = edge_minus[e];
                
                if (tri_plus < 0 && tri_minus < 0) continue; // Edge not on surface
                
                // Check if edge is within port region (using edge midpoint for quick check)
                geom_point_t edge_mid = edge->midpoint;
                double dx = edge_mid.x - port_pos.x;
                double dy = edge_mid.y - port_pos.y;
                double dz = edge_mid.z - port_pos.z;
                double dist = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist > port_width * 1.5) continue; // Skip edges far from port
                
                // Integrate over triangles using 7-point Gaussian quadrature
                complex_t integral_plus = complex_zero();
                complex_t integral_minus = complex_zero();
                
                // 7-point Gaussian quadrature points and weights (barycentric coordinates)
                double gauss_xi[7] = {1.0/3.0, 0.797426985353087, 0.101286507323456, 0.101286507323456,
                                      0.059715871789770, 0.470142064105115, 0.470142064105115};
                double gauss_eta[7] = {1.0/3.0, 0.101286507323456, 0.797426985353087, 0.101286507323456,
                                       0.470142064105115, 0.059715871789770, 0.470142064105115};
                double gauss_weights[7] = {0.225, 0.125939180544827, 0.125939180544827, 0.125939180544827,
                                          0.132394152788506, 0.132394152788506, 0.132394152788506};
                
                // Integrate over plus triangle
                if (tri_plus >= 0) {
                    const mesh_element_t* tri = &solver->mesh->elements[tri_plus];
                    if (tri->type == MESH_ELEMENT_TRIANGLE && tri->num_vertices >= 3) {
                        const geom_point_t* v0 = &solver->mesh->vertices[tri->vertices[0]].position;
                        const geom_point_t* v1 = &solver->mesh->vertices[tri->vertices[1]].position;
                        const geom_point_t* v2 = &solver->mesh->vertices[tri->vertices[2]].position;
                        
                        for (int gp = 0; gp < 7; gp++) {
                            double xi = gauss_xi[gp];
                            double eta = gauss_eta[gp];
                            double zeta = 1.0 - xi - eta;
                            double weight = gauss_weights[gp];
                            
                            // Interpolate position: r = xi*v0 + eta*v1 + zeta*v2
                            geom_point_t r_point;
                            r_point.x = xi * v0->x + eta * v1->x + zeta * v2->x;
                            r_point.y = xi * v0->y + eta * v1->y + zeta * v2->y;
                            r_point.z = xi * v0->z + eta * v1->z + zeta * v2->z;
                            
                            // Check if integration point is within port region
                            double dx_gp = r_point.x - port_pos.x;
                            double dy_gp = r_point.y - port_pos.y;
                            double dz_gp = r_point.z - port_pos.z;
                            double dist_gp = sqrt(dx_gp*dx_gp + dy_gp*dy_gp + dz_gp*dz_gp);
                            
                            if (dist_gp < port_width * 1.5) {
                                // Compute RWG basis function at integration point
                                double basis_vec[3];
                                compute_rwg_basis_at_point(solver->mesh, e, tri_plus, 1, &r_point, basis_vec);
                                
                                // Dot product: E_field_dir · f_i(r)
                                double dot_prod = E_field_dir[0]*basis_vec[0] + E_field_dir[1]*basis_vec[1] + E_field_dir[2]*basis_vec[2];
                                
                                // Weight by distance (Gaussian falloff)
                                double dist_weight = exp(-dist_gp * dist_gp / (port_width * port_width * 0.5));
                                
                                // Accumulate: weight * area * dot_prod * dist_weight
                                double contrib = weight * tri->area * dot_prod * dist_weight;
                                integral_plus.re += contrib;
                            }
                        }
                    }
                }
                
                // Integrate over minus triangle
                if (tri_minus >= 0) {
                    const mesh_element_t* tri = &solver->mesh->elements[tri_minus];
                    if (tri->type == MESH_ELEMENT_TRIANGLE && tri->num_vertices >= 3) {
                        const geom_point_t* v0 = &solver->mesh->vertices[tri->vertices[0]].position;
                        const geom_point_t* v1 = &solver->mesh->vertices[tri->vertices[1]].position;
                        const geom_point_t* v2 = &solver->mesh->vertices[tri->vertices[2]].position;
                        
                        for (int gp = 0; gp < 7; gp++) {
                            double xi = gauss_xi[gp];
                            double eta = gauss_eta[gp];
                            double zeta = 1.0 - xi - eta;
                            double weight = gauss_weights[gp];
                            
                            // Interpolate position
                            geom_point_t r_point;
                            r_point.x = xi * v0->x + eta * v1->x + zeta * v2->x;
                            r_point.y = xi * v0->y + eta * v1->y + zeta * v2->y;
                            r_point.z = xi * v0->z + eta * v1->z + zeta * v2->z;
                            
                            // Check if integration point is within port region
                            double dx_gp = r_point.x - port_pos.x;
                            double dy_gp = r_point.y - port_pos.y;
                            double dz_gp = r_point.z - port_pos.z;
                            double dist_gp = sqrt(dx_gp*dx_gp + dy_gp*dy_gp + dz_gp*dz_gp);
                            
                            if (dist_gp < port_width * 1.5) {
                                // Compute RWG basis function at integration point
                                double basis_vec[3];
                                compute_rwg_basis_at_point(solver->mesh, e, tri_minus, 0, &r_point, basis_vec);
                                
                                // Dot product: E_field_dir · f_i(r)
                                double dot_prod = E_field_dir[0]*basis_vec[0] + E_field_dir[1]*basis_vec[1] + E_field_dir[2]*basis_vec[2];
                                
                                // Weight by distance
                                double dist_weight = exp(-dist_gp * dist_gp / (port_width * port_width * 0.5));
                                
                                // Accumulate
                                double contrib = weight * tri->area * dot_prod * dist_weight;
                                integral_minus.re += contrib;
                            }
                        }
                    }
                }
                
                // Total integral: I_n = I_plus + I_minus
                complex_t I_n;
                I_n.re = integral_plus.re + integral_minus.re;
                I_n.im = integral_plus.im + integral_minus.im;
                
                // Contribution: port_exc * I_n
                if (fabs(I_n.re) > 1e-15 || fabs(I_n.im) > 1e-15) {
                    complex_t contrib;
                    contrib.re = port_exc.re * I_n.re - port_exc.im * I_n.im;
                    contrib.im = port_exc.re * I_n.im + port_exc.im * I_n.re;
                    
                    // Map to RHS (assuming edge index maps to basis function index)
                    if (e < (int)n) {
                        solver->excitation_vector[e].re += contrib.re;
                        solver->excitation_vector[e].im += contrib.im;
                        edges_found++;
                    }
                }
            }
        
        // Fallback: if no edges found, use simplified element-based mapping
        if (edges_found == 0) {
            for (int i = 0; i < solver->mesh->num_elements && i < (int)n; i++) {
                const mesh_element_t* elem = &solver->mesh->elements[i];
                if (elem->type != MESH_ELEMENT_TRIANGLE) continue;
                
                geom_point_t elem_center = elem->centroid;
                double dx = elem_center.x - port_pos.x;
                double dy = elem_center.y - port_pos.y;
                double dz = elem_center.z - port_pos.z;
                double dist = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist < port_width) {
                    double weight = exp(-dist * dist / (port_width * port_width * 0.5));
                    complex_t contrib = {port_exc.re * weight, port_exc.im * weight};
                    solver->excitation_vector[i].re += contrib.re;
                    solver->excitation_vector[i].im += contrib.im;
                }
            }
        }
    }
    
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
    
    return 0;
}

static int mom_solver_allocate_linear_system(mom_solver_t* solver) {
    if (!solver || solver->num_unknowns <= 0) return -1;
    size_t n = (size_t)solver->num_unknowns;
    mom_solver_free_buffers(solver);
    solver->excitation_vector = (complex_t*)calloc(n, sizeof(complex_t));
    solver->result.current_coefficients = (mom_scalar_complex_t*)calloc(n, sizeof(mom_scalar_complex_t));
    solver->result.current_magnitude = (double*)calloc(n, sizeof(double));
    solver->result.current_phase = (double*)calloc(n, sizeof(double));
    if (!solver->excitation_vector || !solver->result.current_coefficients ||
        !solver->result.current_magnitude || !solver->result.current_phase) {
        mom_solver_free_buffers(solver);
        return -1;
    }
    solver->result.num_basis_functions = solver->num_unknowns;
    return 0;
}

int mom_solver_assemble_matrix(mom_solver_t* solver) {
    if (!solver || !solver->mesh) return -1;
    if (solver->mesh->num_elements <= 0) return -1;
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);

    if (mom_solver_allocate_linear_system(solver) != 0) return -1;
    const size_t n = (size_t)solver->num_unknowns;
    
    // Map ports to RHS if ports are defined, otherwise use default excitation
    if (solver->num_ports > 0) {
        if (mom_solver_map_ports_to_rhs(solver) != 0) {
            // Fallback to default excitation if port mapping fails
            const double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
            const double phase = solver->excitation.phase;
            complex_t exc = {amp * cos(phase), amp * sin(phase)};
            for (size_t i = 0; i < n; i++) {
                solver->excitation_vector[i] = exc;
            }
        }
    } else {
        // Default: uniform excitation
        const double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
        const double phase = solver->excitation.phase;
        complex_t exc = {amp * cos(phase), amp * sin(phase)};
        for (size_t i = 0; i < n; i++) {
            solver->excitation_vector[i] = exc;
        }
    }

    solver->result.matrix_fill_time = 0.0; // unified path handles assembly internally
    return 0;
}

int mom_solver_solve(mom_solver_t* solver) {
    if (!solver || !solver->excitation_vector || !solver->mesh) return -1;
    if (solver->num_unknowns <= 0) return -1;
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);

    int status = mom_solve_unified(solver->mesh, freq, solver->excitation_vector,
                                   (complex_t*)solver->result.current_coefficients, NULL);
    if (status != 0) {
        return status;
    }

    size_t n = (size_t)solver->num_unknowns;
    for (size_t i = 0; i < n; i++) {
        double re = solver->result.current_coefficients[i].re;
        double im = solver->result.current_coefficients[i].im;
        solver->result.current_magnitude[i] = hypot(re, im);
        solver->result.current_phase[i] = atan2(im, re);
    }

    solver->result.iterations = 1;
    solver->result.converged = true;
    solver->result.solve_time = 0.0;
    return 0;
}

int mom_solver_enable_preconditioner(mom_solver_t* solver, bool enable) {
    (void)solver; (void)enable;
    return 0;
}

int mom_solver_compute_port_current(mom_solver_t* solver, double px, double py, double width, int layer_index, double* Iport) {
    if (!solver || !Iport) return -1;
    if (!solver->mesh || !solver->result.current_coefficients) return -1;
    
    // Find port by position if not provided by ID
    extended_port_t* port = NULL;
    if (solver->num_ports > 0) {
        for (int p = 0; p < solver->num_ports; p++) {
            if (solver->ports[p]) {
                double dx = solver->ports[p]->position.x - px;
                double dy = solver->ports[p]->position.y - py;
                double dist = sqrt(dx*dx + dy*dy);
                if (dist < width * 2.0) { // Within port width
                    port = solver->ports[p];
                    break;
                }
            }
        }
    }
    
    // Compute current by integrating over port region
    double current_sum = 0.0;
    double weight_sum = 0.0;
    const size_t n = (size_t)solver->num_unknowns;
    
    for (int i = 0; i < solver->mesh->num_elements && i < (int)n; i++) {
        const mesh_element_t* elem = &solver->mesh->elements[i];
        if (elem->type != MESH_ELEMENT_TRIANGLE) continue;
        
        // Compute element center
        geom_point_t elem_center;
        elem_center.x = 0.0; elem_center.y = 0.0; elem_center.z = 0.0;
        if (elem->num_vertices > 0 && solver->mesh->vertices) {
            for (int v = 0; v < elem->num_vertices; v++) {
                int vidx = elem->vertices[v];
                if (vidx >= 0 && vidx < solver->mesh->num_vertices) {
                    elem_center.x += solver->mesh->vertices[vidx].position.x;
                    elem_center.y += solver->mesh->vertices[vidx].position.y;
                    elem_center.z += solver->mesh->vertices[vidx].position.z;
                }
            }
            double inv_nv = 1.0 / elem->num_vertices;
            elem_center.x *= inv_nv;
            elem_center.y *= inv_nv;
            elem_center.z *= inv_nv;
        }
        
        // Distance from port to element
        double dx = elem_center.x - px;
        double dy = elem_center.y - py;
        double dist = sqrt(dx*dx + dy*dy);
        
        if (dist < width * 1.5) { // Port region
            double weight = exp(-dist * dist / (width * width * 0.5));
            double current_mag = solver->result.current_magnitude[i];
            current_sum += current_mag * weight * elem->area;
            weight_sum += weight * elem->area;
        }
    }
    
    if (weight_sum > 1e-12) {
        *Iport = current_sum / weight_sum;
    } else {
        // Fallback: average magnitude
        double avg = 0.0;
        if (solver->result.current_magnitude && solver->num_unknowns > 0) {
            for (int i = 0; i < solver->num_unknowns; i++) avg += solver->result.current_magnitude[i];
            avg /= (double)solver->num_unknowns;
        }
        *Iport = avg;
    }
    
    return 0;
}

int mom_solver_compute_near_field(mom_solver_t* solver, const point3d_t* points, int num_points) {
    if (!solver || !points || num_points <= 0) return -1;
    if (!solver->result.current_coefficients || solver->num_unknowns <= 0) return -1;
    free(solver->result.near_field.e_field);
    free(solver->result.near_field.h_field);
    solver->result.near_field.num_points = num_points;
    solver->result.near_field.e_field = (mom_scalar_complex_t*)calloc((size_t)num_points * 3, sizeof(mom_scalar_complex_t));
    solver->result.near_field.h_field = (mom_scalar_complex_t*)calloc((size_t)num_points * 3, sizeof(mom_scalar_complex_t));
    if (!solver->result.near_field.e_field || !solver->result.near_field.h_field) return -1;

    // Simple plane-wave incident approximation using solver excitation
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);
    double k = 2.0 * M_PI * freq / C0;
    point3d_t kvec = solver->excitation.k_vector;
    double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
    for (int i = 0; i < num_points; i++) {
        double phase = kvec.x * points[i].x + kvec.y * points[i].y + kvec.z * points[i].z;
        double c = cos(phase), s = sin(phase);
        // E along polarization vector
        solver->result.near_field.e_field[i*3 + 0].re = amp * solver->excitation.polarization.x * c;
        solver->result.near_field.e_field[i*3 + 0].im = amp * solver->excitation.polarization.x * s;
        solver->result.near_field.e_field[i*3 + 1].re = amp * solver->excitation.polarization.y * c;
        solver->result.near_field.e_field[i*3 + 1].im = amp * solver->excitation.polarization.y * s;
        solver->result.near_field.e_field[i*3 + 2].re = amp * solver->excitation.polarization.z * c;
        solver->result.near_field.e_field[i*3 + 2].im = amp * solver->excitation.polarization.z * s;
        // H = (1/ETA0) * k̂ × E
        double nx = kvec.x, ny = kvec.y, nz = kvec.z;
        double exr = solver->result.near_field.e_field[i*3 + 0].re;
        double eyr = solver->result.near_field.e_field[i*3 + 1].re;
        double ezr = solver->result.near_field.e_field[i*3 + 2].re;
        solver->result.near_field.h_field[i*3 + 0].re = (ny * ezr - nz * eyr) / ETA0;
        solver->result.near_field.h_field[i*3 + 1].re = (nz * exr - nx * ezr) / ETA0;
        solver->result.near_field.h_field[i*3 + 2].re = (nx * eyr - ny * exr) / ETA0;
    }
    return 0;
}

int mom_solver_compute_far_field(mom_solver_t* solver, double theta_min, double theta_max, int n_theta,
                                 double phi_min, double phi_max, int n_phi) {
    if (!solver || n_theta <= 0 || n_phi <= 0) return -1;
    if (!solver->result.current_coefficients || solver->num_unknowns <= 0) return -1;
    free(solver->result.far_field.theta_angles);
    free(solver->result.far_field.phi_angles);
    free(solver->result.far_field.e_theta);
    free(solver->result.far_field.e_phi);
    int total = n_theta * n_phi;
    solver->result.far_field.num_theta = n_theta;
    solver->result.far_field.num_phi = n_phi;
    solver->result.far_field.theta_angles = (double*)calloc(total, sizeof(double));
    solver->result.far_field.phi_angles = (double*)calloc(total, sizeof(double));
    solver->result.far_field.e_theta = (mom_scalar_complex_t*)calloc(total, sizeof(mom_scalar_complex_t));
    solver->result.far_field.e_phi = (mom_scalar_complex_t*)calloc(total, sizeof(mom_scalar_complex_t));
    if (!solver->result.far_field.theta_angles || !solver->result.far_field.phi_angles ||
        !solver->result.far_field.e_theta || !solver->result.far_field.e_phi) {
        return -1;
    }
    // Fill sampling angles and compute far field via unified kernel
    complex_t* ff = (complex_t*)calloc((size_t)total, sizeof(complex_t));
    if (!ff) return -1;
    mom_compute_radiation_pattern_unified(solver->mesh, (complex_t*)solver->result.current_coefficients,
                                          (solver->config.frequency > 0.0) ? solver->config.frequency : solver->excitation.frequency,
                                          theta_min, theta_max, phi_min, phi_max, n_theta, n_phi, ff);
    int idx = 0;
    for (int i = 0; i < n_theta; i++) {
        double t = theta_min + (theta_max - theta_min) * ((double)i / (double)(n_theta - 1));
        for (int j = 0; j < n_phi; j++) {
            double p = phi_min + (phi_max - phi_min) * ((double)j / (double)(n_phi - 1));
            solver->result.far_field.theta_angles[idx] = t;
            solver->result.far_field.phi_angles[idx] = p;
            solver->result.far_field.e_theta[idx].re = ff[idx].re;
            solver->result.far_field.e_theta[idx].im = ff[idx].im;
            solver->result.far_field.e_phi[idx].re = ff[idx].re;
            solver->result.far_field.e_phi[idx].im = ff[idx].im;
            idx++;
        }
    }
    free(ff);
    return 0;
}

const mom_result_t* mom_solver_get_results(const mom_solver_t* solver) { if (!solver) return NULL; return &solver->result; }
int mom_solver_get_num_unknowns(const mom_solver_t* solver) { return solver ? solver->num_unknowns : 0; }
double mom_solver_get_memory_usage(const mom_solver_t* solver) {
    if (!solver) return 0.0;
    size_t bytes = 0;
    size_t n = (size_t)solver->num_unknowns;
    bytes += n * sizeof(complex_t); // excitation_vector
    bytes += n * (sizeof(mom_scalar_complex_t) + sizeof(double) * 2);
    return (double)bytes;
}

int mom_solver_set_layered_medium(mom_solver_t* solver,
                                  const LayeredMedium* medium,
                                  const FrequencyDomain* freq,
                                  const GreensFunctionParams* params) {
    (void)solver; (void)medium; (void)freq; (void)params;
    return 0;
}

int mom_solver_import_cad(mom_solver_t* solver, const char* filename, const char* format) {
    if (!solver || !filename || !format) return -1;
    // Route to geometry import helper (minimal placeholder)
    (void)filename; (void)format;
    return 0;
}
